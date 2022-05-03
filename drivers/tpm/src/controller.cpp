#include <chrono>

#include <byteswap.h>
#include <endian.h>

#include "controller.hpp"
#include "command.hpp"
#include "spec.hpp"

using namespace std::literals::chrono_literals;

namespace tpm {

template<typename T>
inline async::result<void> sleepFor(T duration) {
	uint64_t tick;
	HEL_CHECK(helGetClock(&tick));

	auto ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
	helix::AwaitClock await;
	auto &&submit = helix::submitAwaitClock(&await, tick + ticks.count(),
			helix::Dispatcher::global());
	co_await submit.async_wait();
	HEL_CHECK(await.error());
}

template<typename T>
inline async::result<bool> waitLoop(T duration, int pollAttempts, 
std::function<bool()> condition) {
	auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
	auto attempts = 0;
	do {
		if(condition()) {
			co_return true;
		}

		co_await sleepFor(nanos / pollAttempts);
	} while(++attempts < pollAttempts);

	co_return false;
}

void chunkTransfer(uint8_t *dst, const uint8_t *src, size_t size, 
intf_id::DataTransferSize transferSize) {
	if(transferSize != intf_id::Byte4) {
		memcpy(dst, src, size);
		return;
	}

	while(size >= 4) {
		auto *wideBuf = (uint32_t *)src;
		arch::mem_ops<uint32_t>::store((uint32_t *)dst, *wideBuf);
		size /= 4;
		src += 4;
		dst += 4;
	}

	while(size) {
		arch::mem_ops<uint8_t>::store(dst, *src);
		size--;
		src++;
		dst++;
	}
}

struct CRBControllerInterface final : public ControllerInterface {
	CRBControllerInterface(
		helix::Mapping &mmioMapping,
		helix::UniqueDescriptor &mmioDesciptor,
		arch::mem_space &space) 
	:ControllerInterface(mmioMapping, mmioDesciptor, space) {
		auto interfaceId = space_.load(crb_regs::intfId);
		auto hasLocalities = interfaceId & intf_id::capLocality;
		if(hasLocalities) {
			std::cout << "tpm: Device has locality support, selecting Locality 1" << std::endl;
			// If localities are available, use locality 1
			locality_ = 1;
			space_ = space_.subspace(0x1000);
		} else {
			std::cout << "tpm: Device has no locality support, selecting Locality 0" << std::endl;
		}

		dataTransferSize_ = interfaceId & intf_id::capDataXferSizeSupport;
		std::cout << "tpm: Data transfer size is " << (int)dataTransferSize_ << std::endl;
		
		auto physicalCmdBuffer = (uint8_t *)(((uint64_t)space_.load(crb_regs::ctrlCmdHaddr) << 32)
			| space_.load(crb_regs::ctrlCmdLaddr));
		auto physicalRspBuffer = (uint8_t *)space_.load(crb_regs::ctrlRspAddr);

		commandBuffer_ = (uint8_t *)mapping_.get() + 0x80;
		responseBuffer_ = commandBuffer_;
		
		// TPM devices compliant with the PC Client Profile spec guarantee
		// these all to be pointing to the data buffer region of MMIO.  However,
		// They are physical addresses, so do some sanity checking to make sure
		// the virtual MMIO lines up.
		uintptr_t physicalMapping;
		assert(helPointerPhysical(commandBuffer_, &physicalMapping) == kHelErrNone);
		assert((uintptr_t)physicalCmdBuffer == physicalMapping);
		assert((uintptr_t)physicalRspBuffer == physicalMapping);
	}

	uint16_t deviceId() const override {
		return space_.load(crb_regs::intfId) & intf_id::did;
	}

	uint16_t vendorId() const override {
		auto vid = space_.load(crb_regs::intfId) & intf_id::vid;
		return be16toh(vid);
	}

	uint8_t revisionId() const override {
		return space_.load(crb_regs::intfId) & intf_id::rid;
	}

	async::result<std::shared_ptr<GenericPacket>> 
	sendCommand(const uint8_t *buf, size_t size) override {
		std::cout << "tpm: Sending command" << std::endl;
		auto controlStatus = space_.load(crb_regs::ctrlSts);
		if((controlStatus & ctrl_sts::tpmSts) == ctrl_sts::FatalError) {
			co_return std::shared_ptr<GenericPacket>(new GenericPacket {{ 0, 0, RC_FAILURE }});
		}

		std::cout << "tpm: Device is operational" << std::endl;
		auto localityAchieved = co_await requestLocality();
		if(!localityAchieved) {
			co_return std::shared_ptr<GenericPacket>(new GenericPacket {{ 0, 0, RC_FAILURE }});
		}

		std::cout << "tpm: Locality accessed" << std::endl;
		auto idleToReadyResult = co_await idleToReady();
		if(!idleToReadyResult) {
			co_return std::shared_ptr<GenericPacket>(new GenericPacket {{ 0, 0, RC_FAILURE }});
		}

		std::cout << "tpm: Device is ready to receive command" << std::endl;
		chunkTransfer(commandBuffer_, buf, size, dataTransferSize_);

		auto executionResult = co_await startExecution(timeouts::CMD_DURATION_LONG,
			timeouts::CMD_TIMEOUT_LONG);
		if(!executionResult) {
			co_return std::shared_ptr<GenericPacket>(new GenericPacket {{ 0, 0, RC_FAILURE }});
		}

		std::cout << "tpm: Device finished executing command" << std::endl;
		
		PacketHeader header;
		chunkTransfer((uint8_t *)&header, responseBuffer_, sizeof(PacketHeader), dataTransferSize_);
		auto *packet = (GenericPacket *)malloc(header.size);
		packet->header = header;
		chunkTransfer(packet->data, responseBuffer_ + sizeof(PacketHeader), 
			be32toh(header.size) - sizeof(PacketHeader), dataTransferSize_);
		auto returnPacket = std::shared_ptr<GenericPacket>(packet);
		

		auto readyToIdleResult = co_await readyToIdle();
		if(!readyToIdleResult) {
			co_return std::shared_ptr<GenericPacket>(new GenericPacket {{ 0, 0, RC_FAILURE }});
		}

		std::cout << "tpm: Device is idle" << std::endl;

		co_return returnPacket;
	}

private:
	uint8_t *commandBuffer_;
	uint8_t *responseBuffer_;
	uint8_t locality_ {0};
	intf_id::DataTransferSize dataTransferSize_;

