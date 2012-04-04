/*
 * brightness.c
 *
 * Created: 10.3.2011 г. 16:07:56
 *  Author: mmirev
 */ 
#include <string.h>
#include <avr/pgmspace.h>

#include "eventcodes.h"
#include "brightness.h"
#include "util.h"
#include "event.h"
#include "tick.h"
#include "encoder.h"


#define TIMING_PROCESS_QUEUE_TASK			(TIMING_MILLISECOND_IN_TICKS * 16)
#define TIMING_BRIGHTNESS_CHANGE_TASK			TIMING_PROCESS_QUEUE_TASK + 15

/**
 * Some comments are available here:
 * http://web.cecs.pdx.edu/~gerry/class/EAS199A/topics/pdf/breathing_LED_equations.pdf
 *
 * WolframAplpha :
 *
 * Ramp up : Table[{N[x],round(10*Exp[0.23*x])},  {x, 0, 20,  1}]
 * Ramp down: Table[{N[x],round(1000*Exp[-0.13*x])},  {x, 0, 35,  1}]
 *
 *
 */


/**
 * Other 128 values from 0 to 1024 (10 bit)
 * as seen at http://catmacey.wordpress.com/2010/10/20/updated-code-for-simple-pwm-led-controller/
 */

//[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19, 21, 22, 24, 25, 27, 28, 30, 32, 33, 35, 37, 39, 41, 43, 45, 47, 49, 52, 54, 56, 59, 61, 64, 66, 69, 72, 75, 78, 81, 84, 87, 91, 94, 98, 101, 105, 109, 113, 117, 121, 125, 130, 134, 139, 144, 149, 154, 159, 165, 170, 176, 182, 188, 194, 200, 207, 214, 221, 228, 235, 243, 250, 258, 267, 275, 284, 293, 302, 311, 321, 331, 342, 352, 363, 374, 386, 398, 410, 422, 435, 449, 462, 476, 491, 506, 521, 537, 553, 570, 587, 605, 623, 641, 661, 680, 701, 721, 743, 765, 788, 811, 835, 860, 885, 912, 939, 966, 995, 1023]

//64 steps, 10 bits of resolution
const prog_uint16_t ledFadePWMValues[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 16, 18, 20, 22, 25, 28, 31, 34, 37, 41, 45, 49, 53, 58, 63, 69, 75, 81, 88, 95, 102, 111, 120, 129, 139, 150, 162, 174, 187, 201, 217, 233, 250, 269, 289, 311, 334, 358, 384, 413, 443, 475, 510, 547, 586, 629, 674, 723, 775, 831, 891, 955, 1023};
//const uint16_t ledFadePWMValues[] PROGMEM = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 16, 18, 20, 22, 25, 28, 31, 34, 37, 41, 45, 49, 53, 58, 63, 69, 75, 81, 88, 95, 102, 111, 120, 129, 139, 150, 162, 174, 187, 201, 217, 233, 250, 269, 289, 311, 334, 358, 384, 413, 443, 475, 510, 547, 586, 629, 674, 723, 775, 831, 891, 955, 1023};
#define SIZE_OF_PWM_VALUES		( NUMBEROFELEMENTS( ledFadePWMValues ) )
#define MAX_PWM_INDEX			( ( NUMBEROFELEMENTS( ledFadePWMValues ) ) - 1 )

#define BLINK_INDEX			( MAX_PWM_INDEX - 5 )
	
/*
 * Queue with the next brightness values
 * The queue contains bytes that contain encoded brightness information
 */
#define BRIGHTNESS_FROM_BD_WORD( bdword ) (((brightVal)bdword).brightness)
#define TIME_FROM_BD_WORD( bdword ) (((brightVal)bdword).delay)

bQueue brightnessQueue = { 0, 0, 0 };

bsState brightnessSystemState = { 0, 0, 0 };

uint8_t BRIGHTNESS_CHANGE_TASK;

uint8_t getSizeOfPWMValuesArray( void )
{
	return SIZE_OF_PWM_VALUES;
}

uint8_t getMaxPWMValuesIndex( void )
{
	return MAX_PWM_INDEX;
}

void addToBrightnessQueue( uint8_t brightness, uint8_t time )
{
	brightVal *currentBVal;
	if( QUEUE_FULL )
	{
		return;
	}

	currentBVal = &brightnessQueue.queue[ brightnessQueue.queueEnd ];
	currentBVal->brightness = brightness;
	currentBVal->delay = time;

	brightnessQueue.queueEnd++;
	brightnessQueue.queueEnd = brightnessQueue.queueEnd % MAX_QUEUE;
	brightnessQueue.queueSize++;
}

uint8_t getFromBrightnessQueue( brightVal *result )
{
	brightVal bdValue;

	if( QUEUE_EMPTY )
	{
		return 0;
	}

	bdValue = brightnessQueue.queue[ brightnessQueue.queueStart ];
	memcpy( result, &bdValue, sizeof( bdValue ) );

	brightnessQueue.queueStart++;
	brightnessQueue.queueStart = brightnessQueue.queueStart % MAX_QUEUE;
	brightnessQueue.queueSize--;

	return 1;
}

void emptyQueue( void )
{
	brightnessQueue.queueStart = brightnessQueue.queueEnd = 0;
	brightnessQueue.queueSize = 0;
}

