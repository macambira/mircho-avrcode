/*****************************************************************************
*
* Mircho Mirev 2010
*
* Based on Atmel
* AppNote       : AVR307 - Half duplex UART using the USI Interface
*
****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "USI_UART.h"

void uart_puts( unsigned char* str );

int main( void ) __attribute__( ( noreturn ) ) __attribute__( ( OS_main ) );

#define SETBIT(x,y) (x |= (_BV(y))) /* Set bit y in byte x*/
#define CLEARBIT(x,y) (x &= (~_BV(y))) /* Clear bit y in byte x*/
#define CHECKBIT(x,y) (x & (_BV(y))) /* Check bit y in byte x*/

//disable adc to conserve power
#define DISABLE_ADC()	({\
	CLEARBIT(ADCSR,ADEN);\
	CLEARBIT(ACSR,ACD);\
	})

/**
 *		Timers control
 **/
static void initTimer1(void);
#define startTimer1() ({\
	SETBIT( TIMSK, TOIE1 );\
	})
static void stopTimer1(void);
volatile uint8_t Timer1Counter0;
volatile uint8_t Timer1Counter1;

static inline void waitTimerTicks( uint8_t ticks ) __attribute__((always_inline));
#define WAIT_TIMER_TICKS( x ) ({\
		Timer1Counter0 = x;\
		do{}while(Timer1Counter0>0);\
	})

/**
 *		Timers control
 **/

#define INIT_LED()\
({\
    CLEARBIT( PORTA, PA0 );\
    SETBIT( DDRA, PA0 );\
})

#define LED_ON SETBIT( PORTA, PA0 )
#define LED_OFF CLEARBIT( PORTA, PA0 )

int main( void )
{
	DISABLE_ADC();

    unsigned char myString[] = "I am alive!\r\n";
    unsigned char dabait;


	INIT_LED();

    initTimer1();

    USI_UART_Flush_Buffers();
    USI_UART_Initialise_Receiver();                                         // Initialisation for USI_UART receiver

    sei();																	// Enable global interrupts
    set_sleep_mode( SLEEP_MODE_IDLE );
	startTimer1();

    for( ; ; ) {                                                            // Run forever
        if( USI_UART_Data_In_Receive_Buffer() ) {
            LED_ON;
            WAIT_TIMER_TICKS(1);
            LED_OFF;
            dabait = USI_UART_Receive_Byte();
            USI_UART_Transmit_Byte( dabait );

            /*
			if( dabait == 'o' )
			{
				LED_ON;
			}
			if( dabait == 'f' )
			{
				LED_OFF;
			}
			*/

            uart_puts( myString );
		}
        else
        {
        	cli();
        	sleep_enable();
        	sei();
        	sleep_cpu();
        	sleep_disable();
        }
    }
}

void uart_puts( unsigned char* str )
{
    while( *str ) {
        USI_UART_Transmit_Byte( (unsigned int)*str++ );
    }
}

static void initTimer1(void)
{
    TCNT1 = 178;
    TCCR1A = 0;
    TCCR1B |= _BV( CS13 ) | _BV( CS11 ) | _BV( CS00  );
}

static void stopTimer1(void)
{
    CLEARBIT( TIMSK, TOIE1 );
    TCCR1A = 0;
    TCCR1B = 0;
}

ISR( TIMER1_OVF1_vect )
{
    uint8_t elapsed;
    if( Timer1Counter0 ) Timer1Counter0--;
    if( Timer1Counter1 ) Timer1Counter1--;
    elapsed = TCNT1;
    TCNT1 = 178 + elapsed;
}

static inline void waitTimerTicks( uint8_t ticks )
{
    Timer1Counter0 = ticks;
    do {} while(Timer1Counter0>0);
}


//
// calculate a Dallas CRC8 checksum
//
static uint8_t calc_crc8(uint8_t* data_pointer, uint8_t number_of_bytes)
{
	uint8_t temp1, bit_counter, feedback_bit, crc8_result=0;

	while(number_of_bytes--)
	{
		temp1= *data_pointer++;

		for(bit_counter=8; bit_counter; bit_counter--)
		{
			feedback_bit=(crc8_result & 0x01);
			crc8_result >>= 1;
			if(feedback_bit ^ (temp1 & 0x01))
			{
				crc8_result ^= 0x8c;
			}
			temp1 >>= 1;
		}
	}
 
	return crc8_result;
}


