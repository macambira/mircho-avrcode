#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <inttypes.h>
#include <util/delay.h>

#include "uart485.h"
#include "spilcd.h"
#include "led.h"

#define UART_BUFFER_SIZE 16	//should be power of 2, other routines are built on that
#define UART_BUFFER_MASK (UART_BUFFER_SIZE-1)

#define ROLLOVER( c ) (c &= UART_BUFFER_MASK)

#define BUFFER_FULL( which ) ((which).uartBufferLen==UART_BUFFER_SIZE)
#define BUFFER_EMPTY( which ) ((which).uartBufferLen==0)

typedef struct _buffer  {
	uint8_t uartBufferHead;
	uint8_t uartBufferTail;
	uint8_t uartBufferLen;
	uint8_t buffer[ UART_BUFFER_SIZE ];	
} UARTBUFFER;

volatile UARTBUFFER	rxBuffer;
volatile UARTBUFFER	txBuffer;

static uint8_t		uart485Address = 0x00;

inline void addToRXBuffer( uint8_t data );
inline void addToTXBuffer( uint8_t data );

inline uint8_t getFromRXBuffer();
inline uint8_t getFromTXBuffer();

void resetBuffer( tBufferType which );

void uart_485_send_start(void);
void uart_485_send_done(void);


#if defined(ATMEGA8)
ISR( USART_RXC_vect )
#elif defined(ATMEGA328)
ISR( USART_RX_vect)
#endif
{
	uint8_t value, addressBitSet;
	addressBitSet = ADDRESS_BIT_RECV_SET(); 
	value = READ_DATA_REGISTER();
	
	uint8_t strBuffer[8];
	//PRINT_TO_LCD_AT( 0, 0, "     " );
	//PRINT_TO_LCD_AT( 0, 0, itoa( value, strBuffer, 10 ) );
	//PRINT_TO_LCD_AT( 1, 0, itoa( UART485_EOP, strBuffer, 10 ) );
	
	if( NO_RX_ERROR() )
	{
		//we are sent an address byte
		if( addressBitSet )
		{
			if( ( ( value & UART485_ADDRESS_MASK ) == uart485Address ) || ( value == UART485_MULTICAST_ADDRESS ) )
			{
				CLEAR_MPCM();
				uartStatus.state = RECEIVING;
			}
			else if( value == UART485_EOP )
			{
				SET_MPCM();
				if( uartStatus.state == RECEIVING )
				{
					uart_485_notify( rxBuffer.uartBufferLen );
					uartStatus.state = IDLE;
					return;
				}
			}
		}
		//data byte
		addToRXBuffer( value );
	}
	else
	{
		SET_MPCM();
		uartStatus.state = IDLE;
	}
}

#if defined(ATMEGA8)
ISR( USART_TXC_vect )
#elif defined(ATMEGA328)
ISR( USART_TX_vect)
#endif
{
	uint8_t value;

	if( BUFFER_EMPTY( txBuffer ) )
	{
		uart_485_send_done();
	}
	else
	{
		value = getFromTXBuffer();
	
		if( BUFFER_EMPTY( txBuffer ) )
		{
			SET_ADDRESS_BIT();
		}
		else
		{
			CLEAR_ADDRESS_BIT();	
		}
		WRITE_DATA_REGISTER( value );
	}
}

void uart_485_init( uint8_t address )
{
 	// set baud rate
	cli();
	SET_BAUD_RATE(BAUD_VALUE);
	INIT_REGISTERS();
	sei();

	INIT_SEND_PIN();
	
	SEND_PIN_SET_RECEIVE();
	
	SET_MPCM();

	uart_485_set_address( address );
	
	uart_485_notify = NULL_CALLBACK;

	uartStatus.state = IDLE;
}

void uart_485_send_start(void)
{
	uint8_t value;

	LEDON;

	uart_485_clear_error_status();

	SEND_START_SETUP();
	SEND_PIN_SET_TRANSMIT();
	
	if( BUFFER_EMPTY( txBuffer ) || BUFFER_FULL( txBuffer ) )
	{
		return;
	}
	else
	{
		uartStatus.state = TRANSMITTING;
		
		//set the ninth bit to one - an address
		addToTXBuffer( UART485_EOP );
		value = getFromTXBuffer();
		SET_ADDRESS_BIT();
		WRITE_DATA_REGISTER( value );

		//wait to have the data in the transmit location, maybe an interrupt occurs somewhere here
		//WAIT_FOR_TX_QUEUE_EMPTY();
		//clear the ninth bit, from now on we are only sending data
		//CLEAR_ADDRESS_BIT();
	}
}

