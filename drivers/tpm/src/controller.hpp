#pragma once

#include <memory>

#include <arch/mem_space.hpp>
#include <helix/memory.hpp>
#include <protocols/hw/client.hpp>
#include <smarter.hpp>

#include "command.hpp"

namespace tpm {

struct ControllerInterface {
	ControllerInterface(const ControllerInterface &) = delete;
	ControllerInterface(ControllerInterface &&) = delete;
	virtual ~ControllerInterface() = default;

	virtual async::result<std::unique_ptr<PacketHeader>> 
		sendCommand(const uint8_t *buf, size_t size) = 0;

	virtual uint16_t deviceId() const = 0;
	virtual uint16_t vendorId() const = 0;
	virtual uint8_t revisionId() const = 0;

protected:	
	ControllerInterface(
		helix::Mapping &mmioMapping,
		helix::UniqueDescriptor &mmioDescriptor,
		const arch::mem_space &space)
		:mapping_(std::move(mmioMapping))
		,mmio_(std::move(mmioDescriptor))
		,space_(space)
	{}

	helix::Mapping mapping_;
	helix::UniqueDescriptor mmio_;
	arch::mem_space space_;
};

struct Controller {
	static smarter::shared_ptr<Controller> make_controller(
		protocols::hw::BusDevice &device,
		helix::Mapping &mmioMapping,
		helix::UniqueDescriptor &mmioDesciptor
	);

	Controller(
		protocols::hw::BusDevice &device,
		std::unique_ptr<ControllerInterface> &interface)
		:hw_device_(std::move(device))
		,interface_(std::move(interface))
	{}

	Controller() = delete;
	Controller(const Controller &) = delete;
	Controller(Controller &&) = delete;

	async::result<uint16_t> start();
	inline async::result<std::unique_ptr<PacketHeader>> sendCommand(const uint8_t *buf, size_t size)
		{return interface_->sendCommand(buf, size);}

	inline uint16_t deviceId() const { return interface_->deviceId(); }
	inline uint16_t vendorId() const { return interface_->vendorId(); }
	inline uint8_t revisionId() const { return interface_->revisionId(); }

private:
	protocols::hw::BusDevice hw_device_;
	std::unique_ptr<ControllerInterface> interface_;
};

}