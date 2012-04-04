//lots of includes, but still fits nicely in a ATTINY13A
#ifdef F_CPU
#undef F_CPU
#endif
#define F_CPU (8000000UL)

#define ATMEGA8

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "util.h"
#include "tick.h"
#include "event.h"
#include "eventcodes.h"
#include "spi.h"
#include "spilcd.h"
#include "brightness.h"
#include "adc.h"
#include "led.h"
#include "indicator.h"
#include "encoder.h"

#define LCD_ATTACHED FALSE

/************************************************************************/
/* Timings for tasks, 1ms is 2 ticks                                    */
/************************************************************************/
#define TIMING_SECONDS_TASK					(TIMING_SECOND_IN_TICKS)
#define TIMING_ADC_PROCESS_DATA_TASK			(TIMING_SECOND_IN_TICKS * 3)
#define TIMING_SAVE_BRIGHTNESS_TASK			(TIMING_MILLISECOND_IN_TICKS * 1000)
#define TIMING_LAMP_AUTO_TURN_ON_TASK		(TIMING_MILLISECOND_IN_TICKS * 1000)
#define TIMING_TOUCH_PIN_CHARGE_TASK			(TIMING_MILLISECOND_IN_TICKS * 3)

#define LOW_LIGHT_VALUE 500
#define HIGH_LIGHT_VALUE 750

#define LIGHT_IS_HIGH( light ) ( light > HIGH_LIGHT_VALUE )
#define LIGHT_IS_LOW( light ) ( light < LOW_LIGHT_VALUE )

//the index of an initial lamp brightness. for example when auto turning on the lamp.
#define INIT_BRIGHTNESS_INDEX 40

#define MANUAL_ACTION_STOP 		1
#define MANUAL_ACTION_LIGHTUP 	2
#define MANUAL_ACTION_NEUTRAL 	0

#define AUTO_ACTION_STOP 		1
#define AUTO_ACTION_LIGHTUP 		2

//6 hours
#define SECONDS_TO_FORGET_THE_LAST_MANUAL_ACTION ( 6 * 60 * 60 )
//10 minutes
//#define SECONDS_TO_FORGET_THE_LAST_MANUAL_ACTION ( 10 * 60 )
//8 hours
#define SECONDS_TO_RESET_LAST_MANUAL_ACTION_BECAUSE_OF_LIGHT ( 8 * 60 * 60 )

#define SECONDS_TO_RESET_LAST_AUTO_ACTION_BECAUSE_OF_LIGHT ( 10 * 60 )


#define ACTION_TRESHOLD 5

/**
 * System state data structure
 */
uint8_t maxLightIndex;

volatile struct
{
    union
    {
        struct
        {
            uint8_t userInputSinceLastAuto:1;
            uint8_t flag1:1;
            uint8_t flag2:1;
            uint8_t flag3:1;
            uint8_t flag4:1;
            uint8_t flag5:1;
            uint8_t flag6:1;
            uint8_t flag7:1;
        };
        uint8_t state;
    } systemState;
    uint8_t savedBrightnessIndex;
    uint8_t manualActionState;
    uint16_t secondsSinceLastAuto;
    uint16_t secondsSinceLastManual;
    uint8_t button1Timeout;
    uint8_t button2Timeout;
} lampSettings;

uint8_t SAVE_BRIGHTNESS_TASK;
uint8_t TURN_LAMP_ON_TASK, TURN_LAMP_OFF_TASK;
uint8_t TOUCH_PIN_CHARGED_TASK;
uint8_t SECONDS_TIMER_TASK;

uint8_t EEMEM EEPROM_status;
uint8_t EEMEM EEPROM_lastBrightnessIndex;

#if LCD_ATTACHED == TRUE
void printToLCDAt( uint8_t row, uint8_t col, const uint8_t *str );
#define PRINT_TO_LCD_AT( row, col, str ) (printToLCDAt( row, col, str ))
#else
#define PRINT_TO_LCD_AT( row, col, str ) while(0){};
#endif

void systemSleep( void );

void setBrightnessByIndex( void );
void saveLastBrightnessValue( sTask * );

//additional actions
#define INCREASE_LIGHT 1
#define DECREASE_LIGHT 2
#define STOP_LIGHT 3
#define START_LIGHT 4

void onManualAction( uint8_t actionType );
void onAutoAction(  uint8_t actionType );

