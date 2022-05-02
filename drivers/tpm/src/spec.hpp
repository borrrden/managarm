#pragma once

#include <arch/register.hpp>
#include <arch/variable.hpp>
#include <chrono>

//-------------------------------------------------
// registers
//-------------------------------------------------

namespace tpm::crb_regs {
	arch::bit_register<uint32_t> locState(0x0);
	arch::bit_register<uint32_t> locCtrl(0x8);
	arch::bit_register<uint32_t> locSts(0xC);
	arch::bit_register<uint64_t> intfId(0x30);
	// Only available to pre-OS firmware
	/*arch::bit_register<uint64_t> ctrlEx(0x38);*/
	arch::bit_register<uint32_t> ctrlReq(0x40);
	arch::bit_register<uint32_t> ctrlSts(0x44);
	arch::scalar_register<uint32_t> ctrlCancel(0x48);
	arch::scalar_register<uint32_t> ctrlStart(0x4C);
	arch::bit_register<uint32_t> crbIntEnable(0x50);
	arch::bit_register<uint32_t> intSts(0x54);
	arch::scalar_register<uint32_t> ctrlCmdSize(0x58);
	arch::scalar_register<uint32_t> ctrlCmdLaddr(0x5C);
	arch::scalar_register<uint32_t> ctrlCmdHaddr(0x60);
	arch::scalar_register<uint32_t> ctrlRspSize(0x64);
	arch::scalar_register<uint64_t> ctrlRspAddr(0x68);
}

namespace tpm::fifo_regs {
	arch::bit_register<uint8_t> access(0x0);
	arch::bit_register<uint32_t> intEnable(0x8);
	arch::bit_register<uint8_t> intVector(0xC);
	arch::bit_register<uint32_t> intStatus(0x10);
	arch::bit_register<uint32_t> intfCapability(0x14);
	arch::bit_register<uint32_t> sts(0x18);
	arch::bit_register<uint32_t> data(0x24);
	arch::bit_register<uint32_t> interfaceId(0x30);
	arch::scalar_register<uint32_t> xdata(0x80);
	arch::bit_register<uint32_t> didVid(0xF00);
	arch::scalar_register<uint8_t> rid(0xF04);
}

namespace tpm::div_vid {
	arch::field<uint32_t, uint16_t> vid(0, 16);
	arch::field<uint32_t, uint16_t> did(16, 16);
}

namespace tpm {
	enum TpmInterface : uint8_t {
		FIFO_TPM2 = 0b0000,
		CRB_TPM2 = 0b0001,
		FIFO_TIS13 = 0b1111
	};

	enum Tpm2Interface : uint8_t {
		TIS = 0b00,
		CRB = 0b01
	};
}

namespace tpm::interface_id {
	/*r-*/ arch::field<uint32_t, TpmInterface> interfaceType(0, 4);
	/*r-*/ arch::field<uint32_t, uint8_t> interfaceVersion(4, 4);
	/*r-*/ arch::field<uint32_t, bool> capLocality(8, 1);
	/*r-*/ arch::field<uint32_t, bool> capTIS(13, 1);
	/*r-*/ arch::field<uint32_t, bool> capCRB(14, 1);
	/*r-*/ arch::field<uint32_t, uint8_t> capIfRes(15,2);
	/*rw*/ arch::field<uint32_t, Tpm2Interface> interfaceSelector(17, 2);
	/*rw*/ arch::field<uint32_t, bool> intfSelLock(19, 1);
}

// Note, largely mirrors interface_id in order to allow hardware
// to support both interfaces
namespace tpm::intf_id {
	/*r-*/ arch::field<uint64_t, TpmInterface> interfaceType(0, 4);
	/*r-*/ arch::field<uint64_t, uint8_t> interfaceVersion(4, 4);
	/*r-*/ arch::field<uint64_t, bool> capLocality(8, 1);
	/*r-*/ arch::field<uint64_t, bool> capIdleBypass(9, 1);
	/*r-*/ arch::field<uint64_t, uint8_t> capDataXferSizeSupport(11, 2);
	/*r-*/ arch::field<uint64_t, bool> capFIFO(13, 1);
	/*r-*/ arch::field<uint64_t, bool> capCRB(14, 1);
	/*r-*/ arch::field<uint64_t, uint8_t> capIfRes(15,2);
	/*rw*/ arch::field<uint64_t, Tpm2Interface> interfaceSelector(17, 2);
	/*rw*/ arch::field<uint64_t, bool> intfSelLock(19, 1);
	/*r-*/ arch::field<uint64_t, uint8_t> rid(24, 8);
	/*r-*/ arch::field<uint64_t, uint16_t> vid(32, 16);
	/*r-*/ arch::field<uint64_t, uint16_t> did(48, 16);
}

