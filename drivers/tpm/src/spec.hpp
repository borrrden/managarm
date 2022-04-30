#pragma once

#include <arch/register.hpp>
#include <arch/variable.hpp>

//-------------------------------------------------
// registers
//-------------------------------------------------

namespace tpm::crb_regs {
    arch::bit_register<uint32_t> loc_state(0x0);
    arch::bit_register<uint32_t> loc_ctrl(0x8);
    arch::bit_register<uint32_t> loc_sts(0xC);
    arch::bit_register<uint64_t> intf_id(0x30);
    arch::bit_register<uint64_t> ctrl_ext(0x38);
    arch::bit_register<uint32_t> ctrl_req(0x40);
    arch::bit_register<uint32_t> ctrl_sts(0x44);
    arch::scalar_register<uint32_t> ctrl_cancel(0x48);
    arch::scalar_register<uint32_t> ctrl_start(0x4C);
    arch::bit_register<uint32_t> crb_int_enable(0x50);
    arch::bit_register<uint32_t> int_sts(0x54);
    arch::scalar_register<uint32_t> ctrl_cmd_size(0x58);
    arch::scalar_register<uint32_t> ctrl_cmd_laddr(0x5C);
    arch::scalar_register<uint32_t> ctrl_cmd_haddr(0x60);
    arch::scalar_register<uint32_t> ctrl_rsp_size(0x64);
    arch::scalar_register<uint32_t> ctrl_rsp_addr(0x68);
    arch::scalar_register<uint32_t> data_buffer(0x80);
}

namespace tpm::fifo_regs {
    arch::bit_register<uint8_t> access(0x0);
    arch::bit_register<uint32_t> int_enable(0x8);
    arch::bit_register<uint8_t> int_vector(0xC);
    arch::bit_register<uint32_t> int_status(0x10);
    arch::bit_register<uint32_t> intf_capability(0x14);
    arch::bit_register<uint32_t> sts(0x18);
    arch::bit_register<uint32_t> data(0x24);
    arch::bit_register<uint32_t> interface_id(0x30);
    arch::scalar_register<uint32_t> xdata(0x80);
    arch::bit_register<uint32_t> did_vid(0xF00);
    arch::scalar_register<uint8_t> rid(0xF04);
}

namespace tpm::div_vid {
    arch::field<uint32_t, uint16_t> vid(0, 16);
    arch::field<uint32_t, uint16_t> did(16, 16);
}

namespace tpm {
    enum class intf_type {
        fifo = 0b0000,
        crb = 0b0001,
        tis_13 = 0b1111
    };
}

namespace tpm::interface_id {
    arch::field<uint32_t, intf_type> interface_type(0, 4);
    arch::field<uint32_t, uint8_t> interface_version(4, 4);
    arch::field<uint32_t, bool> cap_locality(8, 1);
    arch::field<uint32_t, bool> cap_tis(13, 1);
    arch::field<uint32_t, bool> cap_crb(14, 1);
    arch::field<uint32_t, uint8_t> cap_if_res(15,2);
    arch::field<uint32_t, uint8_t> interface_selector(17, 2);
    arch::field<uint32_t, bool> intf_sel_lock(19, 1);
}

// Note, largely mirrors interface_id in order to allow hardware
// to support both interfaces
namespace tpm::intf_if {
    arch::field<uint64_t, intf_type> interface_type(0, 4);
    arch::field<uint64_t, uint8_t> interface_version(4, 4);
    arch::field<uint64_t, bool> cap_locality(8, 1);
    arch::field<uint64_t, bool> cap_idle_bypass(9, 1);
    arch::field<uint64_t, uint8_t> cap_data_xfer_size_support(11, 2);
    arch::field<uint64_t, bool> cap_fifo(13, 1);
    arch::field<uint64_t, bool> cap_crb(14, 1);
    arch::field<uint64_t, uint8_t> cap_if_res(15,2);
    arch::field<uint64_t, uint8_t> interface_selector(17, 2);
    arch::field<uint64_t, bool> intf_sel_lock(19, 1);
    arch::field<uint64_t, uint8_t> rid(24, 8);
    arch::field<uint64_t, uint16_t> vid(32, 16);
    arch::field<uint64_t, uint16_t> did(48, 16);
}

