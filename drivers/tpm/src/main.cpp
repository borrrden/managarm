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

#include "controller.hpp"

namespace tpm {

std::vector<smarter::shared_ptr<Controller>> globalControllers;

async::result<protocols::fs::ReadResult>
read(void *f, const char *, void *buffer, size_t length) {
	auto *controller = reinterpret_cast<Controller *>(f);
	co_return controller->read((uint8_t *)buffer, length);
}

async::result<frg::expected<protocols::fs::Error, size_t>>
write(void *f, const char *, const void *buffer, size_t length) {
	auto *controller = reinterpret_cast<Controller *>(f);
	auto result = co_await controller->write((uint8_t *)buffer, length);
	co_return result;
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
			helix::UniqueLane localLane, remoteLane;
			std::tie(localLane, remoteLane) = helix::createStream();
			async::detach(protocols::fs::servePassthrough(
					std::move(localLane), globalControllers[index], &fileOperations));

			managarm::fs::SvrResponse resp;
			resp.set_error(managarm::fs::Errors::SUCCESS);

			auto ser = resp.SerializeAsString();
			auto [sendResp, pushNode] = co_await helix_ng::exchangeMsgs(conversation,
				helix_ng::sendBuffer(ser.data(), ser.size()),
				helix_ng::pushDescriptor(remoteLane)
			);
			HEL_CHECK(sendResp.error());
			HEL_CHECK(pushNode.error());
		}else{
			throw std::runtime_error("Invalid serveBridge request!");
		}
	}
}

async::detached bindController(mbus::Entity entity) {
	auto tpmDevice = protocols::hw::BusDevice(co_await entity.bind());
	auto mmio = co_await tpmDevice.accessMemory(0);
	helix::Mapping mapping{mmio, 0, 0x5000};
	auto controller = Controller::make_controller(tpmDevice, mapping, mmio);
	if(!controller) {
		co_return;
	}

	std::cout << "Controller registered for TPM2 device:" << std::endl;
	std::cout << "\tVendor ID 0x" << std::hex << controller->vendorId() << std::endl;
	std::cout << "\tDevice ID 0x" << controller->deviceId() << std::endl;
	std::cout << "\tRevision ID 0x" << (int)controller->revisionId() << std::dec << std::endl;
	auto startResult = co_await controller->start();
	if(startResult != RC_SUCCESS && startResult != RC_INITIALIZE) {
		std::cout << "Failed to start TPM device (" << be32toh(startResult) << ")" << std::endl;
		co_return;
	}

	globalControllers.push_back(controller);
	auto index = globalControllers.size() - 1;
	auto root = co_await mbus::Instance::global().getRoot();

	mbus::Properties descriptor{
		{"generic.devtype", mbus::StringItem{"block"}},
		{"generic.devname", mbus::StringItem{"tpm0"}}
	};

	auto handler = mbus::ObjectHandler{}
	.withBind([=] () -> async::result<helix::UniqueDescriptor> {
		helix::UniqueLane localLane, remoteLane;
		std::tie(localLane, remoteLane) = helix::createStream();
		serveBridge(std::move(localLane), index);

		co_return std::move(remoteLane);
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