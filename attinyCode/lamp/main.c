//lots of includes, but still fits nicely in a ATTINY13A
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/pgmspace.h> 
#include <util/atomic.h> 

#include "main.h"

#define F_CPU 9600000UL

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


#define IN	0
#define OUT	1
#define ON	1
#define OFF	0
#define PULLUP_ENABLED 1
#define PULLUP_DISABLED 0

typedef struct {
  uint8_t pinb0:1;
  uint8_t pinb1:1;
  uint8_t pinb2:1;
  uint8_t pinb3:1;
  uint8_t pinb4:1;
  uint8_t pinb5:1;
  uint8_t pinb6:1;
  uint8_t pinb7:1;
} pinb_t;

typedef struct {
  uint8_t ddb0:1;
  uint8_t ddb1:1;
  uint8_t ddb2:1;
  uint8_t ddb3:1;
  uint8_t ddb4:1;
  uint8_t ddb5:1;
  uint8_t ddb6:1;
  uint8_t ddb7:1;
} ddrb_t;

typedef struct {
  uint8_t pb0:1;
  uint8_t pb1:1;
  uint8_t pb2:1;
  uint8_t pb3:1;
  uint8_t pb4:1;
  uint8_t pb5:1;
  uint8_t pb6:1;
  uint8_t pb7:1;
} portb_t; 

#define portb (*((volatile portb_t*)&PORTB))
#define pinb (*((volatile pinb_t*)&PINB))
#define ddrb (*((volatile ddrb_t*)&DDRB))

//values when going up, incrementing the counter
//001 1 1	1	1
//011 2 10	2	3
//010 3 11	3	2
//110 4 101	5	6
//100 5 110	6	4
//101 6 111	7	5

/*
 *
 * more info at:
 * http://mechatronics.mech.northwestern.edu/design_ref/sensors/encoders.html
 * http://mechatronics.mech.northwestern.edu/design_ref/sensors/conv.jpg
 *
 */

uint8_t grayCodes[7] = {0,1,3,2,6,7,5};
#define GRAY_TO_BINARY( num ) (grayCodes[ num ])

// variables to control program logic
uint8_t encoderStatus;
uint8_t oldEncoderStatus;

//this buffer is used to buffer fluctuations in the movement of the encoder
#define MAX_BUFFER 3
uint8_t buffer;
//click buffer counts how many clicks appeared before they were processed by the WDT timer
//I think I can react to quick turn of the wheel by quickly turning on the light
uint8_t clickBuffer;

//this makes for the direction of the movement
#define DIRECTION_NONE 0
#define DIRECTION_FWD 1
#define DIRECTION_BACK 2
uint8_t direction;


//index of the brightness in the ledFadePWMValues array
uint8_t brightnessIndex = 0;


/*
 * Queue with the next brightness values
 * The queue contains bytes that contain encoded brightness information
 * The LSB 5 bits are the brightness index in the ledFadePWMValues array (see main.h)
 * The MSB 3 bits are the duration, in WDT ticks, of this brightness
 *
 */
#define QUEUE_BYTE( brightness, time ) ( time << 5 | brightness )
#define BRIGHTNESS_FROM_QUEUE_BYTE( bbyte ) ( bbyte & 0x1f )
#define TIME_FROM_QUEUE_BYTE( bbyte ) ( bbyte >> 5 )

#define QUEUE_FULL	(brightnessQueue.queueSize == MAX_QUEUE)
#define QUEUE_SIZE	(brightnessQueue.queueSize)
#define QUEUE_EMPTY (QUEUE_SIZE == 0)

#define MAX_QUEUE 8

uint8_t currentQValue;
uint8_t repCounter = 0;

struct _brightnessQueue
{
	uint8_t queueStart;
	uint8_t queueEnd;
	uint8_t queueSize;
	uint8_t queue[ MAX_QUEUE ];
} brightnessQueue = { 0, 0, 0 };

