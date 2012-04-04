//lots of includes, but still fits nicely in a ATTINY13A
#ifdef F_CPU
#undef F_CPU
#endif
//internal oscilator at 8MHz
#define F_CPU (8000000UL)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/delay.h>
#include <util/atomic.h>

#define TRUE	1
#define FALSE	0

#define NUMBER_OF_LEDS	64

//The row sinks and the column sources current
//so the row has to have row bits high but the one that we need to turn on
//and the column has to have all column bits low but the one we need to turn on

#define ROW_PORT	PORTB
#define COL_PORT	PORTD
#define ROW_PIXEL(x)	(ROW_PORT = 0b11111111 ^ _BV(x))
#define ROW_OFF()		(ROW_PORT = 0b11111111)
#define COL_PIXEL(y)	(COL_PORT = 0b00000000 ^ _BV(y))

void initDevice( void );

//about 500 interrupts per second
#define TIMER2_COMPARE_VALUE	125

void initTimer2( void );
void startTimer2( void );
void stopTimer2( void );

//about 1000 interrupts per second
//#define TIMER0_LOAD_VALUE	130
//about 4000 interrupts per second
#define TIMER0_LOAD_VALUE	224

void initTimer0( void );
void startTimer0( void );
void stopTimer0( void );


void setPixel( uint8_t x, uint8_t y );
void clearPixel( uint8_t x, uint8_t y );

volatile uint8_t currentScanPixel;

/**
 * Animations
 */

typedef int8_t (*ANIMATION)( void );

typedef struct
{
	uint8_t		active;
	uint8_t		loop;
	uint8_t		currentFrame;
	uint16_t	ticksForCurrentFrame;
	uint8_t		nextFrameReady;
	uint8_t		tick;
	ANIMATION	currentAnimation;
} animationState;

animationState animation;
void setAnimationFunction( ANIMATION animFunc, uint8_t reset, uint8_t startAnimation );
void animationTick( void );
void setTick( void );
void clearTick( void );
void animationBufferSwap( void );

//types of animation functions
int8_t anim_movingDot( void );
int8_t anim_fillDot( void );
int8_t anim_spiral( void );

/**
 * Animations
 */


uint8_t buf1[] = {
	0b00111100,
	0b11000011,
	0b10100101,
	0b10000001,
	0b10100101,
	0b10011001,
	0b01000010,
	0b00111100
};

uint8_t buf2[] = {
	0b00111100,
	0b11000011,
	0b10100001,
	0b10000001,
	0b10100101,
	0b10011001,
	0b01000010,
	0b00111100
};

uint8_t * frame;
uint8_t * buffer;
uint8_t bufferReady = 0;

#define CLEAR_BUFFER( buf ) (memset( buf, 0, 8 ))

int main(void)
{
	static uint8_t oldPosition;
	static uint8_t divVal, modVal;

	cli();
	wdt_disable();
	set_sleep_mode( SLEEP_MODE_IDLE );
	initDevice();

	DDRD = 0b11111111;
	DDRB = 0b11111111;

	initTimer2();
	initTimer0();
	sei();
	startTimer2();
	startTimer0();
	
	frame	= buf1;
	buffer 	= buf2;

	CLEAR_BUFFER( frame );
	CLEAR_BUFFER( buffer );

	setAnimationFunction( anim_fillDot, TRUE, TRUE );
	animation.loop = TRUE;

    while ( 1 )
    {
		if( animation.tick )
		{
			animationTick();
			clearTick();
		}

		if( currentScanPixel == 0 )
		{
			if( animation.nextFrameReady )
			{
				animationBufferSwap();
			}
		}

		if( oldPosition != currentScanPixel )
		{
			modVal = currentScanPixel % 8;
			
			//we need to change columns only so often
			if( modVal == 0 )
			{
				divVal = currentScanPixel / 8;
				COL_PIXEL( divVal );
			}
			
			if( frame[ divVal ] & _BV( modVal ) )
			{
				ROW_PIXEL( modVal );
			}
			else
			{
				ROW_OFF();
			}
			oldPosition	 = currentScanPixel;
		}

		//if( TIME TO SLEEP ) { sleep_mode(); }

    }

}

void setPixel( uint8_t x, uint8_t y )
{
	COL_PIXEL( y );
	ROW_PIXEL( x );
}

