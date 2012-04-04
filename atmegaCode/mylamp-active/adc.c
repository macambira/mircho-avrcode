/*
 * adc.c
 *
 * Created: 29.3.2011 г. 15:54:10
 *  Author: mmirev
 */
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "util.h"
#include "event.h"
#include "tick.h"
#include "adc.h"

#define TIMING_TOUCH_PIN_CHARGE_TASK			(TIMING_MILLISECOND_IN_TICKS * 3)
#define TIMING_TOUCH_PIN_SLEEP_TASK			(TIMING_MILLISECOND_IN_TICKS * 30)
#define TIMING_ADC_COLLECT_DATA_TASK			(TIMING_MILLISECOND_IN_TICKS * 1200)

#define LIGHT_SENSOR_PHOTOTRANSISTOR FALSE
#define LIGHT_SENSOR_LDR TRUE

#define ADC_BUFFER_SIZE 16	//should be power of 2, other routines are built on that
#define ADC_BUFFER_MASK (ADC_BUFFER_SIZE-1)

#define DVPLUS	2
#define DVMINUS	6

volatile struct _adcbuffer {
	uint8_t adcInitialized;
	uint8_t adcBufferHead;
	uint16_t adcbuffer[ ADC_BUFFER_SIZE ];	
	uint16_t modifiedMovingAverage;
} adcbuffer = {0, 0, 0, 0};

volatile uint8_t resultAvailable = 0;
volatile uint16_t comparatorTime;

extern void adcEnableAndRun( void )
{
	//disable analog comparator
	ACSR	|= _BV( ACD );

	//ADC6 channel (pin 19)
	ADMUX	&= ~_BV(MUX0);
	ADMUX	|= _BV(MUX1);
	ADMUX	|= _BV(MUX2);

	ADCSRA	|= _BV( ADEN ) | _BV( ADSC );
	
	#if ( LIGHT_SENSOR_PHOTOTRANSISTOR == TRUE )
	//power on the photo transistor
	portc.port3 = ON;
	#endif
}

extern void adcDisable( void )
{
	ADCSRA	&= ~( _BV( ADEN ) );

	#if ( LIGHT_SENSOR_PHOTOTRANSISTOR == TRUE )
	//power off the photo transistor
	portc.port3 = OFF;
	#endif
}

//
extern void comparatorCharge( void )
{
	portc.port4 = ON;
}

//timer 1 should be set up in main and the code here depends on a 8 Mhz clock with a prescaler of 8
//so one timer count is equal to 1 microsecond
extern void comparatorEnableAndRun( void )
{
	portc.port4 = OFF;

	//ADC4 channel (pin 27)
	ADMUX	&= ~_BV(MUX0);
	ADMUX	&= ~_BV(MUX1);
	ADMUX	|= _BV(MUX2);
	//disable ADC
	ADCSRA	&= ~( _BV( ADEN ) );
	//enable input capture interrupt for timer 1
	TIMSK	|= _BV( TICIE1 );
	//enable comparator
	ACSR	&= ~( _BV( ACD ) );
}

extern void comparatorDisable( void )
{
	//disable input capture interrupt for timer 1
	TIMSK	&= ~( _BV( TICIE1 ) );
	//disable comparator
	ACSR	|= _BV( ACD );
}

extern uint16_t adcGetLastResult( void )
{
	uint16_t result;
	uint8_t lastLocation;
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{	
		lastLocation = adcbuffer.adcBufferHead - 1;
	}	
	lastLocation &= ADC_BUFFER_MASK;
	result = adcbuffer.adcbuffer[ lastLocation ];
	return result;
}

void adcAddResult( uint16_t result )
{
	uint16_t newMMA;
	uint16_t lastResult;

	if( adcbuffer.adcInitialized == 0 )
	{
		adcbuffer.modifiedMovingAverage = result;
		lastResult = result;
		adcbuffer.adcInitialized = 1;
	}
	else
	{
		lastResult = adcGetLastResult();
	}


	if( ( result < lastResult ) && ( ( lastResult - result ) > DVMINUS ) )
	{
		result = lastResult - DVMINUS;
	}
	else
	if( ( result > lastResult ) && ( ( result - lastResult ) > DVPLUS ) )
	{
		result = lastResult + DVPLUS;
	}

	adcbuffer.adcbuffer[ adcbuffer.adcBufferHead ] = result;
	adcbuffer.adcBufferHead++;
	adcbuffer.adcBufferHead &= ADC_BUFFER_MASK;
	newMMA = ( ( ADC_BUFFER_SIZE - 1 ) * adcbuffer.modifiedMovingAverage + result ) / ADC_BUFFER_SIZE;
	adcbuffer.modifiedMovingAverage = newMMA;
}


extern uint16_t adcGetMovingAverage( void )
{
	return adcbuffer.modifiedMovingAverage;
}

/************************************************************************/
/* TOUCH / ANALOG COMPARATOR                                            */
/************************************************************************/
ISR( TIMER1_CAPT_vect )
{
	comparatorTime = ICR1 - comparatorTime;
	RAISE_EVENT( COMPARATOR_FINISH, comparatorTime )
}

ISR( ADC_vect )
{
	adcAddResult( ADC );
	adcDisable();
}

DEFINE_EVENT_HANDLER( COMPARATOR_FINISH )
//event eventObject as param
{
	uint16_t comparatorTime;
	comparatorTime = eventObject.params.wParam;
	RAISE_EVENT( PIN_TOUCH, comparatorTime )
}

DEFINE_EVENT_HANDLER( TOUCH_PIN_HANDLER )
//event eventObject as param
{
	static uint8_t pinCharged;
	uint8_t task;
	task = eventObject.params.loParam;

	if( pinCharged == 1 )
	{
		comparatorEnableAndRun();
		pinCharged = 0;
		TS_delayTask( task, TIMING_TOUCH_PIN_SLEEP_TASK ); 
	}
	else
	{
		comparatorCharge();
		TS_delayTask( task, TIMING_TOUCH_PIN_CHARGE_TASK ); 
	}
	pinCharged++;
}

DEFINE_EVENT_HANDLER( ADC_RUN_HANDLER )
{
	adcEnableAndRun(); //enable and perform a single conversion
}

extern void adcInit( void )
{
	//init the pin for the comparator as output, it will be used for charging and then comparing
	ddrc.dd4 = OUT;
	//ADC6 is a dedicated ADC pin in TQFP32 Atmega8, so no port and ddr setup is required

	ADCSRA	|= _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

	//AVCC Reference
	ADMUX	&= ~_BV(REFS1);
	ADMUX	|= _BV(REFS0);

	//comparator setup
	ACSR	|= _BV( ACBG ) | _BV( ACIC ) | _BV( ACIS1 ); 

	//enable analog comparator multiplexing
	SFIOR	|= _BV( ACME );
	
	ddrc.dd3 = OUT;
	
	#if ( LIGHT_SENSOR_LDR == TRUE )
	portc.port3 = ON;
	#endif
	
	/*
	REGISTER_EVENT_HANDLER( COMPARATOR_FINISH )
	
	REGISTER_EVENT_HANDLER( TOUCH_PIN_HANDLER )
	ADD_EVENT_TASK( EVENT_CODE( TOUCH_PIN_HANDLER ), 0, TIMING_TOUCH_PIN_SLEEP_TASK )
	*/

	REGISTER_EVENT_HANDLER( ADC_RUN_HANDLER );
	ADD_EVENT_TASK( EVENT_CODE( ADC_RUN_HANDLER ), 0, TIMING_ADC_COLLECT_DATA_TASK );
}