namespace tpm::access {
	/*r-*/ arch::field<uint32_t, bool> establishment(0, 1);
	/*rw*/ arch::field<uint32_t, bool> requestUse(1, 1);
	/*r-*/ arch::field<uint32_t, bool> pendingRequest(2, 1);
	/*-w*/ arch::field<uint32_t, bool> seize(3, 1);
	/*rw*/ arch::field<uint32_t, bool> beenSeized(4, 1);
	/*rw*/ arch::field<uint32_t, bool> activeLocality(5, 1);
	/*r-*/ arch::field<uint32_t, bool> regValidSts(7, 1);
}

namespace tpm::sts {
	enum Family : uint8_t {
		V12 = 0b00,
		V20 = 0b01
	};

	/*-w*/ arch::field<uint32_t, bool> responseRetry(1, 1);
	/*r-*/ arch::field<uint32_t, bool> selfTestDone(2, 1);
	/*r-*/ arch::field<uint32_t, bool> expect(3, 1);
	/*r-*/ arch::field<uint32_t, bool> dataAvail(4, 1);
	/*-w*/ arch::field<uint32_t, bool> go(5, 1);
	/*rw*/ arch::field<uint32_t, bool> commandReady(6, 1);
	/*r-*/ arch::field<uint32_t, bool> stsValid(7, 1);
	/*r-*/ arch::field<uint32_t, uint16_t> burstCount(8, 16);
	/*-w*/ arch::field<uint32_t, bool> commandCancel(24, 1);
	/*-w*/ arch::field<uint32_t, bool> resetEstablishmentBit(25, 1);
	/*r-*/ arch::field<uint32_t, Family> tpmFamily(26, 2); 
}

namespace tpm::data { 
	// Reads return command *response* data
	// Writes set command *send* data
	/*rw*/ arch::field<uint32_t, uint8_t> data(0, 8);
}

namespace tpm::intf_capability {
	enum DataTransferSize : uint8_t {
		Legacy = 0b00,
		Byte8 = 0b01,
		Byte32 = 0b10,
		Byte64 = 0b11
	};

	enum IfcVersion : uint8_t {
		V121 = 0b000,
		V13 = 0b010,
		V13_TPM2 = 0b011
	};

	/*r-*/ arch::field<uint32_t, bool> dataAvailIntSupport(0, 1);
	/*r-*/ arch::field<uint32_t, bool> stsValidIntSupport(1, 1);
	/*r-*/ arch::field<uint32_t, bool> localityChangeIntSupport(2, 1);
	/*r-*/ arch::field<uint32_t, bool> interruptLevelHigh(3, 1);
	/*r-*/ arch::field<uint32_t, bool> interruptLevelLow(4, 1);
	/*r-*/ arch::field<uint32_t, bool> interruptEdgeRising(5, 1);
	/*r-*/ arch::field<uint32_t, bool> interruptEdgeFalling(6, 1);
	/*r-*/ arch::field<uint32_t, bool> commandReadyIntSupport(7, 1);
	/*r-*/ arch::field<uint32_t, bool> burstCountStatic(8, 1);
	/*r-*/ arch::field<uint32_t, DataTransferSize> dataTransferSizeSupport(9, 2);
	/*r-*/ arch::field<uint32_t, IfcVersion> interfaceVersion(28, 3);
}

namespace tpm::loc_state {
	/*r-*/ arch::field<uint32_t, bool> established(0, 1);
	/*r-*/ arch::field<uint32_t, bool> locAssigned(1, 1);
	/*r-*/ arch::field<uint32_t, uint8_t> activeLocality(2, 3);
	/*r-*/ arch::field<uint32_t, bool> regValidSts(7, 1);
}

namespace tpm::loc_ctrl {
	/*-w*/ arch::field<uint32_t, bool> requestAccess(0, 1);
	/*-w*/ arch::field<uint32_t, bool> relinquish(1, 1);
	/*-w*/ arch::field<uint32_t, bool> seize(2, 1);
	/*-w*/ arch::field<uint32_t, bool> resetEstablishmentBit(3, 1);
}