void clearPixel( uint8_t x, uint8_t y )
{
	COL_PIXEL( y );
	ROW_OFF();
}

void initDevice( void )
{
	//disable analog comparator
	ACSR	|= _BV( ACD );
	//disable ADC
	ADCSRA	&= ~_BV( ADEN );
}

void initTimer0( void )
{
	//enable compare match interrupt
	TIMSK	|= _BV(TOIE0);
}

void startTimer0( void )
{
	TCNT0 = TIMER0_LOAD_VALUE;
	//set prescaler to 64
	TCCR0	|=  _BV(CS01) | _BV(CS00);
}

void stopTimer0( void )
{
	TCCR0	&=  ~( _BV(CS01) | _BV(CS00) );
}

ISR(TIMER0_OVF_vect)
{
	TCNT0 = TIMER0_LOAD_VALUE;
	currentScanPixel++;
	currentScanPixel %= NUMBER_OF_LEDS;
}

void initTimer2( void )
{
	//CTC mode
	TCCR2	|= _BV(WGM21);
	//enable compare match interrupt
	TIMSK	|= _BV(OCIE2);
	//this assures 500 FPS
	OCR2	= TIMER2_COMPARE_VALUE;
}

void startTimer2( void )
{
	//set prescaler to 128
	TCCR2	|=  _BV(CS22) | _BV(CS20);
}

void stopTimer2( void )
{
	TCCR2	&=  ~(_BV(CS22) | _BV(CS20));
}

ISR(TIMER2_COMP_vect)
{
	setTick();
}


/**
 * Animations
 */
void setAnimationFunction( ANIMATION animFunc, uint8_t reset, uint8_t startAnimation )
{
	animation.currentAnimation = animFunc;
	if( reset )
	{
		animation.currentFrame = 0;
	}
	
	animation.ticksForCurrentFrame = 0;

	if( startAnimation == TRUE )
	{
		animation.active = TRUE;
	}
	else
	{
		animation.active = FALSE;
	}
}

void animationTick( void )
{
	int8_t ticks;
	
	if( !animation.active )
	{
		return;
	}

	if( animation.ticksForCurrentFrame == 0 )
	{
		ticks = (*animation.currentAnimation)();
		if( ticks < 0 )
		{
			
			CLEAR_BUFFER( buffer );
			CLEAR_BUFFER( frame );
			if( animation.loop )
			{
				animation.currentFrame = 0;
				ticks = 128;
			}
			else
			{
				animation.active = FALSE;			
			}
		}
		else
		{
			animation.nextFrameReady = TRUE;
			animation.ticksForCurrentFrame = ticks;
		}
	}
	else
	{
		animation.ticksForCurrentFrame--;
	}
}

void setTick( void )
{
	animation.tick = TRUE;
}

void clearTick( void )
{
	animation.tick = FALSE;
}

void animationBufferSwap( void )
{
	uint8_t * swp;	
	swp 	= buffer;
	buffer 	= frame;
	frame 	= swp;
	animation.currentFrame++;
	animation.nextFrameReady = FALSE;	
}

int8_t anim_movingDot( void )
{
	if( animation.currentFrame >= NUMBER_OF_LEDS )
	{
		return -1;
	}

	CLEAR_BUFFER( buffer );
	buffer[ animation.currentFrame / 8 ] |= _BV( animation.currentFrame % 8 );
	buffer[ animation.currentFrame % 8 ] |= _BV( animation.currentFrame / 8 );

	return 18;		
}

int8_t anim_fillDot( void )
{
	if( animation.currentFrame >= NUMBER_OF_LEDS )
	{
		return -1;
	}

	memcpy( buffer, frame, 8 );	
	buffer[ animation.currentFrame / 8 ] |= _BV( animation.currentFrame % 8 );
	buffer[ animation.currentFrame % 8 ] |= _BV( animation.currentFrame / 8 );


	return 10;	
}

int8_t anim_spiral( void )
{
	static int8_t movx, movy;
	static uint8_t posx, posy;

	if( animation.currentFrame >= NUMBER_OF_LEDS )
	{
		return -1;
	}

	if( animation.currentFrame == 0 )
	{
		posx = 0;
		posy = 0;
		movx = 1;
		movy = 0;
	}

	memcpy( buffer, frame, 8 );
	
	buffer[ posy ] |= _BV( posx );
	posx += movx;
	posy += movy;
	

	return 10;	
}