namespace tpm::access {
    arch::field<uint32_t, bool> establishment(0, 1);
    arch::field<uint32_t, bool> request_use(1, 1);
    arch::field<uint32_t, bool> pending_request(2, 1);
    arch::field<uint32_t, bool> seize(3, 1);
    arch::field<uint32_t, bool> been_seized(4, 1);
    arch::field<uint32_t, bool> active_locality(5, 1);
    arch::field<uint32_t, bool> reg_valid_sts(7, 1);
}

namespace tpm::sts {
    enum class family : uint8_t {
        v12 = 0b00,
        v20 = 0b01
    };

    /*-w*/ arch::field<uint32_t, bool> response_retry(1, 1);
    /*r-*/ arch::field<uint32_t, bool> self_test_done(2, 1);
    /*r-*/ arch::field<uint32_t, bool> expect(3, 1);
    /*r-*/ arch::field<uint32_t, bool> data_avail(4, 1);
    /*-w*/ arch::field<uint32_t, bool> go(5, 1);
    /*rw*/ arch::field<uint32_t, bool> command_ready(6, 1);
    /*r-*/ arch::field<uint32_t, bool> sts_valid(7, 1);
    /*r-*/ arch::field<uint32_t, uint16_t> burst_count(8, 16);
    /*-w*/ arch::field<uint32_t, bool> command_cancel(24, 1);
    /*-w*/ arch::field<uint32_t, bool> reset_establishment_bit(25, 1);
    /*r-*/ arch::field<uint32_t, family> tpm_family(26, 2); 
}

namespace tpm::data { 
    // Reads return command *response* data
    // Writes set command *send* data
    /*rw*/ arch::field<uint32_t, uint8_t> data(0, 8);
}

namespace tpm::intf_capability {
    enum class data_transfer_size : uint8_t {
        legacy = 0b00,
        byte8 = 0b01,
        byte32 = 0b10,
        byte64 = 0b11
    };

    enum class ifc_version : uint8_t {
        v121 = 0b000,
        v13 = 0b010,
        v13_tpm2 = 0b011
    };

    /*r-*/ arch::field<uint32_t, bool> data_avail_int_support(0, 1);
    /*r-*/ arch::field<uint32_t, bool> sts_valid_int_support(1, 1);
    /*r-*/ arch::field<uint32_t, bool> locality_change_int_support(2, 1);
    /*r-*/ arch::field<uint32_t, bool> interrupt_level_high(3, 1);
    /*r-*/ arch::field<uint32_t, bool> interrupt_level_low(4, 1);
    /*r-*/ arch::field<uint32_t, bool> interrupt_edge_rising(5, 1);
    /*r-*/ arch::field<uint32_t, bool> interrupt_edge_falling(6, 1);
    /*r-*/ arch::field<uint32_t, bool> command_ready_int_support(7, 1);
    /*r-*/ arch::field<uint32_t, bool> burst_count_static(8, 1);
    /*r-*/ arch::field<uint32_t, data_transfer_size> data_transfer_size_support(9, 2);
    /*r-*/ arch::field<uint32_t, ifc_version> interface_version(28, 3);
}

namespace tpm::loc_state {
    /*r-*/ arch::field<uint32_t, bool> established(0, 1);
    /*r-*/ arch::field<uint32_t, bool> ioc_assigned(1, 1);
    /*r-*/ arch::field<uint32_t, uint8_t> active_locality(2, 3);
    /*r-*/ arch::field<uint32_t, bool> reg_valid_sts(7, 1);
}

