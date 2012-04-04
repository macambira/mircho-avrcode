// This file has been prepared for Doxygen automatic documentation generation.
/*! \file ********************************************************************
*
* Atmel Corporation
*
* \li File:               spi_via_usi_driver.c
* \li Compiler:           IAR EWAAVR 3.10c
* \li Support mail:       avr@atmel.com
*
* \li Supported devices:  All devices with Universal Serial Interface (USI)
*                         capabilities can be used.
*                         The example is written for ATmega169.
*
* \li AppNote:            AVR319 - Using the USI module for SPI communication.
*
* \li Description:        Example on how to use the USI module for communicating
*                         with SPI compatible devices. The functions and variables
*                         prefixed 'spiX_' can be renamed to be able to use several
*                         spi drivers (using different interfaces) with similar names.
*                         Some basic SPI knowledge is assumed.
*
*                         $Revision: 1.4 $
*                         $Date: Monday, September 13, 2004 12:08:54 UTC $
****************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "spi_via_usi_driver.h"

//if only slave
#define ONLYSLAVE
//if only master
//#define ONLYMASTER
//if duplex
//#define DUPLEX

/* USI port and pin definitions.
 */
#define USI_OUT_REG	PORTB	//!< USI port output register.
#define USI_IN_REG	PINB	//!< USI port input register.
#define USI_DIR_REG	DDRB	//!< USI port direction register.
#define USI_CLOCK_PIN	PB2	//!< USI clock I/O pin.
#define USI_DATAIN_PIN	PB0	//!< USI data input pin.
#define USI_DATAOUT_PIN	PB1	//!< USI data output pin.


/*  Speed configuration:
 *  Bits per second = CPUSPEED / PRESCALER / (COMPAREVALUE+1) / 2.
 *  Maximum = CPUSPEED / 64.
 */
#define TC1_PRESCALER_VALUE 256	//!< Must be 1, 8, 64, 256 or 1024.
#define TC1_COMPARE_VALUE   1	//!< Must be 0 to 255. Minimum 31 with prescaler CLK/1.


/*  Prescaler value converted to bit settings.
 */
#if TC1_PRESCALER_VALUE == 1
	#define TC1_PS_SETTING (1<<CS10)
#elif TC1_PRESCALER_VALUE == 8
	#define TC1_PS_SETTING (1<<CS12)
#elif TC1_PRESCALER_VALUE == 64
	#define TC1_PS_SETTING (1<<CS12)|(1<<CS11)|(1<<CS10)
#elif TC1_PRESCALER_VALUE == 256
	#define TC1_PS_SETTING (1<<CS13)|(1<<CS10)
#elif TC1_PRESCALER_VALUE == 1024
	#define TC1_PS_SETTING (1<<CS13)|(1<<CS11)|(1<<CS10)
#else
	#error Invalid T/C0 prescaler setting.
#endif


#if defined ONLYMASTER || defined DUPLEX

extern unsigned char spi_send(unsigned char msg);

#endif

#if !defined ONLYMASTER

#define SPI_BUFFER_SIZE 64	//should be power of 2, other routines are built on that
#define SPI_BUFFER_MASK (SPI_BUFFER_SIZE-1)


typedef struct _spibuffer  {
	uint8_t spiBufferHead;
	uint8_t spiBufferTail;
	uint8_t spiBufferLen;
	uint8_t spibuffer[ SPI_BUFFER_SIZE ];	
} SPIBUFFER;

volatile SPIBUFFER spiReceiveBuffer = {0, 0, 0, 0};

inline void spi_receive_buffer_write( uint8_t chr )
{
	if( spiReceiveBuffer.spiBufferLen < SPI_BUFFER_SIZE )
	{
		spiReceiveBuffer.spibuffer[ spiReceiveBuffer.spiBufferTail ] = chr;
		spiReceiveBuffer.spiBufferTail++;
		spiReceiveBuffer.spiBufferTail &= SPI_BUFFER_MASK; //AND with 0b11111 (31)
		spiReceiveBuffer.spiBufferLen++;
	}
}

uint8_t spi_receive_buffer_read( void )
{
	uint8_t result = 0;
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		if( spiReceiveBuffer.spiBufferLen > 0 )
		{
			result = spiReceiveBuffer.spibuffer[ spiReceiveBuffer.spiBufferHead ];
			spiReceiveBuffer.spiBufferHead++;
			spiReceiveBuffer.spiBufferHead &= SPI_BUFFER_MASK; //AND with 0b11111 (31)
			spiReceiveBuffer.spiBufferLen--;		
		}
	}
	return result;
}

inline uint8_t spi_receive_buffer_not_empty( void )
{
	return ( spiReceiveBuffer.spiBufferLen );
}

#endif

volatile struct usidriverStatus_t spiX_status;

/*! \brief  Data input register buffer.
 *
 *  Incoming bytes are stored in this byte until the next transfer is complete.
 *  This byte can be used the same way as the SPI data register in the native
 *  SPI module, which means that the byte must be read before the next transfer
 *  completes and overwrites the current value.
 */
unsigned char storedUSIDR;


/*! \brief  Timer/Counter 0 Compare Match Interrupt handler.
 *
 *  This interrupt handler is only enabled when transferring data
 *  in master mode. It toggles the USI clock pin, i.e. two interrupts
 *  results in one clock period on the clock pin and for the USI counter.
 */
//#pragma vector=TIMER0_COMP_vect
ISR(TIMER1_COMPA_vect)
{
	USICR |= _BV(USITC);	// Toggle clock output pin.
}



/*! \brief  USI Timer Overflow Interrupt handler.
 *
 *  This handler disables the compare match interrupt if in master mode.
 *  When the USI counter overflows, a byte has been transferred, and we
 *  have to stop the timer tick.
 *  For all modes the USIDR contents are stored and flags are updated.
 */
