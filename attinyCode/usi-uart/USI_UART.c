/*****************************************************************************
*
* Copyright (C) 2003 Atmel Corporation
*
* File              : USI_UART.c
* Compiler          : GCC
* Created           : 18.07.2002 by JLL
* Modified          : 02-10-2003 by LTA
*
* Support mail      : avr@atmel.com
*
* Supported devices : ATtiny26
*
* Application Note  : AVR307 - Half duplex UART using the USI Interface
*
* Description       : Functions for USI_UART_receiver and USI_UART_transmitter.
*                     Uses Pin Change Interrupt to detect incomming signals.
*
*
****************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "USI_UART.h"


//********** USI UART Defines **********//
#define DATA_BITS                 8
#define START_BIT                 1
#define STOP_BIT                  1
#define HALF_FRAME                5

#define USI_COUNTER_MAX_COUNT     16
#define USI_COUNTER_SEED_TRANSMIT (USI_COUNTER_MAX_COUNT - HALF_FRAME)

//#define INSTRUCTIONS_BEFORE_COUNTER_STARTS 0x11 //the original value
#define CPU_CLOCKS_BEFORE_COUNTER_STARTS 14
#define INTERRUPT_STARTUP_DELAY   (CPU_CLOCKS_BEFORE_COUNTER_STARTS / TIMER_PRESCALER)

#define CPU_CLOCKS_BEFORE_TIMER_IS_RELOADED	2
#define TIMER0_SEED			( 256 - ( ( SYSTEM_CLOCK / BAUDRATE) / TIMER_PRESCALER ) + ( CPU_CLOCKS_BEFORE_TIMER_IS_RELOADED/TIMER_PRESCALER ) + 1 )
#define TIMER0_SEED_INV 	( ( ( SYSTEM_CLOCK / BAUDRATE ) / TIMER_PRESCALER ) - ( CPU_CLOCKS_BEFORE_TIMER_IS_RELOADED/TIMER_PRESCALER ) - 1 )
//#define TIMER0_SEED_INV	( ( (SYSTEM_CLOCK / BAUDRATE) / TIMER_PRESCALER ) )

//SYSTEM_CLOCK(3686400)/BAUDRATE(19200) = 192
//TIMER_PRESCALER = 1

#if ( (( (SYSTEM_CLOCK / BAUDRATE) / TIMER_PRESCALER ) * 3/2) > (256 - INTERRUPT_STARTUP_DELAY) )
	#define INITIAL_TIMER0_SEED       ( 256 - (( (SYSTEM_CLOCK / BAUDRATE) / TIMER_PRESCALER ) * 1/2) )
	#define USI_COUNTER_SEED_RECEIVE  ( USI_COUNTER_MAX_COUNT - (START_BIT + DATA_BITS) )
#else
	#define INITIAL_TIMER0_SEED       ( 256 - (( (SYSTEM_CLOCK / BAUDRATE) / TIMER_PRESCALER ) * 3/2) )
	#define USI_COUNTER_SEED_RECEIVE  (USI_COUNTER_MAX_COUNT - DATA_BITS)
#endif

#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1 )
#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
#error RX buffer size is not a power of 2
#endif


#define UART_TX_BUFFER_MASK ( UART_TX_BUFFER_SIZE - 1 )
#if ( UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK )
#error TX buffer size is not a power of 2
#endif

#if (TIMER_PRESCALER == 1)
	#define TIMER0_PRESCALE_VALUE ( _BV(CS00) )
#endif

#if (TIMER_PRESCALER == 8)
	#define TIMER0_PRESCALE_VALUE ( _BV(CS01) )
#endif

/* General defines */
#define TRUE                      1
#define FALSE                     0

//********** Static Variables **********//
static volatile unsigned char USI_UART_TxData;

static unsigned char          UART_RxBuf[UART_RX_BUFFER_SIZE];  // UART buffers. Size is definable in the header file.
static volatile unsigned char UART_RxHead;
static volatile unsigned char UART_RxTail;

static unsigned char          UART_TxBuf[UART_TX_BUFFER_SIZE];
static volatile unsigned char UART_TxHead;
static volatile unsigned char UART_TxTail;