namespace tpm::loc_ctrl {
    /*-w*/ arch::field<uint32_t, bool> request_access(0, 1);
    /*-w*/ arch::field<uint32_t, bool> relinquish(1, 1);
    /*-w*/ arch::field<uint32_t, bool> seize(2, 1);
    /*-w*/ arch::field<uint32_t, bool> reset_establishment_bit(3, 1);
}

// Locality 4 is basically equivalent to "ring 0" of the TPM
// and it has special registers (probably not relevant to the system
// since locality 4 is only supposed to be used directly by the 
// platform firmware)
namespace tpm::loc_ctrl_4 {
    /*-w*/ arch::field<uint32_t, bool> hash_start(0, 1);
    /*-w*/ arch::field<uint32_t, bool> hash_data(1, 1);
    /*-w*/ arch::field<uint32_t, bool> hash_end(2, 1);
    /*-w*/ arch::field<uint32_t, bool> reset_establishment(3, 1);
}

namespace tpm::loc_sts {
    /*r-*/ arch::field<uint32_t, bool> granted(0, 1);
    /*r-*/ arch::field<uint32_t, bool> been_seized(1, 1);
}

namespace tpm::ctrl_ext {
    /*r-*/ arch::field<uint64_t, uint32_t> clear(0, 32);
    /*rw*/ arch::field<uint64_t, uint32_t> remaining_bytes(32, 32);
}

namespace tpm::ctrl_req {
    /*rw*/ arch::field<uint32_t, bool> cmd_ready(0, 1);
    /*rw*/ arch::field<uint32_t, bool> go_idle(1, 1);
}

namespace tpm::ctrl_sts {
    enum class status : bool {
        operational = false,
        fatal_error = true
    };

    /*r-*/ arch::field<uint32_t, status> tpm_sts(0, 1);
    /*r-*/ arch::field<uint32_t, bool> tpm_idle(1, 1);
}

namespace tpm::int_enable {
    enum class polarity : uint8_t {
        high_level = 0b00,
        low_level = 0b01,
        rising_edge = 0b10,
        falling_edge = 0b11
    };

    /*rw*/ arch::field<uint32_t, bool> data_avail_enable(0, 1);
    /*rw*/ arch::field<uint32_t, bool> sts_valid_enable(1, 1);
    /*rw*/ arch::field<uint32_t, bool> locality_change_enable(2, 1);
    /*rw*/ arch::field<uint32_t, polarity> type_polarity(3, 2);
    /*rw*/ arch::field<uint32_t, bool> command_ready_enable(7, 1);
    /*rw*/ arch::field<uint32_t, bool> global_enable(31, 1);
}

namespace tpm::int_status {
    /*rw*/ arch::field<uint32_t, bool> data_avail(0, 1);
    /*rw*/ arch::field<uint32_t, bool> sts_valid(1, 1);
    /*rw*/ arch::field<uint32_t, bool> locality_change(2, 1);
    /*rw*/ arch::field<uint32_t, bool> command_ready(7, 1);
}

namespace tpm::int_vector {
    /*rw*/ arch::field<uint32_t, uint8_t> sirq_vec(0, 4);
}

namespace tpm::crb_int_enable {
    /*rw*/ arch::field<uint32_t, bool> start_int_enable(0, 1);
    /*rw*/ arch::field<uint32_t, bool> cmd_ready_enable(1, 1);
    /*rw*/ arch::field<uint32_t, bool> establishment_clear_enable(2, 1);
    /*rw*/ arch::field<uint32_t, bool> locality_change_enable(3, 1);
    /*rw*/ arch::field<uint32_t, bool> global_enable(31, 1);
}

namespace tpm::int_sts {
    /*rw*/ arch::field<uint32_t, bool> start(0, 1);
    /*rw*/ arch::field<uint32_t, bool> cmd_ready(1, 1);
    /*rw*/ arch::field<uint32_t, bool> establishment_clear(2, 1);
    /*rw*/ arch::field<uint32_t, bool> locality_change(3, 1);
}