	async::result<bool> requestLocality() {
		auto locState = space_.load(crb_regs::locState);

		auto pollResult = co_await waitLoop(500us, 5, [this, &locState] {
			locState = space_.load(crb_regs::locState);
			return locState & loc_state::regValidSts;
		});

		if(!pollResult) {
			// TPM is irrecoverably broken
			co_return false;
		}

		if((locState & loc_state::locAssigned) &&
			(locState & loc_state::activeLocality) == locality_) {
			co_return true;
		}

		space_.store(crb_regs::locCtrl, loc_ctrl::requestAccess(true));
		
		// TODO: Seize from locality 0 if it is misbehaving
		// TODO: Interrupt based
		do {
			locState = space_.load(crb_regs::locState);
			if((locState & loc_state::activeLocality) == 1) {
				break;
			}

			co_await sleepFor(100ms); // Arbitrary...
		} while(true);

		co_return true;
	}

	async::result<bool> idleToReady() {
		auto controlStatus = space_.load(crb_regs::ctrlSts);
		if((controlStatus & ctrl_sts::tpmIdle)) {
			space_.store(crb_regs::ctrlReq, ctrl_req::cmdReady(true));
			auto transitionResult = co_await waitLoop(timeouts::INTERFACE_TIMEOUT_C, 20, 
			[this, &controlStatus] {
				controlStatus = space_.load(crb_regs::ctrlSts);
				return !(controlStatus & ctrl_sts::tpmIdle);
			});

			co_return transitionResult;
		} else {
			co_return true;
		}
	}

	async::result<bool> startExecution(const std::chrono::milliseconds& duration, 
	const std::chrono::milliseconds& timeout) {
		space_.store(crb_regs::ctrlStart, 1);
		return waitLoop(timeout, timeout / duration, [this] {
			auto sts = space_.load(crb_regs::ctrlStart);
			return sts == 0;
		});
	}

	async::result<bool> readyToIdle() {
		auto controlStatus = space_.load(crb_regs::ctrlSts);
		if(!(controlStatus & ctrl_sts::tpmIdle)) {
			space_.store(crb_regs::ctrlReq, ctrl_req::goIdle(true));
			auto transitionResult = co_await waitLoop(timeouts::INTERFACE_TIMEOUT_C, 20, 
			[this, &controlStatus] {
				controlStatus = space_.load(crb_regs::ctrlSts);
				return controlStatus & ctrl_sts::tpmIdle;
			});

			co_return transitionResult;
		} else {
			co_return true;
		}
	}
};

std::unique_ptr<ControllerInterface> make_interface(helix::Mapping &mmioMapping, 
		helix::UniqueDescriptor &mmioDesciptor){
	arch::mem_space space{mmioMapping.get()};
	auto interface_id = space.load(fifo_regs::interfaceId);
	switch(interface_id & interface_id::interfaceType) {
		case TpmInterface::FIFO_TPM2:
			// TODO
			std::cout << "tpm: FIFO is not supported" << std::endl;
			return nullptr;
		case TpmInterface::CRB_TPM2:
			return std::make_unique<CRBControllerInterface>(mmioMapping, mmioDesciptor, space);
		case TpmInterface::FIFO_TIS13:
			std::cout << "tpm: TIS 1.3 is not supported!" << std::endl;
			return nullptr;
		default:
			assert(!"Unknown interface");
	}
}

smarter::shared_ptr<Controller> Controller::make_controller(
	protocols::hw::BusDevice &device,
	helix::Mapping &mmioMapping,
	helix::UniqueDescriptor &mmioDesciptor
) { 
	auto interface = make_interface(mmioMapping, mmioDesciptor);
	if(!interface) {
		return nullptr;
	}

	return smarter::make_shared<Controller>(device, interface);
}

async::result<uint32_t> Controller::start() {
	auto cmd = makeStartCommand(SU_STATE);
	auto result = co_await sendCommand((uint8_t *)&cmd, be32toh(cmd.header.size));
	co_return result->header.code;
}

async::result<size_t> Controller::write(const uint8_t *buf, size_t size) {
	std::cout << "tpm: command size is " << size << std::endl;
	auto result = co_await sendCommand(buf, size);
	auto responseSize = be32toh(result->header.size);
	lastResponseOffset_ = 0;
	lastResponse_.resize(responseSize);
	memcpy(lastResponse_.data(), result.get(), responseSize);
	co_return size;
}

size_t Controller::read(uint8_t *buf, size_t size) {
	if(lastResponseOffset_ == lastResponse_.size()) {
		return 0;
	}

	if(lastResponse_.size() - lastResponseOffset_ < size) {
		size = lastResponse_.size() - lastResponseOffset_;
	}

	memcpy(buf, lastResponse_.data() + lastResponseOffset_, size);
	lastResponseOffset_ += size;
	return size;
}

}