void incBrightnessIdx( uint8_t delay )
{
	uint8_t bI;
	bI = brightnessSystemState.brightnessIndex;
	if( bI == MAX_PWM_INDEX )
	{
	}
	else
	{
		bI++;
		addToBrightnessQueue( bI, delay );
	}
	brightnessSystemState.status.maxBrightnessReached = ( bI == MAX_PWM_INDEX ? TRUE : FALSE );
	brightnessSystemState.status.increasing = TRUE;
}

void decBrightnessIdx( uint8_t delay )
{
	uint8_t bI;
	bI = brightnessSystemState.brightnessIndex;
	if( bI == 0 )
	{
	}
	else
	{
		bI--;
		addToBrightnessQueue( bI, delay );
	}
	brightnessSystemState.status.maxBrightnessReached = ( bI == MAX_PWM_INDEX ? TRUE : FALSE );
	brightnessSystemState.status.increasing = FALSE;
}

void setBrightnessIdx( uint8_t brightnessIndex, uint8_t delay )
{
	if( ( brightnessIndex >= 0 ) && ( brightnessIndex <= MAX_PWM_INDEX ) )
	{
		addToBrightnessQueue( brightnessIndex, delay );	
	}	
}

void setBrightnessDummy( void )
{
	uint8_t bI = brightnessSystemState.brightnessIndex;
}

void setBrightnessByIndex( void )
{
	uint8_t brightnessIndex;
	uint16_t brightnessValue;
	brightnessIndex = brightnessSystemState.brightnessIndex;
	brightnessValue = GetLedPwmValueByIndex( brightnessIndex );
	
	OCR1A = brightnessValue;
	OCR1B = brightnessValue;

	//RAISE_EVENT( BRIGHTNESS_CHANGE, EVENT_PARAM( 0, brightnessIndex ) )
	UNFREEZE_TASK( BRIGHTNESS_CHANGE_TASK );
	TS_delayTask( BRIGHTNESS_CHANGE_TASK, TIMING_BRIGHTNESS_CHANGE_TASK );
}

DEFINE_EVENT_HANDLER( PROCESS_QUEUE_HANDLER )
{
	brightVal *currentQValue;
	if( brightnessSystemState.repeats == 0 )
	{
		if( !QUEUE_EMPTY )
		{
			currentQValue = &brightnessQueue.queue[ brightnessQueue.queueStart ];
			brightnessQueue.queueStart++;
			brightnessQueue.queueStart = brightnessQueue.queueStart % MAX_QUEUE;
			brightnessQueue.queueSize--;
			
			brightnessSystemState.brightnessIndex = currentQValue->brightness;
			brightnessSystemState.repeats = currentQValue->delay - 1;
			
			setBrightnessByIndex();
		}
		else
		{
		}
	}
	else
	{
		brightnessSystemState.repeats--;
	}
}

extern void insertBrightnessRange( uint8_t brightnessIndexStart, uint8_t brightnessIndexEnd )
{
    uint8_t step;
	if( brightnessIndexStart == brightnessIndexEnd )
	{
		addToBrightnessQueue( brightnessIndexStart, 1 );
		return;
	}
	else if( brightnessIndexStart > brightnessIndexEnd )
	{
		step = -1;
	}
	else
	{
		step = 1;
	}
	
	do
	{
		brightnessIndexStart += step;
		addToBrightnessQueue( brightnessIndexStart, 1 );
	}
	while( brightnessIndexStart != brightnessIndexEnd );
}

/**
 * PWM for the power leds
 */
void setupPWMTimer( void )
{
	TCCR1A	= 0;
	TCCR1B	= 0;
	OCR1A	= 0;
	OCR1B	= 0;
	
	//on on compare
	TCCR1A	|= _BV( COM1A1 ) | _BV( COM1B1 );
	//PWM, Phase Correct, 10-bit
	TCCR1A	|= _BV( WGM11 ) | _BV( WGM10 );
	
	//TCCR1B |= _BV( CS10 );		//no prescaling
	TCCR1B	|= _BV( CS11 );			//prescale 8
}


void disconnectPWMPins( void )
{
	TCCR1A &= ~( _BV( COM1A1 ) | _BV( COM1B1 ) );
}

void connectPWMPins( void )
{
	TCCR1A |= _BV( COM1A1 ) | _BV( COM1B1 );
}

//setup the PWM pins to be output
void setupPWMPort( void )
{
	portb.port1		= OFF; //0
    portb.port2		= OFF; //0
	ddrb.dd1		= OUT;
	ddrb.dd2		= OUT;
}

/**
 * End of PWM
 */

extern void brightnessInit( void )
{
	setupPWMTimer();
	connectPWMPins();	
	setupPWMPort();

	brightnessSystemState.brightnessIndex = 0;
	brightnessSystemState.status.maxBrightnessReached = FALSE;

	REGISTER_EVENT_HANDLER( PROCESS_QUEUE_HANDLER );
	ADD_EVENT_TASK( EVENT_CODE( PROCESS_QUEUE_HANDLER ), 0, TIMING_PROCESS_QUEUE_TASK );

	BRIGHTNESS_CHANGE_TASK = ADD_EVENT_TASK_AUTOFREEZE( EVENT_CODE( BRIGHTNESS_CHANGE ), TIMING_BRIGHTNESS_CHANGE_TASK, TIMING_BRIGHTNESS_CHANGE_TASK );
}
