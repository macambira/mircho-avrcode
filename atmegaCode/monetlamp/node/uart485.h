#ifndef __SNAP_LNK_H
#define __SNAP_LNK_H

#include <inttypes.h>
#include "uart485_def.h"

#define UART485_BAUD_RATE	9600		/* desired baud rate    */
#define BAUD_VALUE			(UART485_F_CLK/(UART485_BAUD_RATE*16l) - 1) /* value to load into UDR */
#define UART485_EOP			0xFA		/* end of packet char */
#define UART485_ADDRESS_MASK 0b01111111	/* address mask */
#define UART485_MULTICAST_ADDRESS	0b11111111 /* 0xff */



typedef void (*UART_CALLBACK_FUNCTION)( uint8_t size );
#define NULL_CALLBACK	((UART_CALLBACK_FUNCTION)0x00)

UART_CALLBACK_FUNCTION uart_485_notify;

struct _uart_status 
{
	enum
	{
		IDLE,
		TRANSMITTING,
		RECEIVING
	} state;
	enum
	{
		NO_ERROR,
		RX_FRAMING_ERROR,
		RX_OVERRUN_ERROR,
		RX_BUFFER_EMPTY,
		RX_BUFFER_OVERRUN,
		TX_BUFFER_EMPTY,
		TX_BUFFER_OVERRUN		
	} errorStatus;
} uartStatus;


typedef enum _bufferType
{
	RX_BUFFER,
	TX_BUFFER
} tBufferType;

void uart_485_init( uint8_t address );

void uart_485_set_address( uint8_t address );
uint8_t uart_485_get_address( void );

inline void uart_485_set_as_receiver(void) { SEND_PIN_SET_RECEIVE(); }
inline void uart_485_set_as_transmitter(void) { SEND_PIN_SET_TRANSMIT(); }

//transmi a string; put it into the transmit buffer
void uart_485_tx_buffer(uint8_t *buf, uint8_t size);

//how many chars are there in the receive buffer
uint8_t uart_485_rx_char_available( void );
//directly copy several bytes in a buffer, e.g. receive the packet
uint8_t uart_485_rx_buffer( uint8_t *buf, uint8_t size );

void uart_485_clear_error_status( void );

#endif
