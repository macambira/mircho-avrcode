#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <hd44780/hd44780.h>

#include "hd44780_settings.h"

#define LCD_AVAILABLE 1

#include "main.h"

#define SETBIT(x,y) (x |= (_BV(y))) /* Set bit y in byte x*/
#define CLEARBIT(x,y) (x &= (~_BV(y))) /* Clear bit y in byte x*/
#define CHECKBIT(x,y) (x & (_BV(y))) /* Check bit y in byte x*/

#define BITFIELD_REG	USIDR
#define CONSUMER_ENABLED REGISTER_BIT(BITFIELD_REG, 0)

/**
 * Timers control
 **/
static void initTimer0(void);
#define startTimer0() ({\
	SETBIT( TIMSK, TOIE0 );\
	})
volatile uint8_t Timer0Counter0;
volatile uint8_t Timer0Counter1;

static inline void waitTimerTicks( uint8_t ticks ) __attribute__((always_inline));
#define WAIT_UNTIL(x) do{}while(x)

#define WAIT_TIMER_TICKS( x ) ({\
		Timer0Counter0 = x;\
		do{}while(Timer0Counter0>0);\
	})
/*
#define WAIT_TIMER_TICKS( x ) waitTimerTicks( x )
*/


static void initTimer1(void);
#define startTimer1() ({\
	SETBIT( TIMSK, TOIE1 );\
	})
#define stopTimer1() ({\
	CLEARBIT( TIMSK, TOIE1 );\
	})
#define isTimer1Running() ({\
	CHECKBIT( TIMSK, TOIE1 );\
	})
/**
 * Timers control
 **/

/**
 * Software PWM Channels
 **/

#define PWM_DDR DDRB
#define PWM_PORT PORTB

typedef struct _softPWMSettings
{
	uint8_t *pLedDDR;
	uint8_t	*pLedPort;
	uint8_t bitmask;
} pwmSettings;

pwmSettings SoftPWMSettings = { (uint8_t *)&PWM_DDR, (uint8_t *)&PWM_PORT, 0 };

//max 8 channels, code is designed that way
#define SOFTPWMCHANNELSNUM	5
//make it the power of 2
//with a prescaler of only 1 it will be too fast
#define PWMPRESCALER 0b00000010

typedef struct _pwmChannel
{
	uint8_t ledPin;
	uint8_t pwmPosition;
	uint8_t prescale;
} pwmChannel;

pwmChannel myLeds[SOFTPWMCHANNELSNUM] = {
	{ PB0, 0, PWMPRESCALER },
	{ PB1, 0, PWMPRESCALER },
	{ PB4, 0, PWMPRESCALER },
	{ PB2, 0, PWMPRESCALER },
	{ PB3, 0, PWMPRESCALER }
};

static void initChannels( void );
static void startFade( uint8_t channel );
static void stopFade( uint8_t channel );
static void offChannel( uint8_t channel );

#define turnOnAllActive() ( PWM_PORT|=SoftPWMSettings.bitmask )
#define isActiveChannel(x) ( SoftPWMSettings.bitmask & _BV( myLeds[ x ].ledPin ) )
#define hasActiveChannels(x) ( SoftPWMSettings.bitmask > 0 )
 
/**
 * Software PWM Channels
 *
 **/
 
/**
 *
 * ADC
 *
 **/

#define ADC_Light 5
#define ADC_Temp 6 

#define REF_VOLTAGE 266 //have to detect it for each chip

#define ADC_PINS 11
static void initADC( void );
uint16_t getADCReading( uint8_t channel );

uint16_t adcResult;

/**
 *
 * ADC
 *
 **/

/**
 * Temperature ranges
 **/
uint16_t tempRanges[] = {
	262,	265, //18
	266,	269,
	269,	273,
	273,	277,
	277,	281,
	281,	284,
	285,	288,
	289,	292,
	293,	296,
	296,	300,
	300,	304 //28
};
/**
 * Temperature ranges
 **/

 
