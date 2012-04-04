/*****************************************************************************
*
* Mircho Mirev 2010
*
* Canon-IR remote control
*
****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>

#define TRUE		1
#define FALSE		0

typedef uint8_t bool;
typedef struct _packedBool{
	bool f0:1;
	bool f1:1;
	bool f2:1;
	bool f3:1;
	bool f4:1;
	bool f5:1;
	bool f6:1;
	bool f7:1;
} PackedBool;

#define bLedIsOn ( (volatile PackedBool*) (&USIDR) )->f7
#define bTimeToSleep  ( (volatile PackedBool*) (&USIDR) )->f6
#define bReadAButton  ( (volatile PackedBool*) (&USIDR) )->f5

#define DEBUG_MODE FALSE

int main( void ) __attribute__( ( noreturn ) ) __attribute__( ( OS_main ) );

#define SETBIT(x,y) (x |= (_BV(y))) /* Set bit y in byte x*/
#define CLEARBIT(x,y) (x &= (~_BV(y))) /* Clear bit y in byte x*/
#define CHECKBIT(x,y) (x & (_BV(y))) /* Check bit y in byte x*/

//disable adc to conserve power
#define DISABLE_ADC()	({\
	CLEARBIT(ADCSR,ADEN);\
	CLEARBIT(ACSR,ACD);\
	})
	
//disable wdt to conserve power
#define DISABLE_WDT()	({\
	WDTCR |= _BV( WDCE ) | _BV( WDE );\
	CLEARBIT( WDTCR, WDE );\
	})

//
#define IS_ODD( x ) ( x & 1 )

/**
 *		Timers control
 **/
//timer 0 is used for debouncing
#define NUMBER_OF_CHECKS 4
#define SLEEP_AFTER_CYCLES	50 //this is a little more than a second
#define TIMER_LOAD_VALUE (255-15) //this with a prescaler of 1024 @ 8MHz gives ~520 interrupts per second
#define ENOUGH_CHECKS() (checkCounter>NUMBER_OF_CHECKS)
#define TIME_TO_SLEEP() (checkCounter>SLEEP_AFTER_CYCLES)

volatile uint8_t checkCounter = 0;

#define timer0Running() (TCCR0!=0)
static void startTimer0( void );
static void stopTimer0( void );

#define IIMER_TOGGLE_VALUE		120 //121 ticks
#define TIMER_OVERFLOW_VALUE	241 //242 ticks. this, at no prescaler and 8MHz means 65573 ints in a second
static void startTimer1(void);
static void stopTimer1(void);

volatile uint8_t	tickCounter;
volatile uint8_t	currentPoint;

//tickMark is a point in the counter the timer increases
typedef struct _ctrlPoint {
	uint8_t tickMark;
	uint8_t enableLed;
} controlPoint;


#if DEBUG_MODE == TRUE
controlPoint timingPointsDelayed[] = {
	{2, 1},
	{0, 0}
};

controlPoint timingPointsImmediate[] = {
	{8, 1},	//end of cycle
	{4, 0},
	{2, 1},
	{0, 0}
};

#else
controlPoint timingPointsDelayed[] = {
	{100,0},
	{16, 1},
	{178, 0},
	{16, 1},
	{0, 0}
};

controlPoint timingPointsImmediate[] = {
	{100, 0},
	{16, 1},
	{244, 0},
	{16, 1},
	{0, 0}
};
#endif


volatile controlPoint *currentTimingPoints;
/**
 *		Timers control
 **/

/**
 * Led control
 *
 **/
#define LED_PORT PORTB
#define LED_DDR DDRB

#define LED_PIN_TOGGLE PB1
#define LED_PIN_ENABLE PB2

#define LED_ON()  ({\
	LED_PORT |= _BV( LED_PIN_TOGGLE );\
})

#define LED_OFF()  ({\
	LED_PORT &= ~( _BV( LED_PIN_TOGGLE ) );\
})

#define LED_ENABLE() ({\
	LED_PORT &= ~_BV( LED_PIN_ENABLE );\
})

#define LED_DISABLE() ({\
	LED_PORT |= _BV( LED_PIN_ENABLE );\
})
/**
 * Led control
 *
 **/

/**
 * Button states
 *
 */
#define BUTTON_PORT PORTA
#define BUTTON_PIN_PORT PINA
#define BUTTON_DDR DDRA
#define BUTTON_IMMEDIATE_PIN PA7
#define BUTTON_DELAYED_PIN PA6

static void initButtons( void );

#define BUTTON_IMMEDIATE_PRESSED() (getKey(_BV(BUTTON_IMMEDIATE_PIN)))
#define BUTTON_DELAYED_PRESSED() (getKey(_BV(BUTTON_DELAYED_PIN)))

volatile uint8_t input_state;
volatile uint8_t input_press;
static uint8_t getKey( uint8_t key_mask );
/**
 * Button states
 *
 */