// Locality 4 is basically equivalent to "ring 0" of the TPM
// and it has special registers (probably not relevant to the system
// since locality 4 is only supposed to be used directly by the 
// platform firmware)
namespace tpm::loc_ctrl_4 {
	/*-w*/ arch::field<uint32_t, bool> hashStart(0, 1);
	/*-w*/ arch::field<uint32_t, bool> hashData(1, 1);
	/*-w*/ arch::field<uint32_t, bool> hashEnd(2, 1);
	/*-w*/ arch::field<uint32_t, bool> resetEstablishment(3, 1);
}

namespace tpm::loc_sts {
	/*r-*/ arch::field<uint32_t, bool> granted(0, 1);
	/*r-*/ arch::field<uint32_t, bool> beenSeized(1, 1);
}

namespace tpm::ctrl_req {
	/*rw*/ arch::field<uint32_t, bool> cmdReady(0, 1);
	/*rw*/ arch::field<uint32_t, bool> goIdle(1, 1);
}

namespace tpm::ctrl_sts {
	enum Status : bool {
		Operational = false,
		FatalError = true
	};

	/*r-*/ arch::field<uint32_t, Status> tpmSts(0, 1);
	/*r-*/ arch::field<uint32_t, bool> tpmIdle(1, 1);
}

namespace tpm::int_enable {
	enum Polarity : uint8_t {
		HighLevel = 0b00,
		LowLevel = 0b01,
		RisingEdge = 0b10,
		FallingEdge = 0b11
	};

	/*rw*/ arch::field<uint32_t, bool> dataAvailEnable(0, 1);
	/*rw*/ arch::field<uint32_t, bool> stsValidEnable(1, 1);
	/*rw*/ arch::field<uint32_t, bool> localityChangeEnable(2, 1);
	/*rw*/ arch::field<uint32_t, Polarity> typePolarity(3, 2);
	/*rw*/ arch::field<uint32_t, bool> commandReadyEnable(7, 1);
	/*rw*/ arch::field<uint32_t, bool> globalEnable(31, 1);
}

namespace tpm::int_status {
	/*rw*/ arch::field<uint32_t, bool> dataAvail(0, 1);
	/*rw*/ arch::field<uint32_t, bool> stsValid(1, 1);
	/*rw*/ arch::field<uint32_t, bool> localityChange(2, 1);
	/*rw*/ arch::field<uint32_t, bool> commandReady(7, 1);
}

namespace tpm::int_vector {
	/*rw*/ arch::field<uint32_t, uint8_t> sirqVec(0, 4);
}

namespace tpm::crb_int_enable {
	/*rw*/ arch::field<uint32_t, bool> startEnable(0, 1);
	/*rw*/ arch::field<uint32_t, bool> cmdReadyEnable(1, 1);
	/*rw*/ arch::field<uint32_t, bool> establishmentClearEnable(2, 1);
	/*rw*/ arch::field<uint32_t, bool> localityChangeEnable(3, 1);
	/*rw*/ arch::field<uint32_t, bool> globalEnable(31, 1);
}

namespace tpm::int_sts {
	/*rw*/ arch::field<uint32_t, bool> start(0, 1);
	/*rw*/ arch::field<uint32_t, bool> cmdReady(1, 1);
	/*rw*/ arch::field<uint32_t, bool> establishmentClear(2, 1);
	/*rw*/ arch::field<uint32_t, bool> localityChange(3, 1);
}

namespace tpm::timeouts {
	using namespace std::literals::chrono_literals;

	// 6.5.1.3 Command Duration
	constexpr std::chrono::duration CMD_DURATION_SHORT 	= 20ms;
	constexpr std::chrono::duration CMD_DURATION_LONG	= 750ms;
	constexpr std::chrono::duration CMD_TIMEOUT_SHORT	= 750ms;
	constexpr std::chrono::duration CMD_TIMEOUT_LONG	= 2000ms;

	// 6.5.1.4 Interface Timeouts
	constexpr std::chrono::duration INTERFACE_TIMEOUT_A = 750ms;
	constexpr std::chrono::duration INTERFACE_TIMEOUT_B = 2000ms;
	constexpr std::chrono::duration INTERFACE_TIMEOUT_C = 200ms;
	constexpr std::chrono::duration INTERFACE_TIMEOUT_D = 30ms;
}