#define DELAY_BETWEEN_CHANNELS 40

int main( void ) __attribute__( ( noreturn ) ) __attribute__( ( OS_main ) );

#if (LCD_AVAILABLE==1)
uint8_t sec_str[10];
uint8_t pad;
uint8_t padcounter;
#endif 

int main( void )
{
#if (LCD_AVAILABLE==1)
	lcd_init();
	lcd_clrscr();
	lcd_command( ( _BV(LCD_DISPLAYMODE) | _BV(LCD_DISPLAYMODE_ON) ) & ~(_BV(LCD_DISPLAYMODE_CURSOR)|_BV(LCD_DISPLAYMODE_BLINK)) );
#endif

#if (LCD_AVAILABLE==1)
	lcd_clrscr();
	lcd_home();
	lcd_puts( "TEMP" );
	lcd_goto( 0x40 );
#endif

	initADC();

	CONSUMER_ENABLED = 1;
	
	DDRB = 0xFF;
	PORTB = 0x00;

	initChannels();
	
	static uint8_t debugVal = 0;

	initTimer1();
	startTimer1();

	initTimer0();
	startTimer0();

	sei();

	startFade( 0 );
	WAIT_TIMER_TICKS( 50 );
	startFade( 1 );
	WAIT_TIMER_TICKS( 50 );
	/*
	startFade( 2 );
	WAIT_TIMER_TICKS( 50 );
	startFade( 3 );
	WAIT_TIMER_TICKS( 50 );
	startFade( 4 );
	WAIT_TIMER_TICKS( 50 );
	*/

	WAIT_TIMER_TICKS( 200 );


	for(;;)
	{
		if( Timer0Counter1 == 0 )
		{
			Timer0Counter1 = DELAY_BETWEEN_CHANNELS;
			
			startFade( debugVal++ );
			
			adcResult = getADCReading( ADC_Temp );

			#if (LCD_AVAILABLE==1)
				lcd_goto( 0x40 );
				itoa( adcResult, sec_str, 10 );
				pad = 4 - strlen( sec_str );
				for( padcounter = 0; padcounter < pad; padcounter++ ) lcd_putc( '0' );
				lcd_puts( (char *)sec_str );
			#endif
		
			if( debugVal == 5 )
			{
				debugVal = 0;
			}
		}
	}
}


static void initTimer0(void)
{
	TCNT0 = 178;
	TCCR0 |= _BV( CS02 ) | _BV( CS00  );
}

ISR( TIMER0_OVF0_vect )
{
	uint8_t elapsed;
	if( Timer0Counter0 ) Timer0Counter0--;
	if( Timer0Counter1 ) Timer0Counter1--;
	elapsed = TCNT0;
	TCNT0 = 178 + elapsed;
}

static inline void waitTimerTicks( uint8_t ticks )
{
	Timer0Counter0 = ticks;
	do{}while(Timer0Counter0>0);
}

/**
 * With those values of prescaler and overflow value we have a ~60Hz software PWM
 *
 **/
static void initTimer1(void)
{
	/*
	// Setup PLL
	PLLCSR = _BV(PLLE);	// Enable PLL
	while( !( PLLCSR & _BV( PLOCK ) ) );	// Wait for PLL to lock
	PLLCSR |= _BV(PCKE);	// Change the timer1 clock source to the PLL (64 MHz) 
	*/

	TCCR1A = 0;

	//clear on compare match
	SETBIT( TCCR1B, CTC1 );
	
	OCR1C = SOFTPWM_OCR;

	//set prescaler to 256 and start the timer
	TCCR1B |= SOFTPWM_PRESCALER;
}