int main( void )
{
	USIDR = 0;

	//because I try to use PA6 and PA7 I have to disable the analog comparator
	ACSR |= _BV(ACD); 
	//ADMUX |= _BV( MUX3 ) | _BV( MUX2 );
	//diabling ADC (to conserve power) makes it hard to read states on PA6 and PA7
	//DISABLE_ADC();

	DISABLE_WDT();
	
	//init the LED, define LED_PIN_TOGGLE, LED_PIN_ENABLE outputs
	//LED_PIN_TOGGLE source current, and LED_PIN_ENABLE sinks it
	LED_DDR      |= _BV( LED_PIN_TOGGLE ) | _BV( LED_PIN_ENABLE );	
	initButtons();
	//PORTA  	  =  0b11111111;
	//PORTB  	  =  0b11111111;

	bTimeToSleep = 1;
	bReadAButton = 0;

	//init the timer1 overflow to a limiting value
	OCR1A	 = IIMER_TOGGLE_VALUE;
	OCR1C 	 = TIMER_OVERFLOW_VALUE;
	
	//use pin change interrupt to wake up from sleep
	GIMSK |=  _BV( PCIE1 );
	GIFR   =  _BV( PCIF );

	sei();																	// Enable global interrupts
	set_sleep_mode( SLEEP_MODE_PWR_DOWN );									// Go to sleep immediately

	currentTimingPoints = timingPointsImmediate;
 
	for( ; ; ) {															// Run forever


		cli();
		if( bTimeToSleep )
		{
			GIFR   =  _BV( PCIF );
			sleep_enable();
			sei();
			sleep_cpu();
			sleep_disable();
			
			bTimeToSleep = FALSE;
			bReadAButton = TRUE;
			checkCounter = 0;											//timer0 ticks while checking the buttons for debounce
			startTimer0();
		}
		sei();
		
		if( bReadAButton )
		{
			//wait for enough checks of the buttons
			while(!ENOUGH_CHECKS()){}
			stopTimer0();

			if( BUTTON_IMMEDIATE_PRESSED() )
			{
				currentTimingPoints = timingPointsImmediate;
			}
			else if( BUTTON_DELAYED_PRESSED() )
			{
				currentTimingPoints = timingPointsDelayed;
			}
			else
			{
				bTimeToSleep = TRUE;
			}

			stopTimer0();
			bReadAButton = FALSE;

			if( !bTimeToSleep )
			{
				currentPoint = 0;
				tickCounter = currentTimingPoints[ currentPoint ].tickMark;
				if( currentTimingPoints[ currentPoint ].enableLed )
				{
					LED_ENABLE();
				}
				startTimer1();													// The timer starts and the ISR takes over;
				LED_ON();
			}
		}
		
		if( tickCounter == 0 )
		{
			currentPoint++;
			
			if( currentTimingPoints[ currentPoint ].tickMark == 0 )
			{
				stopTimer1();
				LED_OFF();
				LED_DISABLE();
				bTimeToSleep = TRUE;
			}
			else
			{
				tickCounter = currentTimingPoints[ currentPoint ].tickMark;
				if( currentTimingPoints[ currentPoint ].enableLed )
				{
					LED_ENABLE();
				}
				else
				{
					LED_DISABLE();
				}
			}

		}

#if DEBUG_MODE == TRUE
#endif

	}

}

static void initButtons( void )
{
	BUTTON_DDR   &= ~( _BV( BUTTON_IMMEDIATE_PIN ) | _BV( BUTTON_DELAYED_PIN ) );
	BUTTON_PORT  |= _BV( BUTTON_IMMEDIATE_PIN ) | _BV( BUTTON_DELAYED_PIN );
}

static void startTimer1(void)
{
	TCNT1	 = 0;
	//toggle OC1A output line
	TCCR1A 	|= _BV(COM1A0);
#if DEBUG_MODE == TRUE
	TCCR1B 	|= _BV( CTC1 ) | _BV( CS13 ) | _BV( CS12 ) |  _BV( CS11 ) | _BV( CS10 );
#else
	TCCR1B 	|= _BV( CTC1 ) | _BV( CS10 );
#endif
	TIMSK 	|= _BV( TOIE1 );
}

static void stopTimer1(void)
{
	TIMSK	&= ~_BV( TOIE1 );
	TCCR1A	 = 0;
	TCCR1B	 = 0;
}

ISR( TIMER1_OVF1_vect )
{
	//force output compare, which in turn forces toggling the OC1A pin
	TCCR1A |= _BV( FOC1A );

	if( tickCounter )
	{
		--tickCounter;
	}
}


static void startTimer0( void )
{
	TCNT0	 = TIMER_LOAD_VALUE;
	TCCR0 	 = _BV( CS02 ) | _BV( CS00 );
	TIMSK 	|= _BV( TOIE0 );
}

static void stopTimer0( void )
{
	TIMSK	&= ~_BV( TOIE1 );
	TCCR0	 = 0;
}

// more at
// http://www.friday.com/bbum/2008/04/05/using-a-vertical-stack-counter-to-debounce-switches/
// http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=33821

ISR( TIMER0_OVF0_vect )
{
	static uint8_t ct0 = 0xFF, ct1 = 0xFF;
	uint8_t i;

	i	  = ~BUTTON_PIN_PORT;				// read keys (low active) 
	i	 ^=  input_state;					// key changed ? 
	ct0	  =  ~( ct0 & i );					// reset or count ct0 
	ct1   = ct0 ^ (ct1 & i );				// reset or count ct1 
	i	 &= ct0 & ct1;						// count until roll over ? 
	input_state ^= i;						// then toggle debounced state 
	input_press |= input_state & i;			// 0->1: key press detect
	
	TCNT0	= TIMER_LOAD_VALUE;
	checkCounter++;
}

static uint8_t getKey( uint8_t key_mask )
{
	ATOMIC_BLOCK(ATOMIC_FORCEON)   			// read and clear atomic!
	{
		key_mask &= input_press;			// read key(s)
		input_press ^= key_mask;			// clear key(s)
	}
	return key_mask; 
}


EMPTY_INTERRUPT( IO_PINS_vect )

/*
ISR( IO_PINS_vect )
{
	LED_ENABLE();
	LED_ON();
	_delay_ms( 100 );
	LED_OFF();
	LED_DISABLE();
}
*/
