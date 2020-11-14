#include <thor-internal/arch/cpu.hpp>
#include <thor-internal/arch/ints.hpp>
#include <thor-internal/arch/gic.hpp>
#include <thor-internal/debug.hpp>
#include <thor-internal/thread.hpp>
#include <assert.h>

namespace thor {

extern "C" void *thorExcVectors;

void initializeIrqVectors() {
	asm volatile ("msr vbar_el1, %0" :: "r"(&thorExcVectors));
}

void suspendSelf() { assert(!"Not implemented"); }

extern frg::manual_box<GicDistributor> dist;

void sendPingIpi(int id) {
	// TODO: The GIC cpu id *may* differ from the normal cpu id,
	// get the id from the GIC and store it in the cpu local data and
	// use that here
	dist->sendIpi(id, 0);
}

extern "C" void onPlatformInvalidException(FaultImageAccessor image) {
	thor::panicLogger() << "thor: an invalid exception has occured" << frg::endlog;
}

namespace {
	Word mmuAbortError(uint64_t esr) {
		Word errorCode = 0;

		auto ec = esr >> 26;
		auto iss = esr & ((1 << 25) - 1);

		// Originated from EL0
		if (ec == 0x20 || ec == 0x24)
			errorCode |= kPfUser;

		// Is an instruction abort
		if (ec == 0x20 || ec == 0x21) {
			errorCode |= kPfInstruction;
		} else {
			if (iss & (1 << 6))
				errorCode |= kPfWrite;
		}

		auto sc = iss & 0x3F;

		if (sc < 16) {
			auto type = (sc >> 2) & 0b11;
			if (type == 0) // Address size fault
				errorCode |= kPfBadTable;
			if (type != 1) // Not a translation fault
				errorCode |= kPfAccess;
		}

		return errorCode;
	}
} // namespace anonymous

void handlePageFault(FaultImageAccessor image, uintptr_t address, Word errorCode);
void handleOtherFault(FaultImageAccessor image, Interrupt fault);
void handleSyscall(SyscallImageAccessor image);

extern "C" void onPlatformSyncFault(FaultImageAccessor image) {
	auto ec = *image.code() >> 26;

	switch (ec) {
		case 0x20: // Instruction abort, lower EL
		case 0x21: // Instruction abort, same EL
		case 0x24: // Data abort, lower EL
		case 0x25: // Data abort, same EL
			handlePageFault(image, *image.faultAddr(),
					mmuAbortError(*image.code()));
			break;
		case 0x15: // Trapped SVC in AArch64
			handleSyscall(image);
			break;
		case 0x30: // Breakpoint, lower EL
		case 0x31: // Breakpoint, same EL
			handleOtherFault(image, kIntrBreakpoint);
			break;
		case 0x0E: // Illegal Execution fault
		case 0x22: // IP alignment fault
		case 0x26: // SP alignment fault
			handleOtherFault(image, kIntrGeneralFault);
			break;
		default:
			panicLogger() << "Unexpected fault " << ec
				<< " from ip: " << (void *)*image.ip() << "\n"
				<< "sp: " << (void *)*image.sp()
				<< "syndrome: 0x" << frg::hex_fmt(*image.code())
				<< "saved state: 0x" << frg::hex_fmt(*image.rflags()) << frg::endlog;
	}
}

extern "C" void onPlatformAsyncFault(FaultImageAccessor image) {
	infoLogger() << "onPlatformAsyncFault" << frg::endlog;
}

extern frg::manual_box<GicCpuInterface> cpuInterface;

void handleIrq(IrqImageAccessor image, int number);
void handlePreemption(IrqImageAccessor image);

static constexpr bool logSGIs = false;
static constexpr bool logSpurious = false;

extern "C" void onPlatformIrq(IrqImageAccessor image) {
	auto [cpu, irq] = cpuInterface->get();

	if (irq < 16) {
		if constexpr (logSGIs)
			infoLogger() << "thor: onPlatformIrq: got a SGI (no. " << irq << ") that originated from cpu " << cpu << frg::endlog;

		cpuInterface->eoi(cpu, irq);
		handlePreemption(image);
		return;
	}

	if (irq >= 1020) {
		if constexpr (logSpurious)
			infoLogger() << "thor: spurious irq " << irq << " occured" << frg::endlog;
		// no need to EOI spurious irqs
		return;
	}

	handleIrq(image, irq);
}

} // namespace thor