#define CODESTR(str) ((uint8_t *)str)
const uint8_t *codes[8] =
{
    CODESTR("000"),
    CODESTR("001"),
    CODESTR("010"),
    CODESTR("011"),
    CODESTR("100"),
    CODESTR("101"),
    CODESTR("110"),
    CODESTR("111")
};

DEFINE_EVENT_HANDLER( ENCODER_STATUS_CHANGE )
//event eventObject as param
{
    static uint8_t direction;
    direction = eventObject.params.loParam;

    if ( direction == DIRECTION_FWD )
    {
        incBrightnessIdx( 1 );
        onManualAction( INCREASE_LIGHT );
    	PRINT_TO_LCD_AT( 1, 1, "I" );
    }
    else if ( direction == DIRECTION_BACK )
    {
        decBrightnessIdx( 1 );
        onManualAction( DECREASE_LIGHT );
    	PRINT_TO_LCD_AT( 1, 1, "D" );
    }

    PRINT_TO_LCD_AT( 0, 0, codes[ eventObject.params.hiParam ] ); //new status
}

DEFINE_EVENT_HANDLER( BUTTON1_PRESS )
//event eventObject as param
{
    PRINT_TO_LCD_AT( 1, 0, "B" );

    TS_delayTask( TURN_LAMP_OFF_TASK,  ( TIMING_SECOND_IN_TICKS * 1 ) );
    UNFREEZE_TASK( TURN_LAMP_OFF_TASK );
    FREEZE_TASK( TURN_LAMP_ON_TASK );

    onManualAction( STOP_LIGHT );
}

DEFINE_EVENT_HANDLER( BUTTON2_PRESS )
//event eventObject as param
{
    PRINT_TO_LCD_AT( 1, 1, "B" );

    TS_delayTask( TURN_LAMP_ON_TASK, ( TIMING_MILLISECOND_IN_TICKS * 200 ) ); //5 sec delay
    UNFREEZE_TASK( TURN_LAMP_ON_TASK );
    FREEZE_TASK( TURN_LAMP_OFF_TASK );

    onManualAction( START_LIGHT );
}

//event "fired" in brightness.c
DEFINE_EVENT_HANDLER( BRIGHTNESS_CHANGE )
//event eventObject as param
{
	if ( brightnessSystemState.brightnessIndex >= ACTION_TRESHOLD )
	{
		indicatorStopPlay();
	}
	if ( brightnessSystemState.brightnessIndex < ACTION_TRESHOLD )
	{
		indicatorPlay();
	}
}

DEFINE_EVENT_HANDLER( PIN_TOUCH )
//event eventObject as param
{
    uint8_t strBuffer[ 8 ];
    uint16_t comparatorTime;
    comparatorTime = eventObject.params.wParam;
    PRINT_TO_LCD_AT( 0, 4, itoa( comparatorTime, strBuffer, 10 ) );
}

DEFINE_EVENT_HANDLER( SECONDS_TIMER_HANDLER )
//event eventObject as param
{
    uint8_t strBuffer[ 8 ];
    lampSettings.secondsSinceLastManual++;
    lampSettings.secondsSinceLastAuto++;

    if ( lampSettings.secondsSinceLastManual > SECONDS_TO_FORGET_THE_LAST_MANUAL_ACTION )
    {
        lampSettings.manualActionState = MANUAL_ACTION_NEUTRAL;
    }

    PRINT_TO_LCD_AT( 1, 2, itoa( lampSettings.secondsSinceLastManual, strBuffer, 10 ) );
}

DEFINE_EVENT_HANDLER( ADC_HANDLER )
//event eventObject as param
{
    uint16_t lightValue;
    uint8_t strBuffer[8];

    lightValue = adcGetMovingAverage();

    PRINT_TO_LCD_AT( 0, 5, "   " );
    PRINT_TO_LCD_AT( 0, 5, itoa( lightValue, strBuffer, 10 ) );

    if ( LIGHT_IS_HIGH( lightValue ) )
    {
		if( lampSettings.manualActionState == MANUAL_ACTION_STOP )
		{
            lampSettings.manualActionState = MANUAL_ACTION_NEUTRAL;
		}
			
        if ( ( lampSettings.secondsSinceLastAuto > SECONDS_TO_RESET_LAST_AUTO_ACTION_BECAUSE_OF_LIGHT ) && ( lampSettings.manualActionState != MANUAL_ACTION_LIGHTUP ) )
        {
            if ( brightnessSystemState.brightnessIndex > 30 )
            {
                TS_delayTask( TURN_LAMP_OFF_TASK,  ( TIMING_SECOND_IN_TICKS * 5 ) ); //5 sec delay
                UNFREEZE_TASK( TURN_LAMP_OFF_TASK );
                onAutoAction( AUTO_ACTION_STOP );
            }
        }
    }
    else if ( LIGHT_IS_LOW( lightValue ) )
    {
		if( lampSettings.manualActionState == MANUAL_ACTION_LIGHTUP )
		{
            lampSettings.manualActionState = MANUAL_ACTION_NEUTRAL;
		}

        if ( ( lampSettings.secondsSinceLastAuto > SECONDS_TO_RESET_LAST_AUTO_ACTION_BECAUSE_OF_LIGHT ) && ( lampSettings.manualActionState != MANUAL_ACTION_STOP ) )
        {
            if ( brightnessSystemState.brightnessIndex < 10 )
            {
                TS_delayTask( TURN_LAMP_ON_TASK, ( TIMING_SECOND_IN_TICKS * 5 ) ); //5 sec delay
                UNFREEZE_TASK( TURN_LAMP_ON_TASK );
                onAutoAction( AUTO_ACTION_LIGHTUP );
            }
        }
    }
}