//#pragma vector=USI_OVF_vect
ISR(USI_OVF_vect)
{
	// Master must now disable the compare match interrupt
	// to prevent more USI counter clocks.
	#if defined ONLYMASTER || defined DUPLEX
	if( spiX_status.masterMode == 1 ) 
	{
		TIMSK &= ~_BV(OCIE1A);
	}
	#endif

	// Update flags and clear USI counter
	USISR = _BV(USIOIF);
	spiX_status.transferComplete = 1;

	// Copy USIDR to buffer to prevent overwrite on next transfer.
	storedUSIDR = USIDR;
	spi_receive_buffer_write( storedUSIDR );
}



/*! \brief  Initialize USI as SPI master.
 *
 *  This function sets up all pin directions and module configurations.
 *  Use this function initially or when changing from slave to master mode.
 *  Note that the stored USIDR value is cleared.
 *
 *  \param spi_mode  Required SPI mode, must be 0 or 1.
 */
#if defined ONLYMASTER || defined DUPLEX

void spiX_initmaster( char spi_mode )
{
	//Reinitializing after Slave changes USIDR content

	// Configure USI to 3-wire master mode with overflow interrupt.
	USICR = _BV(USIOIE) | _BV(USIWM0) |
	        _BV(USICS1) | (spi_mode<<USICS0) |
	        _BV(USICLK);

	// Configure port directions.
	USI_DIR_REG |= _BV(USI_DATAOUT_PIN) | _BV(USI_CLOCK_PIN); // Outputs.
	USI_DIR_REG &= ~_BV(USI_DATAIN_PIN);                      // Inputs.
	USI_OUT_REG |= _BV(USI_DATAIN_PIN);                       // Pull-ups.


	// Enable 'Clear Timer on Compare match' and init prescaler.
	TCCR1A = _BV(CTC1) | TC1_PS_SETTING;
	// Initialize Output Compare Register.
	OCR1A = TC1_COMPARE_VALUE;

	// Init driver status register.
	spiX_status.masterMode       = 1;
	spiX_status.transferComplete = 0;
	spiX_status.writeCollision   = 0;

	storedUSIDR = 0;
}

#endif



/*! \brief  Initialize USI as SPI slave.
 *
 *  This function sets up all pin directions and module configurations.
 *  Use this function initially or when changing from master to slave mode.
 *  Note that the stored USIDR value is cleared.
 *
 *  \param spi_mode  Required SPI mode, must be 0 or 1.
 */
#if defined ONLYSLAVE || defined  DUPLEX

void spiX_initslave( char spi_mode )
{
	// Configure port directions.
	//USI_OUT_REG = 0b00000000;
	USI_DIR_REG |=  _BV(USI_DATAOUT_PIN);                      // Outputs.
	USI_OUT_REG |=  _BV(USI_DATAIN_PIN) | _BV(USI_CLOCK_PIN);  // Pull-ups.
	USI_DIR_REG &= ~( _BV(USI_DATAIN_PIN) | _BV(USI_CLOCK_PIN) ); // Inputs.

	// Configure USI to 3-wire slave mode with overflow interrupt.
	USICR = _BV(USIOIE) | _BV(USIWM0) |
	        _BV(USICS1) | (spi_mode<<USICS0);

	// Init driver status register.
	spiX_status.masterMode       = 0;
	spiX_status.transferComplete = 0;
	spiX_status.writeCollision   = 0;

	storedUSIDR = 0;
}

#endif

/*! \brief  Put one byte on bus.
 *
 *  Use this function like you would write to the SPDR register in the native SPI module.
 *  Calling this function in master mode starts a transfer, while in slave mode, a
 *  byte will be prepared for the next transfer initiated by the master device.
 *  If a transfer is in progress, this function will set the write collision flag
 *  and return without altering the data registers.
 *
 *  \returns  0 if a write collision occurred, 1 otherwise.
 */
unsigned char spiX_put( unsigned char val )
{
	// Check if transmission in progress,
	// i.e. USI counter unequal to zero.
	if( (USISR & 0x0F) != 0 ) {
		// Indicate write collision and return.
		spiX_status.writeCollision = 1;
		return 0;
	}

	// Reinit flags.
	spiX_status.transferComplete = 0;
	spiX_status.writeCollision = 0;

	#ifdef ONLYMASTER
		// Put data in USI data register.
			spi_send(val);

	#endif

	#ifdef DUPLEX
		// Put data in USI data register.
		if(spiX_status.masterMode){
			spi_send(val);
		}
		else
		{
			USIDR=val;//In Slave Mode fill Shift Register, which gets externally clocked
		}
		
	#endif

	#ifdef ONLYSLAVE

		USIDR=val;//In Slave Mode fill Shift Register, which gets externally clocked

	#endif

	// Master should now enable compare match interrupts.
	if( spiX_status.masterMode == 1 ) {
		TIFR  |= _BV(OCF1A);   // Clear compare match flag.
		TIMSK |= _BV(OCIE1A); // Enable compare match interrupt.
	}	

	if( spiX_status.writeCollision == 0 ) return 1;
	return 0;
}



/*! \brief  Get one byte from bus.
 *
 *  This function only returns the previous stored USIDR value.
 *  The transfer complete flag is not checked. Use this function
 *  like you would read from the SPDR register in the native SPI module.
 */
unsigned char spiX_get(void)
{
	return storedUSIDR;
}



/*! \brief  Wait for transfer to complete.
 *
 *  This function waits until the transfer complete flag is set.
 *  Use this function like you would wait for the native SPI interrupt flag.
 */
void spiX_wait(void)
{
	do {} while( spiX_status.transferComplete == 0 );
}



// end of file
