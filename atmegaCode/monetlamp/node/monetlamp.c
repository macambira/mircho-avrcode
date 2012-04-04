#include "monetlamp.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "monetlamp.h"
#include "util.h"
#include "tick.h"
#include "event.h"
#include "eventcodes.h"
#include "spi.h"
#include "spilcd.h"
#include "led.h"
#include "uart485.h"
#include "packet.h"
#include "uart485_def.h"

/************************************************************************/
/* Timings for tasks, 1ms is 2 ticks                                    */
/************************************************************************/
#define TIMING_SECONDS_TASK					(TIMING_SECOND_IN_TICKS)

#define TIMING_SEND_TO_SLAVE1_TASK			(TIMING_SECOND_IN_TICKS*5)

#define TIMING_STOP_LED_IN_ONE_SECOND_TASK			(TIMING_SECOND_IN_TICKS)

uint8_t STOP_LED_IN_ONE_SECOND_TASK;

void systemSleep( void );

void uart485DataAvailable( uint8_t sizeOfData );

DEFINE_EVENT_HANDLER( SECONDS_TIMER_HANDLER )
//event eventObject as param
{
	static uint8_t counter;
	counter++;
	
	if( counter == 2 )
	{
		//LEDON
	}
	else if( counter == 3 )
	{
		//LEDOFF
		counter = 0;
	}
}

DEFINE_EVENT_HANDLER( UART_DATA_AVAILABLE )
{
	readPacket();

	LEDON;
	if( isPacketValid() )
	{
		//if( thePacket.pdata.commandByte == 1 )
		{
			LEDON;
			UNFREEZE_TASK( STOP_LED_IN_ONE_SECOND_TASK );
		}
	}
	else
	{
		LEDOFF;
	}
}

DEFINE_EVENT_HANDLER( STOP_LED_IN_ONE_SECOND )
{
	LEDOFF;
}

#if NODE==MASTER
DEFINE_EVENT_HANDLER( UART_DATA_SEND_TO_SLAVE1 )
{
	thePacket.pdata.sendAddress = NODE_ADDRESS;
	thePacket.pdata.recvAddress = SLAVE1_ADDRESS;
	thePacket.pdata.commandByte = 1;
	thePacket.pdata.dataByte1	= 1;
	thePacket.pdata.crc8		= 0xFF;
	
	sendPacket();
}
#endif

int main(void)
{

    cli();
    wdt_disable();

#if defined (LED_H_)

    LEDINIT()

#endif

    //enable global interrupts
    sei();

    LEDON
    _delay_ms( 500 );
    LEDOFF
    _delay_ms( 1000 );
    
    TS_init();

    LEDON
    _delay_ms( 100 );
    LEDOFF
    _delay_ms( 100 );
    LEDON
    _delay_ms( 100 );
    LEDOFF
    _delay_ms( 100 );
    LEDON
    _delay_ms( 100 );
    LEDOFF
    _delay_ms( 100 );

#if defined(LCD_ATTACHED)
    spi_init();
    _delay_ms( 1000 );
#endif

    //this clears the display
    PRINT_TO_LCD_AT( 0, 0, (uint8_t *)"        " );

#if NODE==MASTER
    PRINT_TO_LCD_AT( 1, 0, (uint8_t *)"      m1" );
#elif NODE==SLAVE1
    PRINT_TO_LCD_AT( 1, 0, (uint8_t *)"      s1" );
#endif

    REGISTER_EVENT_HANDLER( SECONDS_TIMER_HANDLER )
    REGISTER_EVENT_HANDLER( STOP_LED_IN_ONE_SECOND )
    REGISTER_EVENT_HANDLER( UART_DATA_AVAILABLE )
    /**
     * Tasks adding and initialization
     */

    ADD_EVENT_TASK( EVENT_CODE( SECONDS_TIMER_HANDLER ), 0, TIMING_SECONDS_TASK )
	STOP_LED_IN_ONE_SECOND_TASK = ADD_EVENT_TASK_AUTOFREEZE( EVENT_CODE( STOP_LED_IN_ONE_SECOND ), 0, TIMING_STOP_LED_IN_ONE_SECOND_TASK )

	#if NODE==MASTER
    REGISTER_EVENT_HANDLER( UART_DATA_SEND_TO_SLAVE1 )
    ADD_EVENT_TASK( EVENT_CODE( UART_DATA_SEND_TO_SLAVE1 ), 0, TIMING_SEND_TO_SLAVE1_TASK )
	#endif

    TS_start();

    /**
     * Tasks adding and initialization
     */
	
	/**
	 * UART485
	 */
	uart_485_init( NODE_ADDRESS );
	uart_485_set_as_receiver();
	uart_485_notify = (UART_CALLBACK_FUNCTION)uart485DataAvailable;

    while ( 1 )
    {
		TS_dispatchTasks();
		E_dispatchEvents();
    }
}

void uart485DataAvailable( uint8_t sizeOfData )
{
	RAISE_EVENT( UART_DATA_AVAILABLE, EVENT_PARAM( 0, sizeOfData ) );
}

// method to put MCU to sleep, deep power saving sleep
void systemSleep(void)
{
    set_sleep_mode( SLEEP_MODE_PWR_DOWN );
    sleep_enable();

    sleep_mode();

    // system continues execution here when watchdog times out
    sleep_disable();
}
