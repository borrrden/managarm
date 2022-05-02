#include <thor-internal/fiber.hpp>
#include <thor-internal/io.hpp>
#include <thor-internal/kernel_heap.hpp>
#include <thor-internal/address-space.hpp>
#include <thor-internal/acpi/acpi.hpp>
#include <thor-internal/main.hpp>
#include <thor-internal/stream.hpp>
#include <mbus.frigg_pb.hpp>
#include <hw.frigg_bragi.hpp>

#include <lai/core.h>
#include <bragi/helpers-all.hpp>
#include <bragi/helpers-frigg.hpp>

namespace thor {

// TODO: Move this to a header file.
extern frg::manual_box<LaneHandle> mbusClient;

namespace {

enum StartMethod : uint32_t {
	AcpiStart = 2,
	MMIO = 6,
	CRB = 7,
	CRB_AcpiStart = 8,
	CRB_SMC_HVC = 11,
	FIFO_I2C = 12
};

constexpr uint32_t TPM_PTP_BASE_ADDRESS = 0xfed40000;

coroutine<Error> handleRequest(LaneHandle boundLane, uintptr_t acpiMmio) {
	auto sendResponse = [] (LaneHandle &conversation,
			managarm::hw::SvrResponse<KernelAlloc> &&resp) -> coroutine<frg::tuple<Error, Error>> {
		frg::unique_memory<KernelAlloc> respHeadBuffer{*kernelAlloc,
			resp.head_size};

		frg::unique_memory<KernelAlloc> respTailBuffer{*kernelAlloc,
			resp.size_of_tail()};

		bragi::write_head_tail(resp, respHeadBuffer, respTailBuffer);

		auto respHeadError = co_await SendBufferSender{conversation, std::move(respHeadBuffer)};
		auto respTailError = co_await SendBufferSender{conversation, std::move(respTailBuffer)};

		co_return {respHeadError, respTailError};
	};

	auto [acceptError, lane] = co_await AcceptSender{boundLane};
	if(acceptError != Error::success)
		co_return acceptError;

	auto [reqError, reqBuffer] = co_await RecvBufferSender{lane};
		if(reqError != Error::success)
			co_return reqError;

	auto preamble = bragi::read_preamble(reqBuffer);
	assert(!preamble.error());

	if(preamble.id() == bragi::message_id<managarm::hw::GetMemoryRegionsRequest>) {
		auto req = bragi::parse_head_only<managarm::hw::GetMemoryRegionsRequest>(reqBuffer, *kernelAlloc);

		if (!req) {
			infoLogger() << "thor: Closing lane due to illegal HW request." << frg::endlog;
			co_return Error::protocolViolation;
		}

		managarm::hw::SvrResponse<KernelAlloc> resp{*kernelAlloc};
		resp.set_error(managarm::hw::Errors::SUCCESS);

		managarm::hw::BusDeviceMemoryInfo<KernelAlloc> region{*kernelAlloc};
		region.set_tag(frg::string<KernelAlloc>(*kernelAlloc, "localities"));
		region.set_type(managarm::hw::IoType::MEMORY);
		region.set_address(acpiMmio);
		region.set_length(0x5000);
		resp.add_regions(region);

		auto [headError, tailError] = co_await sendResponse(lane, std::move(resp));

		if(headError != Error::success)
			co_return headError;
		if(tailError != Error::success)
			co_return tailError;
	}else if(preamble.id() == bragi::message_id<managarm::hw::AccessMemoryRequest>) {
		auto req = bragi::parse_head_only<managarm::hw::AccessMemoryRequest>(reqBuffer, *kernelAlloc);

		if (!req) {
			infoLogger() << "thor: Closing lane due to illegal HW request." << frg::endlog;
			co_return Error::protocolViolation;
		}

		managarm::hw::SvrResponse<KernelAlloc> resp{*kernelAlloc};

		smarter::shared_ptr<MemoryView> space;
		if(req->index() > 0) {
			resp.set_error(managarm::hw::Errors::OUT_OF_BOUNDS);
		} else {
			managarm::hw::SvrResponse<KernelAlloc> resp{*kernelAlloc};
			space = smarter::allocate_shared<HardwareMemory>(*kernelAlloc, acpiMmio, 0x5000, thor::CachingMode::mmioNonPosted);
			resp.set_error(managarm::hw::Errors::SUCCESS);
		}

		auto [headError, tailError] = co_await sendResponse(lane, std::move(resp));
		if(headError != Error::success)
			co_return headError;
		if(tailError != Error::success)
			co_return tailError;

		auto memError = co_await PushDescriptorSender{lane, MemoryViewDescriptor{space}};
		if(memError != Error::success) 
			co_return memError;
	}else{
		infoLogger() << "thor: Dismissing conversation due to illegal HW request." << frg::endlog;
		co_await DismissSender{lane};
		co_return Error::protocolViolation;
	}

	co_return Error::success;
}

coroutine<LaneHandle> createObject(LaneHandle mbusLane, unsigned major, unsigned minor) { 
	auto [offerError, conversation] = co_await OfferSender{mbusLane};
	// TODO: improve error handling here.
	assert(offerError == Error::success);

	managarm::mbus::Property<KernelAlloc> cls_prop(*kernelAlloc);
	cls_prop.set_name(frg::string<KernelAlloc>(*kernelAlloc, "class"));
	auto& cls_item = cls_prop.mutable_item().mutable_string_item();
	cls_item.set_value(frg::string<KernelAlloc>(*kernelAlloc, "tpm"));

	managarm::mbus::CntRequest<KernelAlloc> req(*kernelAlloc);
	req.set_req_type(managarm::mbus::CntReqType::CREATE_OBJECT);
	req.set_parent_id(1);
	req.add_properties(std::move(cls_prop));

	frg::string<KernelAlloc> ser(*kernelAlloc);
	req.SerializeToString(&ser);
	frg::unique_memory<KernelAlloc> reqBuffer{*kernelAlloc, ser.size()};
	memcpy(reqBuffer.data(), ser.data(), ser.size());
	auto reqError = co_await SendBufferSender{conversation, std::move(reqBuffer)};
	// TODO: improve error handling here.
	assert(reqError == Error::success);

	auto [respError, respBuffer] = co_await RecvBufferSender{conversation};
	// TODO: improve error handling here.
	assert(respError == Error::success);
	managarm::mbus::SvrResponse<KernelAlloc> resp(*kernelAlloc);
	resp.ParseFromArray(respBuffer.data(), respBuffer.size());
	assert(resp.error() == managarm::mbus::Error::SUCCESS);

	auto [descError, descriptor] = co_await PullDescriptorSender{conversation};
	// TODO: improve error handling here.
	assert(descError == Error::success);
	assert(descriptor.is<LaneDescriptor>());
	co_return descriptor.get<LaneDescriptor>().handle;
}

coroutine<void> handleBind(LaneHandle objectLane, uintptr_t acpiMmio) {  
	auto [acceptError, lane] = co_await AcceptSender{objectLane};
	assert(acceptError == Error::success && "Unexpected mbus transaction");

	auto [reqError, reqBuffer] = co_await RecvBufferSender{lane};
	assert(reqError == Error::success && "Unexpected mbus transaction");
	managarm::mbus::SvrRequest<KernelAlloc> req(*kernelAlloc);
	req.ParseFromArray(reqBuffer.data(), reqBuffer.size());
	assert(req.req_type() == managarm::mbus::SvrReqType::BIND
				&& "Unexpected mbus transaction");

	auto stream = createStream();
	managarm::mbus::CntResponse<KernelAlloc> resp(*kernelAlloc); 
	resp.set_error(managarm::mbus::Error::SUCCESS);

	frg::string<KernelAlloc> ser(*kernelAlloc);
	resp.SerializeToString(&ser);
	frg::unique_memory<KernelAlloc> respBuffer{*kernelAlloc, ser.size()};
	memcpy(respBuffer.data(), ser.data(), ser.size());
	auto respError = co_await SendBufferSender{lane, std::move(respBuffer)};
	assert(respError == Error::success && "Unexpected mbus transaction");
	auto boundError = co_await PushDescriptorSender{lane, LaneDescriptor{stream.get<1>()}};
	assert(boundError == Error::success && "Unexpected mbus transaction");
	auto boundLane = stream.get<0>();
	while(true) {
		// Terminate the connection on protocol errors.
		auto error = co_await handleRequest(boundLane, acpiMmio);
		if(error == Error::endOfLane)
			break;
		if(isRemoteIpcError(error))
			infoLogger() << "thor: Aborting tpm request"
					" after remote violated the protocol" << frg::endlog;
		assert(error == Error::success);
	}
}

} // anonymous namespace

static initgraph::Task discoverTpmDevices{&globalInitEngine, "tpm.discover-device",
	initgraph::Requires{acpi::getNsAvailableStage()},
	[] {
		auto *tpm2Window = laihost_scan("TPM2", 0);
		if(!tpm2Window) {
			// TODO: TPM 1.2
			return;
		}
		
		auto tpm2 = reinterpret_cast<acpi_tpm2_t *>(tpm2Window);
		KernelFiber::run([=] {
			async::detach_with_allocator(*kernelAlloc, [=]() -> coroutine<void> { 
				auto objectLane = co_await createObject(*mbusClient, 2, 0);
				auto mmio = tpm2->mmio_area;

				if(tpm2->start_method == StartMethod::MMIO && tpm2->mmio_area == 0) {
					// TCG ACPI General Spec v1.3 p. 10:
					// For FIFO interface with a fixed physical base address as
					// defined in PC Client PTP Specification, [mmio_area]
					// MAY be set to zero.
					// TODO: This might include FIFO_I2C?
					mmio = TPM_PTP_BASE_ADDRESS;
				} else {
					mmio = tpm2->mmio_area / kPageSize * kPageSize;
				}

				while(true)
					co_await handleBind(objectLane, mmio);
			}());
		});
	}	
};
} // namespace thor