static volatile union USI_UART_status {                         // Status byte holding flags.
    unsigned char status;
    struct {
        unsigned char ongoing_Transmission_From_Buffer:1;
        unsigned char ongoing_Transmission_Of_Package:1;
        unsigned char ongoing_Reception_Of_Package:1;
        unsigned char reception_Buffer_Overflow:1;
        unsigned char flag4:1;
        unsigned char flag5:1;
        unsigned char flag6:1;
        unsigned char flag7:1;
    };
} USI_UART_status = {0};


//********** USI_UART functions **********//

// Reverses the order of bits in a byte.
// I.e. MSB is swapped with LSB, etc.
unsigned char Bit_Reverse( unsigned char x )
{
    x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
    x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
    x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
    return x;
}

// Flush the UART buffers.
void USI_UART_Flush_Buffers( void )
{
    uint8_t oldSREG = SREG;
    cli();

    UART_RxTail = 0;
    UART_RxHead = 0;

    // Re-enable interrupts
    SREG = oldSREG;
}


// Initialise USI for UART transmission.
void USI_UART_Initialise_Transmitter( void )
{
    uint8_t oldSREG = SREG;
    cli();

    TCNT0   = 0x00;
	TCCR0   = _BV(PSR0)|TIMER0_PRESCALE_VALUE;				// Reset the prescaler and start Timer0.
    TIFR    = _BV(TOV0);									// Clear Timer0 OVF interrupt flag.
    TIMSK  |= _BV(TOIE0);									// Enable Timer0 OVF interrupt.

/*
    USICR  = ~_BV(USISIE)| _BV(USIOIE)|					// Enable USI Counter OVF interrupt.
             ~_BV(USIWM1)| _BV(USIWM0)|					// Select Three Wire mode.
             ~_BV(USICS1)| _BV(USICS0)| ~_BV(USICLK)|		// Select Timer0 OVER as USI Clock source.
             ~_BV(USITC);
*/
    USICR  = _BV(USIOIE)|									// Enable USI Counter OVF interrupt.
             _BV(USIWM0)|									// Select Three Wire mode.
             _BV(USICS0); 									// Select Timer0 OVER as USI Clock source.

    USIDR  = 0xFF;											// Make sure MSB is '1' before enabling USI_DO.
    USISR  = 0xF0 |											// Clear all USI interrupt flags.
             0x0F;											// Preload the USI counter to generate interrupt at first USI clock.

    DDRB  |=  _BV(PB1);										// Configure USI_DO as output.

    USI_UART_status.ongoing_Transmission_From_Buffer = TRUE;

    // Re-enable interrupts
    SREG = oldSREG;
}

// Initialise USI for UART reception.
// The USI is configured to start data reading on USI Start Condition Interrupt.
void USI_UART_Initialise_Receiver( void )
{
    uint8_t oldSREG = SREG;
    cli();

	USISR  = 0xF0;
	USICR  =  _BV(USISIE)|									// Enable USI Start Condition Interrupt.
			  _BV(USIWM0)|									// Select Three Wire mode.
			  _BV(USICS1)|  _BV(USICS0);					// Select Timer0 OVER as USI Clock source.

    PORTB |= _BV(PB0) | _BV(PB1);							// Enable pull up on USI DI; MIRCHO: SCK has an external pull up.
    DDRB  &= ~( _BV(PB2) | _BV(PB0) | _BV(PB1) );			// Set USI DI, DO and SCK pins as inputs.
    // Re-enable interrupts
    SREG = oldSREG;
}

// Puts data in the transmission buffer, after reverseing the bits in the byte.
// Initiates the transmission rutines if not already started.
void USI_UART_Transmit_Byte( unsigned char data )
{
    unsigned char tmphead;
	tmphead = ( UART_TxHead + 1 ) & UART_TX_BUFFER_MASK;		// Calculate buffer index.
    while ( tmphead == UART_TxTail );							// Wait for free space in buffer.
	UART_TxBuf[tmphead] = Bit_Reverse(data);					// Reverse the order of the bits in the data byte and store data in buffer.
	UART_TxHead = tmphead;										// Store new index.
    if ( !USI_UART_status.ongoing_Transmission_From_Buffer ) {	// Start transmission from buffer (if not already started).
        while ( USI_UART_status.ongoing_Reception_Of_Package );	// Wait for USI to finsh reading incoming data.
        USI_UART_Initialise_Transmitter();
    }
}