void addToBrightnessQueue( uint8_t brightness, uint8_t time )
{
	if( QUEUE_FULL )
	{
		return;
	}
	
	brightnessQueue.queue[ brightnessQueue.queueEnd ] = QUEUE_BYTE( brightness, time );
	
	brightnessQueue.queueEnd++;
	brightnessQueue.queueEnd = brightnessQueue.queueEnd % MAX_QUEUE;
	brightnessQueue.queueSize++;
}

uint8_t getFromBrightnessQueue( void )
{
	uint8_t brightness;
	
	if( QUEUE_EMPTY )
	{
		return 0;
	}
	
	brightness = brightnessQueue.queue[ brightnessQueue.queueStart ];

	brightnessQueue.queueStart++;
	brightnessQueue.queueStart = brightnessQueue.queueStart % MAX_QUEUE;
	brightnessQueue.queueSize--;
	
	return brightness;
}

void emptyQueue()
{
	brightnessQueue.queueStart = brightnessQueue.queueEnd = 0;
	brightnessQueue.queueSize = 0;
}


// method to put MCU to sleep, deep power saving sleep
void systemSleep(void)
{
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();

	sleep_mode();

	// system continues execution here when watchdog times out
	sleep_disable();
}

void setupEncoderReading(void)
{

	ddrb.ddb2 = IN;
	ddrb.ddb3 = IN;
	ddrb.ddb4 = IN;
	
	portb.pb2 = PULLUP_ENABLED;
	portb.pb3 = PULLUP_ENABLED;
	portb.pb4 = PULLUP_ENABLED;
	
	/*
	DDRB |= 0b00011100;
	PORTB |= 0b00011100;
	*/
}

//
#define CHECK_PINS() ( ((volatile PackedBool*)(&EEDR))->f1 = TRUE )
#define CLEAR_CHECK_PINS() ( ((volatile PackedBool*)(&EEDR))->f1 = FALSE )
#define IS_CHECK_ON() ( ((volatile PackedBool*)(&EEDR))->f1 )
//

// Configure Timer/Counter
void setupTimer( void )
{
	TCCR0A = 0;
	TCCR0B = 0;

	OCR0A = 0;
	OCR0B = 0;
	
	//TCCR0A |= _BV( COM0A1 ) | _BV( COM0B1 ); 
	//TCCR0A |= _BV( WGM01 ) | _BV( WGM00 ); 		// Select Fast PWM Mode
	TCCR0A |= _BV( WGM00 ); 						// Select Phase Correct Mode
	TCCR0B |= _BV( CS01 ) | _BV( CS00 ); 			// Use the internal clock for the counter with prescaling.
	TIMSK0  |= _BV( TOIE0 ); 						// enable overflow interrupt, and use it for scanning the encoder state
}

void disconnectPins( void )
{
	TCCR0A &= ~( _BV( COM0A1 ) | _BV( COM0B1 ) );
}

void connectPins( void )
{
	TCCR0A |= _BV( COM0A1 ) | _BV( COM0B1 );
}


ISR( TIM0_OVF_vect, ISR_NAKED )
{
	CHECK_PINS();
	asm volatile ("reti"); 
}


//
#define SET_TICK() ( ((volatile PackedBool*)(&EEDR))->f0 = TRUE )
#define CLEAR_TICK() ( ((volatile PackedBool*)(&EEDR))->f0 = FALSE )
#define IS_TICK_ON() ( ((volatile PackedBool*)(&EEDR))->f0 )
//

// configure WDT
void setupWatchdog(void) 
{
	wdt_reset();
	WDTCR |=  _BV( WDCE ) | _BV( WDE ); 				// set the change enable bit and enable bit
	WDTCR  =  _BV(  WDTIE );	// set the interrupt bit and set timeout to ~2ms seconds (1024 cycles)
}

// interrupt method for WDT timeout
ISR( WDT_vect, ISR_NAKED )
{
	SET_TICK();
	asm volatile ("reti"); 
}

