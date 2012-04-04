#include <avr/io.h>
#include <avr/signal.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <inttypes.h>

//i2c state machine
//http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=102492&start=0

#include "snap2.h"

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 1
#endif

/*
typedef tSnapAddress;
typedef tSnapProtocolFlagBytes;
typedef tSnapEDM;
*/


#if (SNAP_NDB_SIZE >= SNAP_NDB_0) && (SNAP_NDB_SIZE<=SNAP_NDB_8)
	#define SNAP_DATA_BYTES SNAP_NDB_SIZE
#elif( SNAP_NDB_SIZE == SNAP_NDB_16 )
	#define SNAP_DATA_BYTES 16
#elif( SNAP_NDB_SIZE == SNAP_NDB_32 )
	#define SNAP_DATA_BYTES 32
#elif( SNAP_NDB_SIZE == SNAP_NDB_64 )
	#define SNAP_DATA_BYTES 64
#elif( SNAP_NDB_SIZE == SNAP_NDB_128 )
	#define SNAP_DATA_BYTES 128
#elif( SNAP_NDB_SIZE == SNAP_NDB_256 )
	#define SNAP_DATA_BYTES 256
#elif( SNAP_NDB_SIZE == SNAP_NDB_512 )
	#define SNAP_DATA_BYTES 512
#else
	#define SNAP_DATA_BYTES 1
#endif

#define SNAP_MAX_ADDRESS_BYTES 4

#define SNAP_MAX_EDM_BYTES 4

enum
{
	EDM_NO_CORRECTION = SNAP_EDM_NONE,
	EDM_RETRANSMIT = SNAP_EDM_RETRANSMIT,
	EDM_8BIT_CHECKSUM = SNAP_EDM_CHECKSUM_8,
	EDM_8BIT_CRC = SNAP_EDM_CRC_8,
	EDM_16BIT_CRC = SNAP_EDM_CRC_16,
	EDM_32BIT_CRC = SNAP_EDM_CRC_32,
	EDM_FEC  = SNAP_EDM_FEC,
	EDM_USER = SNAP_EDM_USER
} snap_edm;

uint8_t edm_length_table[] PROGMEM = {0,0,1,1,2,4,0,0};
	
#define getEDMByteLength( edm ) (pgm_read_byte(edm_length_table+edm))
#define skipEDMBytes(edm) ( (sizeof(tSnapEDM) - getEDMByteLength(edm) )

#define getPFBByteLength() (0)
#define skipPFBBytes(edm) ( (sizeof(tSnapProtocolFlagBytes) - getPFBByteLength() )

enum
{
	NDB_0 = SNAP_NDB_0,
	NDB_1,
	NDB_2,
	NDB_3,
	NDB_4,
	NDB_5,
	NDB_6,
	NDB_7,
	NDB_8,
	NDB_16,
	NDB_32,
	NDB_64,
	NDB_128,
	NDB_256,
	NDB_512,
	NDB_USER	
} snap_ndb;

enum
{
	CMD_NONE = 0,
	CMD = 1
} snap_cmd;

enum
{
	ACK_NO = SNAP_REQU_NO_ACK,
	ACK_REQUEST = SNAP_REQU_ACK,
	ACK_REPLY = SNAP_REPLY_ACK,
	NACK_REPLY = SNAP_REPLY_NACK
} snap_ack;

typedef union
{
	uint8_t		sa8;
	uint16_t	sa16;
	uint32_t	sa32;	
	uint8_t		saa[ SNAP_MAX_ADDRESS_BYTES ];
} tSnapAddress;

typedef union
{
	struct 
	{
		uint8_t pfb3;
		uint8_t pfb2;
		uint8_t pfb1;
	};
	uint8_t pfbytes[ ];
} tSnapProtocolFlagBytes;

typedef union
{
	uint8_t		checksum;
	uint8_t		crc08;
	uint16_t	crc16;
	uint32_t	crc32;
	uint8_t		arr[ SNAP_MAX_EDM_BYTES ];
} tSnapEDM;

typedef struct _snap_packet
{
	union
	{
		struct
		{
			uint8_t ack: 2;
			uint8_t pfb: 2;
			uint8_t sab: 2;
			uint8_t dab: 2;
		};

		uint8_t hdb2byte;
	} hdb2;
	
	union
	{
		struct
		{
			uint8_t ndb: 4;
			uint8_t edm: 3;
			uint8_t c: 1;
		};

		uint8_t hdb1byte;
	} hdb1;
	
	tSnapAddress	da;
	
	tSnapAddress	sa;

	tSnapProtocolFlagBytes pfb;

	uint8_t			dataBytes[ SNAP_DATA_BYTES ];

	tSnapEDM		edm;
} tSnapPacket;
//__attribute__((aligned(sizeof(word)))); 