// Returns a byte from the receive buffer. Waits if buffer is empty.
unsigned char USI_UART_Receive_Byte( void )
{
    unsigned char tmptail;
    unsigned char result;
    while ( UART_RxHead == UART_RxTail );					// Wait for incomming data
    tmptail = ( UART_RxTail + 1 ) & UART_RX_BUFFER_MASK;	// Calculate buffer index
    UART_RxTail = tmptail;									// Store new index
    result = Bit_Reverse( UART_RxBuf[tmptail] );			// Reverse the order of the bits in the data byte before it returns data from the buffer
    return result;              
}

// Check if there is data in the receive buffer.
unsigned char USI_UART_Data_In_Receive_Buffer( void )
{
    return ( UART_RxHead != UART_RxTail );					// Return 0 (FALSE) if the receive buffer is empty.
}


// ********** Interrupt Handlers ********** //

ISR( USI_STRT_vect )
{
	TCNT0  	=  INTERRUPT_STARTUP_DELAY + INITIAL_TIMER0_SEED;	// Plant TIMER0 seed to match baudrate (incl interrupt start up time.).
	TCCR0   =  _BV(PSR0)|TIMER0_PRESCALE_VALUE;					// Reset the prescaler and start Timer0.
	TIFR   	=  _BV(TOV0);										// Clear Timer0 OVF interrupt flag.
	TIMSK  |=  _BV(TOIE0);										// Enable Timer0 OVF interrupt.

	USICR   = _BV(USIOIE)|										// Enable USI Counter OVF interrupt.
			  _BV(USIWM0)|										// Select Three Wire mode.
			  _BV(USICS0);										// Select Timer0 OVER as USI Clock source.

	USISR   = 0xF0 |											// Clear all USI interrupt flags.
			  USI_COUNTER_SEED_RECEIVE;							// Preload the USI counter to generate interrupt.

	USI_UART_status.ongoing_Reception_Of_Package = TRUE;
}

/*
// The pin change interrupt is used to detect USI_UART reseption.
// It is here the USI is configured to sample the UART signal.
ISR( IO_PINS_vect )
{
    if (!( PINB & (1<<PB0) )) {                                   // If the USI DI pin is low, then it is likely that it
        //  was this pin that generated the pin change interrupt.
        TCNT0  = INTERRUPT_STARTUP_DELAY + INITIAL_TIMER0_SEED;   // Plant TIMER0 seed to match baudrate (incl interrupt start up time.).
        TCCR0  = (1<<PSR0)|(0<<CS02)|(1<<CS01)|(0<<CS00);         // Reset the prescaler and star
		TIFR   = (1<<TOV0);                                       // Clear Timer0 OVF interrupt flag.
        TIMSK |= (1<<TOIE0);                                      // Enable Timer0 OVF interrupt.

        GIFR   =  (1<<PCIF);					  // Clear pin change interrupt flag.
        //GIMSK &=  ~(1<<PCIE0);                                  // Disable pin change interrupt for PB3:0.

        USICR  = (0<<USISIE)|(1<<USIOIE)|                         // Enable USI Counter OVF interrupt.
                 (0<<USIWM1)|(1<<USIWM0)|                         // Select Three Wire mode.
                 (0<<USICS1)|(1<<USICS0)|(0<<USICLK)|             // Select Timer0 OVER as USI Clock source.
                 (0<<USITC);

        USISR  = 0xF0 |                                           // Clear all USI interrupt flags.
                 USI_COUNTER_SEED_RECEIVE;                        // Preload the USI counter to generate interrupt.
		USIDR  = 0x00;
        // Note that enabling the USI will also disable the pin change interrupt.


        USI_UART_status.ongoing_Reception_Of_Package = TRUE;
    }
}
*/