//ISR that processes when overflow occurs
ISR( TIMER1_OVF1_vect )
{
	static uint8_t cycle = 0xff;
	static pwmSettings *pwms = &SoftPWMSettings;
	static uint8_t *pwmValues = (uint8_t *)ledFadePWMValues;
	static pwmChannel *pwmLeds = (pwmChannel *)myLeds;
	uint8_t i;
	
	if( !hasActiveChannels() )
	{
		return;
	}

 	if( ++cycle == 0 )	
	{
		//*pwms->pLedPort |= pwms->bitmask;
		turnOnAllActive();
		//PWM_PORT |= pwms->bitmask;
	}
	else
	{
		for( i = 0; i < SOFTPWMCHANNELSNUM; i++ )
		{
			//we know (and made sure) that an inactive channel has a pwmvalue == 0
			//so no inactive channels will be updated here
			if( pwmValues[ pwmLeds[ i ].pwmPosition ] == cycle )
			{
				//offChannel( i );
				CLEARBIT( *pwms->pLedPort, myLeds[ i ].ledPin );
				if( myLeds[ i ].prescale == 0 )
				{
					if( ++pwmLeds[ i ].pwmPosition == ( PWMValuesLength - 1 ) )
					{
						//myLeds[ i ].pwmPosition = 0;
						CLEARBIT( pwms->bitmask, myLeds[ i ].ledPin );						
					}
					else
					{
						pwmLeds[ i ].prescale = PWMPRESCALER;		
					}
				}
				//else
				{
					pwmLeds[ i ].prescale >>= 1;
				}
			}
		}		
	}
}

/**
 * Software PWM Channels
 *
 **/
static void initChannels( void )
{
	uint8_t i;
	uint8_t bitmask = 0;

	static pwmSettings *pwms = &SoftPWMSettings;

	for( i = 0; i < SOFTPWMCHANNELSNUM; i++ )
	{
		bitmask |= _BV( myLeds[ i ].ledPin );
	}
	*pwms->pLedDDR |= bitmask;
}

static void startFade( uint8_t channel )
{
	static pwmSettings *pwms = &SoftPWMSettings;

	if( channel >= 0 && channel < SOFTPWMCHANNELSNUM )
	{
		SETBIT( pwms->bitmask, myLeds[ channel ].ledPin );
		myLeds[ channel ].prescale = PWMPRESCALER;
		myLeds[ channel ].pwmPosition = 1;
		//myLeds[ channel ].pwmValue = ledFadePWMValues[ 1 ];
		//myLeds[ channel ].pwmDirection = PWMDIRUP;
	}
}

static void offChannel( uint8_t channel )
{
	static pwmSettings *pwms = &SoftPWMSettings;
	*pwms->pLedPort &= ~_BV( myLeds[ channel ].ledPin );
}

static void stopFade( uint8_t channel )
{
	static pwmSettings *pwms = &SoftPWMSettings;
	//if( channel >= 0 && channel < SOFTPWMCHANNELSNUM )
	{
		myLeds[ channel ].prescale = 0;
		myLeds[ channel ].pwmPosition = 0;
		//myLeds[ channel ].pwmDirection = PWMDIRNONE;
		CLEARBIT( pwms->bitmask, myLeds[ channel ].ledPin );
	}
}

// ADC

static void initADC( void )
{
	ADMUX |= _BV( REFS1 );
	ADCSR |= ( _BV( ADPS2 ) | _BV( ADPS1 ) | _BV( ADPS0 ) );   // adc enable, conversion complete interrupt enabled 
}

uint16_t getADCReading( uint8_t channel )
{
	uint16_t result;
	SETBIT( ADCSR, ADEN );
	SETBIT( ADCSR, ADSC );

	channel %= ADC_PINS;

	channel  |= ADMUX & -8; // '-8' creates a bit mask to clear the lower 3 bits of ADMUX
	ADMUX = channel; // Good thing the binary values for pin #s fit straight into ADMUX
	
	loop_until_bit_is_clear( ADCSR, ADSC );
	loop_until_bit_is_clear( ADCSR, ADSC );	
 
	CLEARBIT( ADCSR, ADEN );

	result = ADCL;
	result |= ( 0b00000011 & ADCH ) << 8;

	//return 1234;
	return result;
}