static struct
{
	union
	{
		tSnapPacket		snapPacket;
		uint8_t			snapPacketArray[ sizeof( tSnapPacket ) + 2 ];
	} packet;
	
	tSnapEDM calculatededm;
	
	//index in the array, will be used to skip depending on the headers
	uint8_t	snapPacketIdx;
	
	//
	uint8_t snapPacketReadState;
	
	//reading or writing
	uint8_t snapPacketState;
	
	//
	uint8_t snapPacketTimeout;
} snapP;

#define SNAP_BUFFER_LENGTH	32
#define SNAP_BUFFER_MASK ( SNAP_BUFFER_LENGTH-1 )

static struct
{
	enum
	{
		SNAP_IDLE,
		SNAP_RECEINVING,
		SNAP_RECEIVE_SUCCESS,
		SNAP_RECEIVE_CORRECTED,
		SNAP_RECEIVE_FAIL_EDM,
		SNAP_RECEIVE_FAIL_INVALID,
		SNAP_RECEIVE_FAIL_ABORT,
		SNAP_TRANSMITING,
		SNAP_TRANSMIT_SUCCESS,
		SNAP_TRANSMIT_FAIL
	} status;
	
	enum
	{
		
	} state;
	
	tSnapAddress snapAddress;
	
	struct
	{
		uint8_t head;
		uint8_t tail;
		uint8_t length;
		uint8_t buf[ SNAP_BUFFER_LENGTH ];	
	} buffer;
} snap_processor;


#define snapReceiveSuccess() ( snap_processor.status = SNAP_RECEIVE_SUCCESS || snap_processor.status = SNAP_RECEIVE_CORRECTED )
#define snapReceiveFail() ( snap_processor.status = SNAP_RECEIVE_FAIL_EDM || snap_processor.status = SNAP_RECEIVE_FAIL_INVALID || snap_processor.status = SNAP_RECEIVE_FAIL_ABORT )

#define snapBufferIsEmpty() (snap_processor.buffer.length==0)
#define snapBufferHasSpace() ( snap_processor.buffer.length <= SNAP_BUFFER_LENGTH )
#define snapBufferHasData() ( snap_processor.buffer.length > 0 )

uint8_t snapCharReceive( uint8_t chr, uint8_t err )
{
	if( snapBufferHasSpace() )
	{
		snap_processor.buffer.buf[ snap_processor.buffer.tail ] = chr;
		snap_processor.buffer.tail++;
		snap_processor.buffer.tail &= SNAP_BUFFER_MASK;
		return TRUE;	
	}
	return FALSE;
}

uint8_t processSnapBuffer()
{
	if( snapBufferHasData() )		
	{
		
	}
	return FALSE;
}

/*
 *
 *
 */

/***
 * EDM functions.
 */
#if defined (SNAP_EDM_CRC_32)
#	define SNAP_MAX_EDM_BYTES	4
#elif defined (SNAP_EDM_CRC_16)
#	define SNAP_MAX_EDM_BYTES	2
#elif defined (SNAP_EDM_CRC_8)
#	define SNAP_MAX_EDM_BYTES	1
#elif defined (SNAP_EDM_CHECKSUM_8)
#	define SNAP_MAX_EDM_BYTES	1
#else
#	define SNAP_MAX_EDM_BYTES	0
#endif

#if defined(SNAP_EDM_CHECKSUM_8) || defined(SNAP_EDM_CRC_8) ||   \
	defined(SNAP_EDM_CRC_16) || defined(SNAP_EDM_CRC_32) ||  \
	defined(SNAP_EDM_FEC)
	
	#define SNAP_EDM_ANY 1
#endif

typedef union {
#ifdef SNAP_EDM_CHECKSUM_8
	uint8_t checksum;
#endif
#ifdef SNAP_EDM_CRC_8
	uint8_t crc08;
#endif
#ifdef SNAP_EDM_CRC_16
	uint16_t crc16;
#endif
#ifdef SNAP_EDM_CRC_32
	u32 crc32;
#endif
	uint8_t crc_raw[SNAP_MAX_EDM_BYTES];
} edm_stat_u;


#ifdef SNAP_EDM_ANY
	static edm_stat_u edm_stat;
#endif

// crc polynomials
#define POLY_CRC08	0x18
#define POLY_CRC16	0x1021
#define POLY_CRC32	0xedb88320	/* original 0x04c11db7, but reflected for algorithm */


static void inline edm_init( void )
{
	uint8_t i;
	for( i = 0; i < SNAP_MAX_EDM_BYTES; i++ )
	{
		#if SNAP_EDM_TYPE==SNAP_EDM_CRC_32
			snapP.snapPacket.edm.arr[i] = 0xff;
		#else
			snapP.snapPacket.edm.arr[i] = 0x00;
		#endif
	}
}