// The USI Counter Overflow interrupt is used for moving data between memmory and the USI data register.
// The interrupt is used for both transmission and reception.
ISR( USI_OVF_vect )
{
    unsigned char tmphead, tmptail, resetUSI = FALSE;

    // Check if we are running in Transmit mode.
    if( USI_UART_status.ongoing_Transmission_From_Buffer ) {
        // If ongoing transmission, then send second half of transmit data.
        if( USI_UART_status.ongoing_Transmission_Of_Package ) {
            USI_UART_status.ongoing_Transmission_Of_Package = FALSE;		// Clear on-going package transmission flag.

            USISR = 0xF0 | ( USI_COUNTER_SEED_TRANSMIT );					// Load USI Counter seed and clear all USI flags.
            USIDR = ( USI_UART_TxData << 3 ) | 0x07;						// Reload the USIDR with the rest of the data and a stop-bit.
        }
        // Else start sendinbg more data or leave transmit mode.
        else {
            // If there is data in the transmit buffer, then send first half of data.
            if ( UART_TxHead != UART_TxTail ) {
                USI_UART_status.ongoing_Transmission_Of_Package = TRUE;		// Set on-going package transmission flag.

                tmptail = ( UART_TxTail + 1 ) & UART_TX_BUFFER_MASK;		// Calculate buffer index.
                UART_TxTail = tmptail;										// Store new index.
                USI_UART_TxData = UART_TxBuf[tmptail];						// Read out the data that is to be sent. Note that the data must be bit reversed before sent.
                // The bit reversing is moved to the application section to save time within the interrupt.
                USISR  = 0xF0 | ( USI_COUNTER_SEED_TRANSMIT );				// Load USI Counter seed and clear all USI flags.
                USIDR  = (USI_UART_TxData >> 2) | 0x80;						// Copy (initial high state,) start-bit and 6 LSB of original data (6 MSB
                															//  of bit reversed data).
            }
            // Else enter receive mode.
            else {
                USI_UART_status.ongoing_Transmission_From_Buffer = FALSE;
                resetUSI = TRUE;
            }
        }
    }
    // Else running in receive mode.
    else {
        USI_UART_status.ongoing_Reception_Of_Package = FALSE;
        tmphead = ( UART_RxHead + 1 ) & UART_RX_BUFFER_MASK;				// Calculate buffer index.

        if ( tmphead == UART_RxTail ) {										// If buffer is full trash data and set buffer full flag.
            USI_UART_status.reception_Buffer_Overflow = TRUE;				// Store status to take actions elsewhere in the application code
        } else {															// If there is space in the buffer then store the data.
            UART_RxHead = tmphead;											// Store new index.
            UART_RxBuf[tmphead] = USIDR;									// Store received data in buffer. Note that the data must be bit reversed before used.
        }																	// The bit reversing is moved to the application section to save time within the interrupt.
        resetUSI = TRUE;
    }

    if( resetUSI ) {
		TIFR   =  _BV(TOV0);												// Clear Timer0 OVF interrupt flag.
		TIMSK &= ~_BV(TOIE0);												// Disable Timer0 OVF interrupt.
        TCCR0  = 0;															// Stop Timer0.

		PORTB |= _BV(PB0) | _BV(PB1);										// Enable pull up on USI DI; MIRCHO: SCK has an external pull up.
		DDRB  &= ~( _BV(PB2) | _BV(PB0) | _BV(PB1) );						// Set USI DI, DO and SCK pins as inputs.

		//we are initializing the receiver here
		USISR  =  0xF0;
		USICR  =  _BV(USISIE)|												// Enable USI Start Condition Interrupt.
				  _BV(USIWM0)|												// Select Three Wire mode.
				  _BV(USICS1)|  _BV(USICS0);								// Select Timer0 OVER as USI Clock source.
    }
}

// Timer0 Overflow interrupt is used to trigger the sampling of signals on the USI ports.
void TIMER0_OVF0_vect(void) __attribute__((signal,naked));
void TIMER0_OVF0_vect(void)
{
    /*     TCNT0 += TIMER0_SEED; */

    asm volatile( "push		r24" );
    asm volatile( "in		r24, 0x3f" );
    asm volatile( "push		r24" );

    asm volatile( "in		r24, 0x32" );
    asm volatile( "subi		r24, %[seed]" :: [seed] "M" TIMER0_SEED_INV );
    asm volatile( "out		0x32, r24" );

    asm volatile( "pop		r24" );
    asm volatile( "out		0x3f, r24" );
    asm volatile( "pop		r24" );

    asm volatile ("reti");
}

/*
ISR(TIMER0_OVF0_vect)
{
    TCNT0 += TIMER0_SEED;                   // Reload the timer,
                                            // current count is added for timing correction.
}
*/

