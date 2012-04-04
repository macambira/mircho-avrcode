/*
 * spi.c
 *
 * Buffered SPI master for ATMega8 to send messages to a SPI LCD screen
 *
 * Created: 28.3.2011 г. 11:57:03
 *  Author: mmirev
 */ 

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "spi.h"

#define SPI_BUFFER_SIZE 64	//should be power of 2, other routines are built on that
#define SPI_BUFFER_MASK (SPI_BUFFER_SIZE-1)

volatile struct _spibuffer {
	uint8_t sending;
	uint8_t spiBufferHead;
	uint8_t spiBufferTail;
	uint8_t spiBufferLen;
	uint8_t spibuffer[ SPI_BUFFER_SIZE ];	
} spibuffer = {0};


void spi_init( void )
{
	//DDRB = _BV(PB3) | _BV(PB4) | _BV(PB5);					// Set MOSI, SCK, SS as output

	//configure SS pin as input with pullup, the default for a single master
	DDRB |= _BV(PB3) | _BV(PB5) | _BV(PB2);					// Set MOSI, SCK as output
	//DDRB &= ~_BV(PB2);
	//PORTB |= _BV(PB2);
	
	SPSR &= ~_BV(SPI2X);
	SPCR = _BV(SPIE) | _BV(SPE) | _BV(MSTR) | _BV(SPR1) | _BV(SPR0);		// Enable SPI, Master, set clock rate fck/128
	//SPCR = _BV(SPIE) | _BV(SPE) | _BV(MSTR) | _BV(SPR0);		// Enable SPI, Master, set clock rate fck/16
	
	//SPSR;
	//SPDR;
}

void spi_write( uint8_t byte )
{
	SPDR = byte;					//Load byte to Data register
	while(!(SPSR & (1<<SPIF))); 	// Wait for transmission complete
	SPDR;
}

void spi_putc( uint8_t chr )
{
	if( spibuffer.spiBufferLen <= SPI_BUFFER_SIZE )
	{
		spibuffer.spibuffer[ spibuffer.spiBufferTail ] = chr;
		spibuffer.spiBufferTail++;
		spibuffer.spiBufferTail &= SPI_BUFFER_MASK;
		ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
		{
			spibuffer.spiBufferLen++;
		}
	}
	if( !spibuffer.sending )
	{
		spi_send();
	}
}

void spi_send( void )
{
	uint8_t c;
	if( spibuffer.spiBufferLen > 0 )
	{
		SPSR;
		SPDR;
		spibuffer.sending = 1;
		c = spibuffer.spibuffer[ spibuffer.spiBufferHead ];
		spibuffer.spiBufferHead++;
		spibuffer.spiBufferHead &= SPI_BUFFER_MASK;
		spibuffer.spiBufferLen--;
		SPDR = c;
	}
}

ISR( SPI_STC_vect )
{
	spibuffer.sending = 0;
	spi_send();
}	
