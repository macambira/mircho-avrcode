#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "eventcodes.h"
#include "util.h"
#include "indicator.h"
#include "event.h"
#include "tick.h"

#define USE_HARDWARE_PWM	TRUE

#if		USE_HARDWARE_PWM == FALSE
#include "led.h"
#elif	USE_HARDWARE_PWM == TRUE
#define LEDOFF	(portb.port3 = OFF);
#define LEDON	(portb.port3 = ON);
#endif


const prog_uint8_t pwmValuesForIndicator[] = {0, 0, 1,1,2,2,3,4,5,7,9,12,16,21,28,37,48,64,84,111,147,193,255,255,255,255,255,255,255,255,255,255,255,255,218,186,159,135,116,99,84,72,61,52,45,38,33,28,24,20,17,15,13,11,9,8,7,6,5,4,4,3,3,2,2,2,1,1,1,0};
//uint8_t pwmValuesForIndicator[] PROGMEM = {0, 0, 1,1,2,2,3,4,5,7,9,12,16,21,28,37,48,64,84,111,147,193,255,255,255,255,255,255,255,255,255,255,255,255,218,186,159,135,116,99,84,72,61,52,45,38,33,28,24,20,17,15,13,11,9,8,7,6,5,4,4,3,3,2,2,2,1,1,1,0};
#define SIZE_OF_PWM_INDICATOR_VALUES		( NUMBEROFELEMENTS( pwmValuesForIndicator ) )
#define MAX_PWM_INDICATOR_INDEX			( ( NUMBEROFELEMENTS( pwmValuesForIndicator ) ) - 1 )

static uint8_t PWM_INDICATOR_TASK;
#define TIMING_PWM_INDICATOR_CYCLE		( TIMING_MILLISECOND_IN_TICKS * 32 )

static uint8_t BLINK_TASK;
#define TIMING_BLINK_CYCLE				( TIMING_MILLISECOND_IN_TICKS * 50 )

#define CYCLES_PER_BLINK					8

#define TIMER_PRESCALE	(_BV( CS22 ) | _BV( CS20 ))  //prescale //128

//#define TIMER_MODE_OF_OPERATION (_BV( WGM21 ) | _BV( WGM20 )) //fast PWM
#define TIMER_MODE_OF_OPERATION (_BV( WGM20 )) //Phase correct PWM

#define TIMER_INTERRUPT_SETUP (_BV( OCIE2 ) | _BV( TOIE2 ))

typedef enum _play_state
{
	INDICATOR_PLAYING,
	INDICATOR_PAUSED
} tPlayState;

typedef enum _blink_state
{
	INDICATOR_BLINKING,
	INDICATOR_NOT_BLINIKNG
} tBlinkState;

struct 
{
	tPlayState	playState;
	tBlinkState blinkState;
	uint8_t		blinkCount;
	uint8_t		blinkIndex;
	uint8_t		pwmIndicatorIndex;
} indicatorState;

//Indicator Led setup
void indicatorSetupPort( void )
{
	//this is for the time we switch off the SPI
	#if USE_HARDWARE_PWM == TRUE
    portb.port3 = OFF; //0
	ddrb.dd3 = OUT;
	#else
	#endif
}

DEFINE_EVENT_HANDLER( BLINK_HANDLER )
{
	if( indicatorState.blinkIndex % 2 )
	{
		LEDOFF
		if( indicatorState.blinkIndex / 2 == indicatorState.blinkCount )
		{
			indicatorStopBlink();
		}
	}
	else
	{
		LEDON
	}
	indicatorState.blinkIndex++;
}