static void edm_add( uint8_t value )
{
	#if SNAP_EDM_TYPE==SNAP_EDM_CHECKSUM_8
	#elif SNAP_EDM_TYPE==SNAP_EDM_CRC_8
	#elif SNAP_EDM_TYPE==SNAP_EDM_CRC_16
	#elif SNAP_EDM_TYPE==SNAP_EDM_CRC_32
	#elif SNAP_EDM_TYPE==SNAP_EDM_CRC_32
	#else
	#endif	
}

#ifdef SNAP_EDM_CHECKSUM_8
static void inline edm_checksum_8_init(void)
{
	edm_stat.checksum = 0;
}
static void inline edm_checksum_8_add(uint8_t value)
{
	edm_stat.checksum += value;
}
static uint8_t inline edm_checksum_8_get(void)
{
	return edm_stat.checksum;
}
#endif

#ifdef SNAP_EDM_CRC_8
/***
 * CRC8 routines taken from Colin O'Flynn, Copyright (c) 2002.
 * The original code can be foud at www.avrfreaks.net as 
 * project 104: "Error Detection Method routines".
 */
static void inline edm_crc_8_init(void)
{
	edm_stat.crc08 = 0x00;
}
static void edm_crc_8_add(uint8_t value)
{
	uint8_t i, feedback_bit;

	for (i=0; i<8; i++) {
		feedback_bit = (edm_stat.crc08 ^ value) & 0x01;
		if (feedback_bit != 0) {
			edm_stat.crc08 ^= POLY_CRC08;
		}
		edm_stat.crc08 = (edm_stat.crc08 >> 1) & 0x7F;
		if (feedback_bit != 0) {
			edm_stat.crc08 |= 0x80;
		}

		value >>= 1;
      	};
}
static uint8_t inline edm_crc_8_get(void)
{
	return edm_stat.crc08;
}
#endif

#ifdef SNAP_EDM_CRC_16
static void inline edm_crc_16_init(void)
{
	edm_stat.crc16 = 0x0000;
}
static void edm_crc_16_add(uint8_t value)
{
	uint8_t i;
	
	edm_stat.crc16 ^= ((uint16_t)value << 8);
	for (i=0; i<8; i++) {
		if (edm_stat.crc16 & 0x8000)
			edm_stat.crc16 = (edm_stat.crc16 << 1) ^ POLY_CRC16;
		else
			edm_stat.crc16 <<= 1;
	};
}
static uint16_t inline edm_crc_16_get(void)
{
	return edm_stat.crc16;
}
#endif

#ifdef SNAP_EDM_CRC_32
static void inline edm_crc_32_init(void)
{
	edm_stat.crc32 = 0xffffffff;
}
static void edm_crc_32_add(uint8_t value)
{
	uint8_t i;
	
	edm_stat.crc32 ^= value;
	for (i=0; i<8; i++) {
		if (edm_stat.crc32 & 0x01)
			edm_stat.crc32 = (edm_stat.crc32 >> 1) ^ POLY_CRC32;
		else
			edm_stat.crc32 >>= 1;
	};
}
static u32 inline edm_crc_32_get(void)
{
	return ~edm_stat.crc32;
}
#endif

/**
 * Init error detection, detect type from header.
 * Returns TRUE if successful, otherwise FALSE.
 */
static int8_t edm_init(uint8_t hdb1)
{
	switch (hdb1 & EDM_MASK) {
		case SNAP_SEND_EDM_NONE:
			return 1;
#ifdef SNAP_EDM_CHECKSUM_8
		case SNAP_SEND_EDM_CHECKSUM_8:
			edm_checksum_8_init();
			return 1;
#endif
#ifdef SNAP_EDM_CRC_8
		case SNAP_SEND_EDM_CRC_8:
			edm_crc_8_init();
			return 1;
#endif
#ifdef SNAP_EDM_CRC_16
		case SNAP_SEND_EDM_CRC_16:
			edm_crc_16_init();
			return 1;
#endif
#ifdef SNAP_EDM_CRC_32
		case SNAP_SEND_EDM_CRC_32:
			edm_crc_32_init();
			return 1;
#endif
		default:
			return 0;
	};
	return 0;
}

/**
 * Add a byte to the active checksum.
 */
static void edm_process(uint8_t hdb1, uint8_t value)
{
	switch (hdb1 & EDM_MASK) {
#ifdef SNAP_EDM_CHECKSUM_8
		case SNAP_SEND_EDM_CHECKSUM_8:
			edm_checksum_8_add(value);
			break;
#endif
#ifdef SNAP_EDM_CRC_8
		case SNAP_SEND_EDM_CRC_8:
			edm_crc_8_add(value);
			break;
#endif
#ifdef SNAP_EDM_CRC_16
		case SNAP_SEND_EDM_CRC_16:
			edm_crc_16_add(value);
			break;
#endif
#ifdef SNAP_EDM_CRC_32
		case SNAP_SEND_EDM_CRC_32:
			edm_crc_32_add(value);
			break;
#endif
	};
}