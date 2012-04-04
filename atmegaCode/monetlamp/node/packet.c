#include <avr/io.h>
#include <avr/signal.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <inttypes.h>

//i2c state machine
//http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=102492&start=0

#include "monetlamp.h"
#include "packet.h"
#include "uart485.h"

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

#define POLY_CRC08	0x18
static uint8_t crcByte;
void crc8Init(void);
void crc8AddByte( uint8_t value );

void sendPacket()
{
	uint8_t i;
	crc8Init();
	for( i = 0; i < ( sizeof( tPacketPayload ) - 1 ); i++ )
	{
		crc8AddByte( thePacket.dataBytes[ i ] );
	}
	thePacket.pdata.crc8 = crcByte;
	uart_485_tx_buffer( thePacket.dataBytes, sizeof( tPacketPayload ) );
}

void readPacket()
{
	uart_485_rx_buffer( thePacket.dataBytes, sizeof( tPacketPayload ) );
}

uint8_t isPacketValid()
{
	uint8_t i;
	crc8Init();
	for( i = 0; i < ( sizeof( tPacketPayload ) - 1 ); i++ )
	{
		crc8AddByte( thePacket.dataBytes[ i ] );
	}

	if( thePacket.pdata.crc8 == crcByte )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

uint8_t isPacketMulticast()
{
	if( thePacket.pdata.recvAddress == UART485_MULTICAST_ADDRESS )
	{
		return TRUE;
	}
	return FALSE;
}

void crc8Init(void)
{
	crcByte = 0;
}

void crc8AddByte( uint8_t value )
{
	uint8_t i, feedback_bit;

	for ( i=0; i<8; i++ ) 
	{
		feedback_bit = (crcByte ^ value) & 0x01;
		if (feedback_bit != 0) {
			crcByte ^= POLY_CRC08;
		}
		crcByte = (crcByte >> 1) & 0x7F;
		if (feedback_bit != 0) {
			crcByte |= 0x80;
		}

		value >>= 1;
	};	
}