DEFINE_EVENT_HANDLER( PWM_INDICATOR_HANDLER )
{
	uint8_t newValue;
	uint8_t task;
	task = eventObject.params.loParam;


	indicatorState.pwmIndicatorIndex++;
	if( indicatorState.pwmIndicatorIndex == MAX_PWM_INDICATOR_INDEX )
	{
		indicatorState.pwmIndicatorIndex = 0;
		TS_delayTask( task, TIMING_SECOND_IN_TICKS * 3 );
	}
	newValue = pgm_read_byte( &pwmValuesForIndicator[ indicatorState.pwmIndicatorIndex ] );
 
	#if USE_HARDWARE_PWM == TRUE
	if( newValue == 0 )
	{
		indicatorDisconnectPWMPin();
		LEDOFF
	}
	else
	{
		indicatorConnectPWMPin();
	}
	#endif

	OCR2 = newValue;
}

void indicatorSetupPWMTimer( void )
{
	TCCR2	= 0;
	TCNT2	= 0;
	OCR2	= 0;
	
	TCCR2	|= TIMER_MODE_OF_OPERATION;
	TCCR2	|= TIMER_PRESCALE;
	
	
	#if		USE_HARDWARE_PWM == FALSE
	//setup interrupts for software driving the LED
	TIMSK	|= TIMER_INTERRUPT_SETUP;
	#endif
}


extern void indicatorInit( void )
{
	indicatorSetupPort();
	indicatorSetupPWMTimer();

	#if		USE_HARDWARE_PWM == TRUE
	indicatorConnectPWMPin();
	#endif

	indicatorState.blinkState = INDICATOR_NOT_BLINIKNG;
	indicatorState.playState = INDICATOR_PAUSED;

	REGISTER_EVENT_HANDLER( PWM_INDICATOR_HANDLER )
	PWM_INDICATOR_TASK = ADD_EVENT_TASK( EVENT_CODE( PWM_INDICATOR_HANDLER ), 0, TIMING_PWM_INDICATOR_CYCLE );

	REGISTER_EVENT_HANDLER( BLINK_HANDLER )
	BLINK_TASK = ADD_EVENT_TASK( EVENT_CODE( BLINK_HANDLER ), 0, TIMING_BLINK_CYCLE );
	FREEZE_TASK( BLINK_TASK );
}


void indicatorStopTimer( void )
{
	TCCR2	&= ~( TIMER_PRESCALE );
}

void indicatorStartTimer( void )
{
	TCNT2	 = 0;
	TCCR2	|= TIMER_PRESCALE;
}

void indicatorConnectPWMPin( void )
{
	TCCR2 |= _BV( COM21 );	
}

void indicatorDisconnectPWMPin( void )
{
	TCCR2 &= ~_BV( COM21 );	
}

void indicatorPause( void )
{
	indicatorDisconnectPWMPin();
	indicatorStopTimer();
	LEDOFF
}

extern void indicatorPlay( void )
{
	if( indicatorState.playState == INDICATOR_PLAYING )
	{
		return;
	}
	indicatorConnectPWMPin();
	indicatorStartTimer();
	indicatorState.pwmIndicatorIndex = 0;
	indicatorState.playState = INDICATOR_PLAYING;
	UNFREEZE_TASK( PWM_INDICATOR_TASK );
}

extern void indicatorStopPlay( void )
{
	indicatorPause();
	indicatorState.pwmIndicatorIndex = 0;
	indicatorState.playState = INDICATOR_PAUSED;
	FREEZE_TASK( PWM_INDICATOR_TASK );
}


extern void indicatorBlink( uint8_t times )
{
	indicatorPause();
	if( indicatorState.blinkState == INDICATOR_BLINKING )
	{
		return;
	}
	indicatorState.blinkIndex = 0;
	indicatorState.blinkCount = times;
	indicatorState.blinkState = INDICATOR_BLINKING;
	UNFREEZE_TASK( BLINK_TASK );
}

void indicatorStopBlink( void )
{
	indicatorState.blinkIndex = 0;
	indicatorState.blinkCount = 0;
	indicatorState.blinkState = INDICATOR_NOT_BLINIKNG;
	FREEZE_TASK( BLINK_TASK );	
}


#if		USE_HARDWARE_PWM == FALSE

ISR( TIMER2_COMP_vect )
{
	LEDOFF
}

ISR( TIMER2_OVF_vect )
{
	LEDON
}

#endif
