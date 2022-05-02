#pragma once

#include <cstdint>
#include <byteswap.h>
#include <endian.h>

namespace tpm {

constexpr uint16_t TAG_NO_SESSIONS 	= be16toh(0x8001); 
constexpr uint16_t TAG_SESSIONS 	= be16toh(0x8002);

constexpr uint32_t CC_FIRST		= be32toh(0x0000011F);
constexpr uint32_t CC_STARTUP 	= be32toh(0x00000145);

constexpr uint16_t SU_CLEAR	= 0x0000;
constexpr uint16_t SU_STATE	= be16toh(0x0001);

constexpr uint16_t RC_SUCCESS		= 0x0000;
constexpr uint16_t RC_BAD_TAG		= be16toh(0x001E);
constexpr uint16_t RC_VER1			= be16toh(0x0100);
constexpr uint16_t RC_INITIALIZE	= be16toh(RC_VER1 + 0x0000u);
constexpr uint16_t RC_FAILURE		= be16toh(RC_VER1 + 0x0001u);


typedef struct PacketHeader {
	uint16_t tag;			/* TAG_ */
	uint32_t size;
	uint32_t commandCode;	/* CC_ or RC_	*/
} __attribute__((packed)) PacketHeader;

typedef struct StartCommand {
	PacketHeader header;
	uint16_t startupType;	/* SU_	*/
} __attribute__((packed)) StartCommand;

inline StartCommand makeStartCommand(uint16_t startupType) {
	return {
		{ TAG_NO_SESSIONS, 12, CC_STARTUP },
		startupType
	};
}

}