// main system method, processes fake fire and handles ADC/WDT logic
int main(void){
	cli();
	wdt_disable();
	
	//setup clock
	CLKPR = ( 1 << CLKPCE );	// enable pre-scale write
	CLKPR = 0; //_BV( CLKPS2 ); 	// set prescale to 0
	
	//MCUCR |= _BV( SE );// | _BV( SM1 );		// setup sleep mode to idle sleep or PWR_SAVE mode
	//PRR	 |= _BV( PRADC );			//power reductions: disable ADC

	ddrb.ddb0 = OUT;
	ddrb.ddb1 = OUT;

	setupWatchdog();
	setupEncoderReading();
	setupTimer();
	connectPins();
	
	buffer = 1;

	sei();							// enable global interrupts

	//forever, consume those batteries
	while( 1 ){

		//atomic operation
		if( IS_TICK_ON() )
		{
			if( clickBuffer > 10 )
			{
				emptyQueue();
				OCR0A = GetLedPwmValueByIndex( PWMValuesMAX );
				OCR0B = OCR0A;
			}
			else
			if( repCounter == 0 )
			{
				if( !QUEUE_EMPTY )
				{
					currentQValue = getFromBrightnessQueue();
					OCR0A = GetLedPwmValueByIndex( BRIGHTNESS_FROM_QUEUE_BYTE( currentQValue ) );
					OCR0B = OCR0A;
					repCounter = TIME_FROM_QUEUE_BYTE( currentQValue ) - 1;
				}
			}
			else
			{
				repCounter--;
			}
			clickBuffer = 0;
			CLEAR_TICK();
		}

		//status has changed
		if( IS_CHECK_ON() )
		{
			//buffer to provide atomicity
			encoderStatus = ( 0b11100 & PINB ) >> 2;
			
			//this mess is for debugging purposes
			if( encoderStatus != oldEncoderStatus )
			{
				if ( GRAY_TO_BINARY( encoderStatus ) > GRAY_TO_BINARY( oldEncoderStatus ) )
				{
					//an exception is when an overflow occurs )
					if( encoderStatus == 0b101 && oldEncoderStatus == 0b001 )
					{
						goto DEC_LABEL;
					}
					goto INC_LABEL;
				}
				else
				if ( GRAY_TO_BINARY( encoderStatus ) < GRAY_TO_BINARY( oldEncoderStatus ) )
				{
					if( encoderStatus == 0b001 && oldEncoderStatus == 0b101 )
					{
						goto INC_LABEL;
					}
					goto DEC_LABEL;
				}
		INC_LABEL:
				clickBuffer++;
				if( buffer == MAX_BUFFER )
				{
					//next step
					direction = DIRECTION_FWD;
				}
				else
				{
					buffer++;
				}
				goto END_ENCODER_LABEL;
		DEC_LABEL:
				clickBuffer = 0;
				if( buffer == 0 )
				{
					//previous step
					direction = DIRECTION_BACK;
				}
				else
				{
					buffer--;
				}
				goto END_ENCODER_LABEL;
		END_ENCODER_LABEL:
				oldEncoderStatus = encoderStatus;
			}

			CLEAR_CHECK_PINS();
		}
	
		if( direction == DIRECTION_FWD )
		{
			if( brightnessIndex < PWMValuesMAX )
			{
				brightnessIndex++;
			}
			addToBrightnessQueue( brightnessIndex, 1 );
			if( brightnessIndex == PWMValuesMAX )
			{
				addToBrightnessQueue( brightnessIndex - 2, 1 );
				addToBrightnessQueue( brightnessIndex, 1 );
			}
		}
		else
		if( direction == DIRECTION_BACK )
		{
			if( brightnessIndex != 0 )
			{
				brightnessIndex--;
			}
			addToBrightnessQueue( brightnessIndex, 1 );
			/*
			if( brightnessIndex == 0 )
			{
			}
			*/
		}
		direction = DIRECTION_NONE;
		//systemSleep();	// go to sleep
	}
}

void debugBlink( uint8_t times )
{
	while( times-- )
	{
		portb.pb1 = 1;
		_delay_ms( 200 );
		portb.pb1 = 0;
		_delay_ms( 200 );
	}
}