void uart_485_send_done(void)
{
	SEND_END_SETUP();	
	SEND_PIN_SET_RECEIVE();
	
	LEDOFF;

	uartStatus.state = IDLE;
}

void uart_485_tx_buffer( uint8_t *buf, uint8_t size )
{
	uint8_t counter = 0;
	
	resetBuffer( TX_BUFFER );
	while( uartStatus.errorStatus != TX_BUFFER_OVERRUN && counter < size )
	{
		addToTXBuffer( buf[ counter++ ] );
	}
	
	if( uartStatus.errorStatus == TX_BUFFER_OVERRUN )
	{
		return;
	}
	else
	{
		uart_485_send_start();
	}
}

uint8_t uart_485_rx_buffer( uint8_t *buf, uint8_t size )
{
	uint8_t counter = 0;
	while( uartStatus.errorStatus != RX_BUFFER_EMPTY && counter < size )
	{
		buf[ counter++ ] = getFromRXBuffer();
		//memcpy( buf, rxBuffer.buffer, size );
	}
	resetBuffer( RX_BUFFER );
	return counter;
}

void uart_485_clear_error_status( void )
{
	uartStatus.errorStatus = NO_ERROR;
}

inline void addToRXBuffer( uint8_t data )
{
	if( BUFFER_FULL( rxBuffer ) )
	{
		uartStatus.errorStatus = RX_BUFFER_OVERRUN;
		return;
	}
	else
	{
		rxBuffer.buffer[ rxBuffer.uartBufferTail ] = data;
		rxBuffer.uartBufferTail++;
		rxBuffer.uartBufferTail &= UART_BUFFER_MASK;
		rxBuffer.uartBufferLen++;		
	}		
}

inline void addToTXBuffer( uint8_t data )
{
	if( BUFFER_FULL( txBuffer ) )
	{
		uartStatus.errorStatus = TX_BUFFER_OVERRUN;
		return;
	}
	else
	{
		txBuffer.buffer[ txBuffer.uartBufferTail ] = data;
		txBuffer.uartBufferTail++;
		txBuffer.uartBufferTail &= UART_BUFFER_MASK;
		txBuffer.uartBufferLen++;		
	}		
}

inline uint8_t getFromRXBuffer()
{
	uint8_t result;
	if( BUFFER_EMPTY( rxBuffer ) )
	{
		uartStatus.errorStatus = RX_BUFFER_EMPTY;
		return 0xf0;
	}
	else
	{
		result = rxBuffer.buffer[ rxBuffer.uartBufferHead ];
		rxBuffer.uartBufferHead++;
		rxBuffer.uartBufferHead &= UART_BUFFER_MASK;
		ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
		{
			rxBuffer.uartBufferLen--;
		}
		return result;
	}		
}

inline uint8_t getFromTXBuffer()
{
	uint8_t result;
	if( BUFFER_EMPTY( txBuffer ) )
	{
		uartStatus.errorStatus = TX_BUFFER_EMPTY;
		return 0xf0;
	}
	else
	{
		result = txBuffer.buffer[ txBuffer.uartBufferHead ];
		txBuffer.uartBufferHead++;
		txBuffer.uartBufferHead &= UART_BUFFER_MASK;
		txBuffer.uartBufferLen--;
		return result;
	}		
}

void resetBuffer( tBufferType which )
{
	UARTBUFFER *buf;
	if( which == RX_BUFFER )
	{
		buf = &rxBuffer;
	}
	else
	{
		buf = &txBuffer;
	}

	buf->uartBufferHead = buf->uartBufferTail = 0;
	buf->uartBufferLen = 0;
}

void uart_485_set_address( uint8_t address )
{
	uart485Address = address;
}

uint8_t uart_485_get_address( void )
{
	return uart485Address;
}

uint8_t uart_485_rx_char_available( void )
{
	return rxBuffer.uartBufferLen;
}
