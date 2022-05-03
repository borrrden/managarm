#pragma once

#include <cstdint>
#include <byteswap.h>
#include <endian.h>

namespace tpm {

constexpr uint16_t TAG_NULL			= htobe16(0x8000);
constexpr uint16_t TAG_NO_SESSIONS 	= htobe16(0x8001); 
constexpr uint16_t TAG_SESSIONS 	= htobe16(0x8002);

constexpr uint32_t CC_FIRST						= htobe32(0x0000011F);
constexpr uint32_t CC_PP_FIRST					= CC_FIRST;
constexpr uint32_t CC_NV_UNDEFINE_SPECIAL_SPACE	= CC_FIRST;
constexpr uint32_t CC_EVICT_CONTROL				= htobe32(0x00000120);
constexpr uint32_t CC_HIERARCHY_CONTROL			= htobe32(0x00000121);
constexpr uint32_t CC_NV_UNDEFINE_SPACE			= htobe32(0x00000122);
constexpr uint32_t CC_CHANGE_EPS				= htobe32(0x00000124);
constexpr uint32_t CC_CHANGE_PPS				= htobe32(0x00000125);
constexpr uint32_t CC_CLEAR						= htobe32(0x00000126);
constexpr uint32_t CC_CLEAR_CONTROL				= htobe32(0x00000127);
constexpr uint32_t CC_HIERARCHY_CHANGE_AUTH		= htobe32(0x00000129);
constexpr uint32_t CC_NV_DEFINE_SPACE			= htobe32(0x0000012A);
constexpr uint32_t CC_PCR_ALLOCATE				= htobe32(0x0000012B);
constexpr uint32_t CC_PCR_SET_AUTH_POLICY		= htobe32(0x0000012C);
constexpr uint32_t CC_PP_COMMANDS				= htobe32(0x0000012D);
constexpr uint32_t CC_SET_PRIMARY_POLICY		= htobe32(0x0000012E);
constexpr uint32_t CC_FIELD_UPGRADE_START		= htobe32(0x0000012F);
constexpr uint32_t CC_CLOCK_RATE_ADJUST			= htobe32(0x00000130);
constexpr uint32_t CC_CREATE_PRIMARY			= htobe32(0x00000131);
constexpr uint32_t CC_NV_GLOBAL_WRITE_LOCK		= htobe32(0x00000132);
constexpr uint32_t CC_PP_LAST					= htobe32(0x00000132);
constexpr uint32_t CC_GET_COMMAND_AUDIT_DIGEST	= htobe32(0x00000133);
constexpr uint32_t CC_NV_INCREMENT				= htobe32(0x00000134);
constexpr uint32_t CC_NV_SETBITS				= htobe32(0x00000135);
constexpr uint32_t CC_NV_EXTEND					= htobe32(0x00000136);
constexpr uint32_t CC_NV_WRITE					= htobe32(0x00000137);
constexpr uint32_t CC_NV_WRITE_LOCK				= htobe32(0x00000138);
constexpr uint32_t CC_DICT_ATTACK_LOCK_RESET	= htobe32(0x00000139);
constexpr uint32_t CC_DICT_ATTACK_PARAMETERS	= htobe32(0x0000013A);
constexpr uint32_t CC_NV_CHANGE_AUTH			= htobe32(0x0000013B);
constexpr uint32_t CC_PCR_EVENT					= htobe32(0x0000013C);
constexpr uint32_t CC_PCR_RESET					= htobe32(0x0000013D);
constexpr uint32_t CC_SEQUENCE_COMPLETE			= htobe32(0x0000013E);
constexpr uint32_t CC_SET_ALGORITHM_SET			= htobe32(0x0000013F);
constexpr uint32_t CC_SET_CMD_CODE_AUDIT_STATUS	= htobe32(0x00000140);
constexpr uint32_t CC_FIELD_UPGRADE_DATA		= htobe32(0x00000141);
constexpr uint32_t CC_INCREMENTAL_SELF_TEST		= htobe32(0x00000142);
constexpr uint32_t CC_SELF_TEST					= htobe32(0x00000143);
constexpr uint32_t CC_STARTUP 					= htobe32(0x00000144);
constexpr uint32_t CC_SHUTDOWN					= htobe32(0x00000145);
constexpr uint32_t CC_STIR_RANDOM				= htobe32(0x00000146);
constexpr uint32_t CC_ACTIVATE_CREDENTIAL		= htobe32(0x00000147);
constexpr uint32_t CC_CERTIFY					= htobe32(0x00000148);
constexpr uint32_t CC_POLICY_NV					= htobe32(0x00000149);
constexpr uint32_t CC_CERTIFY_CREATION			= htobe32(0x0000014A);
constexpr uint32_t CC_DUPLICATE					= htobe32(0x0000014B);
constexpr uint32_t CC_GET_TIME					= htobe32(0x0000014C);
constexpr uint32_t CC_GET_SESSION_AUDIT_DIGEST	= htobe32(0x0000014D);
constexpr uint32_t CC_NV_READ					= htobe32(0x0000014E);
constexpr uint32_t CC_NV_READ_LOCK				= htobe32(0x0000014F);
constexpr uint32_t CC_OBJECT_CHANGE_AUTH		= htobe32(0x00000150);
constexpr uint32_t CC_POLICY_SECRET				= htobe32(0x00000151);
constexpr uint32_t CC_REWRAP					= htobe32(0x00000152);
constexpr uint32_t CC_CREATE					= htobe32(0x00000153);
constexpr uint32_t CC_ECDH_ZGEN					= htobe32(0x00000154);
constexpr uint32_t CC_HMAC						= htobe32(0x00000155);
constexpr uint32_t CC_IMPORT					= htobe32(0x00000156);
constexpr uint32_t CC_LOAD						= htobe32(0x00000157);
constexpr uint32_t CC_QUOTE						= htobe32(0x00000158);
constexpr uint32_t CC_RSA_DECRYPT				= htobe32(0x00000159);
constexpr uint32_t CC_HMAC_START				= htobe32(0x0000015B);
constexpr uint32_t CC_SEQUENCE_UPDATE			= htobe32(0x0000015C);
constexpr uint32_t CC_SIGN						= htobe32(0x0000015D);
constexpr uint32_t CC_UNSEAL					= htobe32(0x0000015E);
constexpr uint32_t CC_POLICY_SIGNED				= htobe32(0x00000160);
constexpr uint32_t CC_CONTEXT_LOAD				= htobe32(0x00000161);
constexpr uint32_t CC_CONTEXT_SAVE				= htobe32(0x00000162);
constexpr uint32_t CC_ECDH_KEYGEN				= htobe32(0x00000163);
constexpr uint32_t CC_ENCRYPT_DECRYPT			= htobe32(0x00000164);
constexpr uint32_t CC_FLUSH_CONTEXT				= htobe32(0x00000165);
constexpr uint32_t CC_LOAD_EXTERNAL				= htobe32(0x00000167);
constexpr uint32_t CC_MAKE_CREDENTIAL			= htobe32(0x00000168);
constexpr uint32_t CC_NV_READ_PUBLIC			= htobe32(0x00000169);
constexpr uint32_t CC_POLICY_AUTHORIZE			= htobe32(0x0000016A);
constexpr uint32_t CC_POLICY_AUTH_VALUE			= htobe32(0x0000016B);
constexpr uint32_t CC_POLICY_COMMAND_CODE		= htobe32(0x0000016C);
constexpr uint32_t CC_POLICY_COUNTER_TIMER		= htobe32(0x0000016D);
constexpr uint32_t CC_POLICY_CP_HASH			= htobe32(0x0000016E);
constexpr uint32_t CC_POLICY_LOCALITY			= htobe32(0x0000016F);
constexpr uint32_t CC_POLICY_NAME_HASH			= htobe32(0x00000170);
constexpr uint32_t CC_POLICY_OR					= htobe32(0x00000171);
constexpr uint32_t CC_POLICY_TICKET				= htobe32(0x00000172);
constexpr uint32_t CC_READ_PUBLIC				= htobe32(0x00000173);
constexpr uint32_t CC_RSA_ENCRYPT				= htobe32(0x00000174);
constexpr uint32_t CC_START_AUTH_SESSION		= htobe32(0x00000176);
constexpr uint32_t CC_VERIFY_SIGNATURE			= htobe32(0x00000177);
constexpr uint32_t CC_ECC_PARAMETERS			= htobe32(0x00000178);
constexpr uint32_t CC_FIRMWARE_READ				= htobe32(0x00000179);
constexpr uint32_t CC_GET_CAPABILITY			= htobe32(0x0000017A);
constexpr uint32_t CC_GET_RANDOM				= htobe32(0x0000017B);
constexpr uint32_t CC_GET_TEST_RESULT			= htobe32(0x0000017C);
constexpr uint32_t CC_HASH						= htobe32(0x0000017D);
constexpr uint32_t CC_PCR_READ					= htobe32(0x0000017E);
constexpr uint32_t CC_POLICY_PCR				= htobe32(0x0000017F);
constexpr uint32_t CC_POLICY_RESTART			= htobe32(0x00000180);
constexpr uint32_t CC_READ_CLOCK				= htobe32(0x00000181);
constexpr uint32_t CC_PCR_EXTEND				= htobe32(0x00000182);
constexpr uint32_t CC_PCR_SET_AUTH_VALUE		= htobe32(0x00000183);
constexpr uint32_t CC_NV_CERTIFY				= htobe32(0x00000184);
constexpr uint32_t CC_EVENT_SEQUENCE_COMPLETE	= htobe32(0x00000185);
constexpr uint32_t CC_HASH_SEQUENCE_START		= htobe32(0x00000186);
constexpr uint32_t CC_POLICY_PHYSICAL_PRESENCE	= htobe32(0x00000187);
constexpr uint32_t CC_POLICY_DUPLICATION_SELECT	= htobe32(0x00000188);
constexpr uint32_t CC_POLICY_GET_DIGEST			= htobe32(0x00000189);
constexpr uint32_t CC_TEST_PARAMS				= htobe32(0x0000018A);
constexpr uint32_t CC_COMMIT					= htobe32(0x0000018B);
constexpr uint32_t CC_POLICY_PASSWORD			= htobe32(0x0000018C);
constexpr uint32_t CC_ZGEN_2PHASE				= htobe32(0x0000018D);
constexpr uint32_t CC_EC_EPHEMERAL				= htobe32(0x0000018E);
constexpr uint32_t CC_POLICY_NV_WRITTEN			= htobe32(0x0000018F);
constexpr uint32_t CC_LAST						= CC_POLICY_NV_WRITTEN;

constexpr uint32_t RC_SUCCESS			= 0x0000;
constexpr uint32_t RC_BAD_TAG			= htobe32(0x001E);
constexpr uint32_t RC_VER1				= 0x0100;
constexpr uint32_t RC_INITIALIZE		= htobe32(RC_VER1);
constexpr uint32_t RC_FAILURE			= htobe32(RC_VER1 + 0x01);
constexpr uint32_t RC_SEQUENCE			= htobe32(RC_VER1 + 0x03);
constexpr uint32_t RC_PRIVATE			= htobe32(RC_VER1 + 0x0B);
constexpr uint32_t RC_HMAC				= htobe32(RC_VER1 + 0x19);
constexpr uint32_t RC_DISABLED			= htobe32(RC_VER1 + 0x20);
constexpr uint32_t RC_EXCLUSIVE			= htobe32(RC_VER1 + 0x21);
constexpr uint32_t RC_AUTH_TYPE			= htobe32(RC_VER1 + 0x24);
constexpr uint32_t RC_AUTH_MISSING		= htobe32(RC_VER1 + 0x25);
constexpr uint32_t RC_POLICY			= htobe32(RC_VER1 + 0x26);
constexpr uint32_t RC_PCR				= htobe32(RC_VER1 + 0x27);
constexpr uint32_t RC_PCR_CHANGED		= htobe32(RC_VER1 + 0x28);
constexpr uint32_t RC_UPGRADE			= htobe32(RC_VER1 + 0x2D);
constexpr uint32_t RC_TOO_MANY_CTX		= htobe32(RC_VER1 + 0x2E);
constexpr uint32_t RC_AUTH_UNAVAILABLE	= htobe32(RC_VER1 + 0x2F);
constexpr uint32_t RC_REBOOT			= htobe32(RC_VER1 + 0x30);
constexpr uint32_t RC_UNBALANCED		= htobe32(RC_VER1 + 0x31);
constexpr uint32_t RC_COMMAND_SIZE		= htobe32(RC_VER1 + 0x42);
constexpr uint32_t RC_COMMAND_CODE		= htobe32(RC_VER1 + 0x43);
constexpr uint32_t RC_AUTH_SIZE			= htobe32(RC_VER1 + 0x44);
constexpr uint32_t RC_AUTH_CONTEXT		= htobe32(RC_VER1 + 0x45);
constexpr uint32_t RC_NV_RANGE			= htobe32(RC_VER1 + 0x46);
constexpr uint32_t RC_NV_SIZE			= htobe32(RC_VER1 + 0x47);
constexpr uint32_t RC_NV_LOCKED			= htobe32(RC_VER1 + 0x48);
constexpr uint32_t RC_NV_AUTHORIZATION	= htobe32(RC_VER1 + 0x49);
constexpr uint32_t RC_NV_UNINITIALIZED	= htobe32(RC_VER1 + 0x4A);
constexpr uint32_t RC_NV_SPACE			= htobe32(RC_VER1 + 0x4B);
constexpr uint32_t RC_NV_DEFINED		= htobe32(RC_VER1 + 0x4C);
constexpr uint32_t RC_BAD_CONTEXT		= htobe32(RC_VER1 + 0x50);
constexpr uint32_t RC_CP_HASH			= htobe32(RC_VER1 + 0x51);
constexpr uint32_t RC_PARENT			= htobe32(RC_VER1 + 0x52);
constexpr uint32_t RC_NEEDS_TEST		= htobe32(RC_VER1 + 0x53);
constexpr uint32_t RC_NO_RESULT			= htobe32(RC_VER1 + 0x54);
constexpr uint32_t RC_SENSITIVE			= htobe32(RC_VER1 + 0x55);
constexpr uint32_t RC_MAX_FM0			= htobe32(RC_VER1 + 0x7F);
constexpr uint32_t RC_FMT1				= 0x0080;
constexpr uint32_t RC_ASYMMETRIC		= htobe32(RC_FMT1 + 0x01);
constexpr uint32_t RC_ATTRIBUTES		= htobe32(RC_FMT1 + 0x02);
constexpr uint32_t RC_HASH				= htobe32(RC_FMT1 + 0x03);
constexpr uint32_t RC_VALUE				= htobe32(RC_FMT1 + 0x04);
constexpr uint32_t RC_HIERARCHY			= htobe32(RC_FMT1 + 0x05);
constexpr uint32_t RC_KEY_SIZE			= htobe32(RC_FMT1 + 0x07);
constexpr uint32_t RC_MGF				= htobe32(RC_FMT1 + 0x08);
constexpr uint32_t RC_MODE				= htobe32(RC_FMT1 + 0x09);
constexpr uint32_t RC_TYPE				= htobe32(RC_FMT1 + 0x0A);
constexpr uint32_t RC_HANDLE			= htobe32(RC_FMT1 + 0x0B);
constexpr uint32_t RC_KDF				= htobe32(RC_FMT1 + 0x0C);
constexpr uint32_t RC_RANGE				= htobe32(RC_FMT1 + 0x0D);
constexpr uint32_t RC_AUTH_FAIL			= htobe32(RC_FMT1 + 0x0E);
constexpr uint32_t RC_NONCE				= htobe32(RC_FMT1 + 0x0F);
constexpr uint32_t RC_PP				= htobe32(RC_FMT1 + 0x10);
constexpr uint32_t RC_SCHEME			= htobe32(RC_FMT1 + 0x12);
constexpr uint32_t RC_SIZE				= htobe32(RC_FMT1 + 0x15);
constexpr uint32_t RC_SYMMETRIC			= htobe32(RC_FMT1 + 0x16);
constexpr uint32_t RC_TAG				= htobe32(RC_FMT1 + 0x17);
constexpr uint32_t RC_SELECTOR			= htobe32(RC_FMT1 + 0x18);
constexpr uint32_t RC_INSUFFICIENT		= htobe32(RC_FMT1 + 0x1A);
constexpr uint32_t RC_SIGNATURE			= htobe32(RC_FMT1 + 0x1B);
constexpr uint32_t RC_KEY				= htobe32(RC_FMT1 + 0x1C);
constexpr uint32_t RC_POLICY_FAIL		= htobe32(RC_FMT1 + 0x1D);
constexpr uint32_t RC_INTEGRITY			= htobe32(RC_FMT1 + 0x1F);
constexpr uint32_t RC_TICKET			= htobe32(RC_FMT1 + 0x20);
constexpr uint32_t RC_RESERVED_BITS		= htobe32(RC_FMT1 + 0x21);
constexpr uint32_t RC_BAD_AUTH			= htobe32(RC_FMT1 + 0x22);
constexpr uint32_t RC_EXPIRED			= htobe32(RC_FMT1 + 0x23);
constexpr uint32_t RC_POLICY_CC			= htobe32(RC_FMT1 + 0x24);
constexpr uint32_t RC_BINDING			= htobe32(RC_FMT1 + 0x25);
constexpr uint32_t RC_CURVE				= htobe32(RC_FMT1 + 0x26);
constexpr uint32_t RC_ECC_POINT			= htobe32(RC_FMT1 + 0x27);
constexpr uint32_t RC_WARN				= 0x900;
constexpr uint32_t RC_CONTEXT_GAP		= htobe32(RC_WARN + 0x01);
constexpr uint32_t RC_OBJECT_MEMORY		= htobe32(RC_WARN + 0x02);
constexpr uint32_t RC_SESSION_MEMORY	= htobe32(RC_WARN + 0x03);
constexpr uint32_t RC_MEMORY			= htobe32(RC_WARN + 0x04);
constexpr uint32_t RC_SESSION_HANDLES	= htobe32(RC_WARN + 0x05);
constexpr uint32_t RC_OBJECT_HANDLES	= htobe32(RC_WARN + 0x06);
constexpr uint32_t RC_LOCALITY			= htobe32(RC_WARN + 0x07);
constexpr uint32_t RC_YIELDED			= htobe32(RC_WARN + 0x08);
constexpr uint32_t RC_CANCELED			= htobe32(RC_WARN + 0x09);
constexpr uint32_t RC_TESTING			= htobe32(RC_WARN + 0x0A);
constexpr uint32_t RC_REFERENCE_H0		= htobe32(RC_WARN + 0x10);
constexpr uint32_t RC_REFERENCE_H1		= htobe32(RC_WARN + 0x11);
constexpr uint32_t RC_REFERENCE_H2		= htobe32(RC_WARN + 0x12);
constexpr uint32_t RC_REFERENCE_H3		= htobe32(RC_WARN + 0x13);
constexpr uint32_t RC_REFERENCE_H4		= htobe32(RC_WARN + 0x14);
constexpr uint32_t RC_REFERENCE_H5		= htobe32(RC_WARN + 0x15);
constexpr uint32_t RC_REFERENCE_H6		= htobe32(RC_WARN + 0x16);
constexpr uint32_t RC_REFERENCE_S0		= htobe32(RC_WARN + 0x18);
constexpr uint32_t RC_REFERENCE_S1		= htobe32(RC_WARN + 0x19);
constexpr uint32_t RC_REFERENCE_S2		= htobe32(RC_WARN + 0x1A);
constexpr uint32_t RC_REFERENCE_S3		= htobe32(RC_WARN + 0x1B);
constexpr uint32_t RC_REFERENCE_S4		= htobe32(RC_WARN + 0x1C);
constexpr uint32_t RC_REFERENCE_S5		= htobe32(RC_WARN + 0x1D);
constexpr uint32_t RC_REFERENCE_S6		= htobe32(RC_WARN + 0x1E);
constexpr uint32_t RC_NV_RATE			= htobe32(RC_WARN + 0x20);
constexpr uint32_t RC_LOCKOUT			= htobe32(RC_WARN + 0x21);
constexpr uint32_t RC_RETRY				= htobe32(RC_WARN + 0x22);
constexpr uint32_t RC_NV_UNAVAILABLE	= htobe32(RC_WARN + 0x23);

constexpr uint16_t SU_CLEAR	= 0x0000;
constexpr uint16_t SU_STATE	= htobe16(0x0001);


typedef struct PacketHeader {
	uint16_t tag;	/* TAG_ */
	uint32_t size;
	uint32_t code;	/* CC_ or RC_	*/
} __attribute__((packed)) PacketHeader;

typedef struct GenericPacket {
	PacketHeader header;
	uint8_t data[];
}  __attribute__((packed)) GenericPacket;

typedef struct StartStopCommand {
	PacketHeader header;
	uint16_t startupType;	/* SU_	*/
} __attribute__((packed)) StartStopCommand;

inline StartStopCommand makeStartCommand(uint16_t startupType) {
	return {
		{ TAG_NO_SESSIONS, htobe32(12), CC_STARTUP },
		htobe16(startupType)
	};
}

inline StartStopCommand makeShutdownCommand(uint16_t shutdownType) {
	return {
		{ TAG_NO_SESSIONS, htobe32(12), CC_SHUTDOWN },
		htobe16(shutdownType)
	};
}

}