#include <iostream>
#include <vector>

#include <arch/mem_space.hpp>
#include <async/oneshot-event.hpp>
#include <async/result.hpp>
#include <helix/memory.hpp>
#include <protocols/fs/server.hpp>
#include <protocols/hw/client.hpp>
#include <protocols/mbus/client.hpp>

#include <fs.bragi.hpp>

#include "spec.hpp"

namespace tpm {

struct Controller {
    Controller(protocols::hw::BusDevice& hw_device,
        helix::Mapping& mapping,
        helix::UniqueDescriptor& mmio) 
    :_hw_device(std::move(hw_device))
    ,_mapping(std::move(mapping))
    ,_mmio(std::move(mmio))
    ,_space(mapping.get())
    {
        auto intf = _space.load(fifo_regs::interface_id) & interface_id::interface_type;
        switch(intf) {
            case intf_type::fifo:
                std::cout << "tpm: FIFO interface active" << std::endl;
                break;
            case intf_type::crb:
                std::cout << "tpm: CRB interface active" << std::endl;
                break;
            case intf_type::tis_13:
                std::cout << "tpm: PTP 1.3 interface active" << std::endl;
                break;
            default:
                assert(!"Unknown interface");
        }
    }

private:
    protocols::hw::BusDevice _hw_device;
    helix::Mapping _mapping;
    helix::UniqueDescriptor _mmio;
    arch::mem_space _space;
};

std::vector<smarter::shared_ptr<Controller>> globalControllers;

async::result<protocols::fs::ReadResult>
read(void *f, const char *, void *buffer, size_t length) {
	co_return size_t{0};
}

async::result<frg::expected<protocols::fs::Error, size_t>>
write(void *, const char *, const void *buffer, size_t length) {
    co_return length;
}

constexpr auto fileOperations = protocols::fs::FileOperations{
    .read = &read,
    .write = &write,
};

async::detached serveBridge(helix::UniqueLane lane, size_t index) {
    std::cout << "tpm: Connection made" << std::endl;

    while(true) {
        auto [accept, recv_req] = co_await helix_ng::exchangeMsgs(lane,
			helix_ng::accept(
				helix_ng::recvInline())
		);
		HEL_CHECK(accept.error());
		HEL_CHECK(recv_req.error());

		auto conversation = accept.descriptor();

        managarm::fs::CntRequest req;
		req.ParseFromArray(recv_req.data(), recv_req.length());
		if(req.req_type() == managarm::fs::CntReqType::DEV_OPEN) {
            helix::UniqueLane local_lane, remote_lane;
            std::tie(local_lane, remote_lane) = helix::createStream();
            async::detach(protocols::fs::servePassthrough(
					std::move(local_lane), globalControllers[index], &fileOperations));

            managarm::fs::SvrResponse resp;
			resp.set_error(managarm::fs::Errors::SUCCESS);

            auto ser = resp.SerializeAsString();
			auto [send_resp, push_node] = co_await helix_ng::exchangeMsgs(conversation,
				helix_ng::sendBuffer(ser.data(), ser.size()),
				helix_ng::pushDescriptor(remote_lane)
			);
			HEL_CHECK(send_resp.error());
			HEL_CHECK(push_node.error());
        }else{
			throw std::runtime_error("Invalid serveBridge request!");
		}
    }
}

async::detached bindController(mbus::Entity entity) {
    auto tpmDevice = protocols::hw::BusDevice(co_await entity.bind());
    auto mmio = co_await tpmDevice.accessMemory(0);
    helix::Mapping mapping{mmio, 0, 0x1000};
    auto controller = smarter::make_shared<Controller>(tpmDevice, mapping, mmio);
    globalControllers.push_back(controller);
    auto index = globalControllers.size() - 1;
    auto root = co_await mbus::Instance::global().getRoot();

    mbus::Properties descriptor{
        {"generic.devtype", mbus::StringItem{"block"}},
        {"generic.devname", mbus::StringItem{"tpm0"}}
    };

    auto handler = mbus::ObjectHandler{}
    .withBind([=] () -> async::result<helix::UniqueDescriptor> {
        helix::UniqueLane local_lane, remote_lane;
        std::tie(local_lane, remote_lane) = helix::createStream();
        serveBridge(std::move(local_lane), index);

        co_return std::move(remote_lane);
    });

    co_await root.createObject("tpmdev", descriptor, std::move(handler));
}

async::detached observeControllers() {
    auto root = co_await mbus::Instance::global().getRoot();

    auto filter = mbus::Conjunction({
        mbus::EqualsFilter("class", "tpm")
    });

    auto handler = mbus::ObserverHandler{}
    .withAttach([] (mbus::Entity entity, mbus::Properties) {
        std::cout << "drivers/tpm: Found TPM" << std::endl;

        bindController(entity);
    });

    co_await root.linkObserver(std::move(filter), std::move(handler));
}

} // namespace tpm

int main() {
    std::cout << "tpm: Starting driver" << std::endl;

    tpm::observeControllers();
    async::run_forever(helix::currentDispatcher);
}