DEFINE_EVENT_HANDLER( TURN_LAMP_ON_WITH_DELAY_HANDLER )
//event eventObject as param
{
    if ( lampSettings.savedBrightnessIndex )
    {
        insertBrightnessRange( 0, lampSettings.savedBrightnessIndex );
    }
    else
    {
        insertBrightnessRange( 0, INIT_BRIGHTNESS_INDEX );
    }
}

DEFINE_EVENT_HANDLER( TURN_LAMP_OFF_WITH_DELAY_HANDLER )
//event eventObject as param
{
    if ( brightnessSystemState.brightnessIndex > 0 )
    {
        insertBrightnessRange( brightnessSystemState.brightnessIndex, 0 );
    }
}

DEFINE_EVENT_HANDLER( SAVE_BRIGHTNESS_VALUE_HANDLER )
//event eventObject as param
{
    uint8_t brightnessIndex;
    brightnessIndex = brightnessSystemState.brightnessIndex;
    saveBrightnessInEEPROM( brightnessIndex );
    PRINT_TO_LCD_AT( 1, 7, "S" );
}

int main(void)
{

    cli();
    wdt_disable();

#if defined (ATMEGA328) || defined (ATMEGA8)

    LEDINIT()

#endif

    //enable global interrupts
    sei();

#if LCD_ATTACHED == TRUE
    spi_init();
#endif

    TS_init();

    //this clears the display
    PRINT_TO_LCD_AT( 0, 0, "        " );
    PRINT_TO_LCD_AT( 1, 0, "      v1" );

    updateSystemStateFromEEPROM();

    adcInit();
    encoderInit();
	
    indicatorInit();
    indicatorPlay();
	
    brightnessInit();

    maxLightIndex = getMaxPWMValuesIndex();

    lampSettings.systemState.userInputSinceLastAuto = FALSE;
    lampSettings.secondsSinceLastManual = SECONDS_TO_RESET_LAST_MANUAL_ACTION_BECAUSE_OF_LIGHT;
    lampSettings.secondsSinceLastAuto = SECONDS_TO_RESET_LAST_AUTO_ACTION_BECAUSE_OF_LIGHT;
    lampSettings.manualActionState = MANUAL_ACTION_NEUTRAL;

    REGISTER_EVENT_HANDLER( PIN_TOUCH )
    REGISTER_EVENT_HANDLER( ADC_HANDLER )
    REGISTER_EVENT_HANDLER( SECONDS_TIMER_HANDLER )

    REGISTER_EVENT_HANDLER( BUTTON1_PRESS )
    REGISTER_EVENT_HANDLER( BUTTON2_PRESS )
    REGISTER_EVENT_HANDLER( ENCODER_STATUS_CHANGE )
    REGISTER_EVENT_HANDLER( BRIGHTNESS_CHANGE )

    REGISTER_EVENT_HANDLER( TURN_LAMP_ON_WITH_DELAY_HANDLER )
    REGISTER_EVENT_HANDLER( TURN_LAMP_OFF_WITH_DELAY_HANDLER )
    REGISTER_EVENT_HANDLER( SAVE_BRIGHTNESS_VALUE_HANDLER )

    //Tasks adding and initialization
     
    ADD_EVENT_TASK( EVENT_CODE( SECONDS_TIMER_HANDLER ), 0, TIMING_SECONDS_TASK );
    ADD_EVENT_TASK( EVENT_CODE( ADC_HANDLER ), TIMING_SECOND_IN_TICKS * 5, TIMING_ADC_PROCESS_DATA_TASK );

    TURN_LAMP_ON_TASK = ADD_EVENT_TASK_AUTOFREEZE( EVENT_CODE( TURN_LAMP_ON_WITH_DELAY_HANDLER ), 0, TIMING_LAMP_AUTO_TURN_ON_TASK );
	TURN_LAMP_OFF_TASK = ADD_EVENT_TASK_AUTOFREEZE( EVENT_CODE( TURN_LAMP_OFF_WITH_DELAY_HANDLER ), 0, TIMING_LAMP_AUTO_TURN_ON_TASK );
    SAVE_BRIGHTNESS_TASK = ADD_EVENT_TASK_AUTOFREEZE( EVENT_CODE( SAVE_BRIGHTNESS_VALUE_HANDLER ), 0, TIMING_SAVE_BRIGHTNESS_TASK );

    TS_start();

    //Tasks adding and initialization

    while ( 1 )
    {
        TS_dispatchTasks();
        E_dispatchEvents();
    }

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


#if LCD_ATTACHED == TRUE
void printToLCDAt( uint8_t row, uint8_t col, const uint8_t *str )
{
    moveCursorTo( row, col );
    printStr( str );
}
#endif

/************************************************************************/
/* The Brightness processor code                                        */
/************************************************************************/


void updateSystemStateFromEEPROM( void )
{
    lampSettings.systemState.state = eeprom_read_byte( &EEPROM_status );
    lampSettings.savedBrightnessIndex = eeprom_read_byte( &EEPROM_lastBrightnessIndex );
}

void saveSystemStateInEEPROM( void )
{
    eeprom_write_byte( &EEPROM_lastBrightnessIndex, lampSettings.systemState.state );
}

void saveBrightnessInEEPROM( uint8_t brightnessIndex )
{
    lampSettings.savedBrightnessIndex = brightnessIndex;
    eeprom_write_byte( &EEPROM_lastBrightnessIndex, brightnessIndex );
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

/**
 * #define INCREASE_LIGHT 1
 * #define DECREASE_LIGHT 2
 * #define STOP_LIGHT 3
 * #define START_LIGHT 4
 */
void onManualAction( uint8_t actionType )
{
    TS_delayTask( SAVE_BRIGHTNESS_TASK, ( TIMING_SECOND_IN_TICKS * 5 ) ); //5 sec delay
    UNFREEZE_TASK( SAVE_BRIGHTNESS_TASK );

    lampSettings.systemState.userInputSinceLastAuto = TRUE;
    lampSettings.secondsSinceLastManual = 0;

    switch ( actionType )
    {
    case INCREASE_LIGHT:
        if ( brightnessSystemState.brightnessIndex > ( maxLightIndex/2 ) )
        {
            lampSettings.manualActionState = MANUAL_ACTION_LIGHTUP;
        }
	if( brightnessSystemState.status.maxBrightnessReached )
	{
		indicatorBlink( 3 );
	}		
        FREEZE_TASK( TURN_LAMP_OFF_TASK );
        break;
    case DECREASE_LIGHT:
	if ( brightnessSystemState.brightnessIndex < ACTION_TRESHOLD )
	{
		lampSettings.manualActionState = MANUAL_ACTION_STOP;
	}
        FREEZE_TASK( TURN_LAMP_ON_TASK );
        break;
    case START_LIGHT:
        lampSettings.manualActionState = MANUAL_ACTION_LIGHTUP;
        FREEZE_TASK( TURN_LAMP_OFF_TASK );
        break;
    case STOP_LIGHT:
        lampSettings.manualActionState = MANUAL_ACTION_STOP;
        FREEZE_TASK( TURN_LAMP_ON_TASK );
        break;
    default:
        lampSettings.manualActionState = MANUAL_ACTION_NEUTRAL;
        break;
    }
}

void onAutoAction( uint8_t actionType )
{
    //define the manual action as neutral
    lampSettings.manualActionState = MANUAL_ACTION_NEUTRAL;
    lampSettings.systemState.userInputSinceLastAuto = FALSE;
    lampSettings.secondsSinceLastAuto = 0;

    switch ( actionType )
    {
    case AUTO_ACTION_LIGHTUP:
		indicatorStopPlay();
        FREEZE_TASK( TURN_LAMP_OFF_TASK );
        break;
    case AUTO_ACTION_STOP:
		indicatorPlay();
        FREEZE_TASK( TURN_LAMP_ON_TASK );
        break;
    }
}
