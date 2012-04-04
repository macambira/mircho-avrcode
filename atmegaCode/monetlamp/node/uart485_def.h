#ifndef __UART_485_DEF_H
#define __UART_485_DEF_H

#include "util.h"
#include "monetlamp.h"

#if defined(ATMEGA8)
	#define INIT_SEND_PIN()			(ddrd.dd2 = 1)
	#define SEND_PIN_SET_RECEIVE()	(portd.port2 = 0)
	#define SEND_PIN_SET_TRANSMIT()	(portd.port2 = 1)

	#define DATA_REGISTER		UDR
	#define BAUD_RATE_REGISTER	UBRRL
	#define SET_ADDRESS_BIT()		(UCSRB |= _BV( TXB8 ))
	#define CLEAR_ADDRESS_BIT()		(UCSRB &= ~_BV( TXB8 ))

	
	#define WAIT_FOR_TX_QUEUE_EMPTY()	while(!(UCSRA & (1<<TXC))){}
	#define SET_MPCM()			(UCSRA |= _BV( MPCM ))
	#define CLEAR_MPCM()			(UCSRA &= ~_BV( MPCM ))
	#define MPCM_SET()			(UCSRA & _BV( MPCM ))
	#define ADDRESS_BIT_RECV_SET()		(UCSRB & _BV(RXB8))
	#define NO_RX_ERROR()		((UCSRA & ( _BV( FE ) | _BV( DOR ) )) == 0)
	
	#define INIT_REGISTERS()	do{\
			UCSRB |= _BV( UCSZ2 ); \
			UCSRC |= _BV( URSEL ) | _BV( UCSZ1 ) | _BV( UCSZ0 ); \
			UCSRB |= _BV( RXCIE ); \
			UCSRB |= _BV( TXCIE ); \
			UCSRB |= _BV( RXEN ); \
			} while(0)

	#define SEND_START_SETUP()\
		do{\
			UCSRB		&= ~_BV( RXEN );\	
			UCSRB		|= _BV( TXEN );\
			UCSRA		|= _BV( TXC );\
		} while(0)

	#define SEND_END_SETUP()\
		do{\
			UCSRB		&= ~_BV( TXEN );\
			UCSRB		|= _BV( RXEN );\
		} while(0)
		
	#define DISABLE_TXIE()\
		do{\
			UCSRB &= ~_BV( TXCIE );\
		} while(0)
 
	#define ENABLE_TXIE()\
		do{\
			UCSRB |= _BV( TXCIE );\
		} while(0)

#elif defined(ATMEGA328)
	#define INIT_SEND_PIN()			(ddrd.dd7 = 1)
	#define SEND_PIN_SET_RECEIVE()	(portd.port7 = 0)
	#define SEND_PIN_SET_TRANSMIT()	(portd.port7 = 1)
	
	#define DATA_REGISTER		UDR0
	#define BAUD_RATE_REGISTER	UBRR0
	#define SET_ADDRESS_BIT()		(UCSR0B |= _BV( TXB80 ))
	#define CLEAR_ADDRESS_BIT()		(UCSR0B &= ~_BV( TXB80 ))
	
	#define WAIT_FOR_TX_QUEUE_EMPTY()	while(!(UCSR0A & (1<<TXC0))){}
	#define SET_MPCM()			(UCSR0A |= _BV( MPCM0 ))
	#define CLEAR_MPCM()			(UCSR0A &= ~_BV( MPCM0 ))
	#define MPCM_SET()			(UCSR0A & _BV( MPCM0 ))
	#define ADDRESS_BIT_RECV_SET()		(UCSR0B & _BV(RXB80))
	#define NO_RX_ERROR()			((UCSR0A & ( _BV( FE0 ) | _BV( DOR0 ) )) == 0) 
	
	#define INIT_REGISTERS()\
		do{\
			UCSR0B |= _BV( UCSZ02 ); \
			UCSR0C |= _BV( UCSZ01 ) | _BV( UCSZ00 ); \
			UCSR0B |= _BV( RXCIE0 ); \
			UCSR0B |= _BV( TXCIE0 ); \
			UCSR0B |= _BV( RXEN0 ); \
		} while(0)

	#define SEND_START_SETUP()\
		do{\
			UCSR0B		&= ~_BV( RXEN0 ); \	
			UCSR0B		|= _BV( TXEN0 ); \
			UCSR0A		|= _BV( TXC0 ); \
		} while(0)

	#define SEND_END_SETUP()\
		do{\
			UCSR0B		&= ~_BV( TXEN0 );\
			UCSR0B		|= _BV( RXEN0 );\
		} while(0)
		
	#define DISABLE_TXIE()\
		do{\
			UCSR0B &= ~_BV( TXCIE0 );\
		} while(0)
 
	#define ENABLE_TXIE()\
		do{\
			UCSR0B |= _BV( TXCIE0 );\
		} while(0)
		
#else

#error "NO UART DEFINITIONS"

#endif

#define READ_DATA_REGISTER()		(DATA_REGISTER)
#define WRITE_DATA_REGISTER(v)		(DATA_REGISTER=v)
#define SET_BAUD_RATE(v)		(BAUD_RATE_REGISTER=v)

/***
 * Specify your desired baud rate and current master clock frequency.
 * Note that the actual baud rate may differ. (see manual)
 * Also keep in mind that high rates may not be feasible due to edm and
 * protocol processing time and other interrupts.
 */
#define UART485_F_CLK		F_CPU		/* your clock frequency */

#endif
