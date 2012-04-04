/**
 * The Encoder code
 *
 * more info at:
 * http://mechatronics.mech.northwestern.edu/design_ref/sensors/encoders.html
 * http://mechatronics.mech.northwestern.edu/design_ref/sensors/conv.jpg
 *
 */

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "util.h"
#include "eventcodes.h"
#include "event.h"
#include "tick.h"
#include "encoder.h"


#define TIMING_READ_ENCODER_TASK		(TIMING_MILLISECOND_IN_TICKS * 1)
#define TIMING_READ_BUTTON_TASK		(TIMING_MILLISECOND_IN_TICKS * 3)

#define BUTTON_COUNT_TIMEOUT			(5)

#define JITTER_BUFFER				(7)
#define LEVEL_BUFFER					(255)

/**
 * Gray encoding doing the following newvalue << 3 | oldvalue
 */
//values when going up, incrementing the counter
//001 1 1	1	1
//011 2 10	2	3
//010 3 11	3	2
//110 4 101	5	6
//100 5 110	6	4
//101 6 111	7	5

uint8_t grayCodes[7] = {0,1,3,2,6,7,5};

#define GRAY_TO_BINARY( num ) (grayCodes[ num ])
#define GRAY_ENCODE( oldval, newval ) ( newval << 3 | oldval )

static volatile uint8_t button1Timeout;
static volatile uint8_t button2Timeout;

uint8_t readEncoder( void )
{
	uint8_t encoderStatus = ( 0b111 & PINC );
	//the first implementation of the module had a hardware wiring error
	/*
	uint8_t swap = encoderStatus<<2;
	encoderStatus &= ~0b101;
	encoderStatus |= swap & 0b100;
	swap >>= 4;
	encoderStatus |= swap & 0b1;
	*/
	//
	return encoderStatus;
}

void disableButtonInt(void)
{
	GICR	&= ~( _BV( INT0 ) | _BV( INT1 ) );
}

void enableButtonInt(void)
{
	GICR	|= _BV( INT0 ) | _BV( INT1 );
}

DEFINE_EVENT_HANDLER( READ_BUTTON_HANDLER )
//event eventObject as param
{
	if( button1Timeout > 0 )
	{
		button1Timeout--;
		if( button1Timeout == 0 )
		{
			if( pind.pin2 == 0 )
			{
				RAISE_EVENT( BUTTON1_PRESS, 0 );
			}
			enableButtonInt();
		}
	}

	if( button2Timeout > 0 )
	{
		button2Timeout--;
		if( button2Timeout == 0 )
		{
			if( pind.pin3 == 0 )
			{
				RAISE_EVENT( BUTTON2_PRESS, 0 );
			}
			enableButtonInt();
		}
	}
}

DEFINE_EVENT_HANDLER( READ_ENCODER_HANDLER )
//event eventObject as param
{
	//this buffer is used to buffer fluctuations in the movement of the encoder
	static uint8_t buffer = 0;
	static uint16_t levelBuffer = 0;
	
	static uint8_t encoderStatus = 0;
	static uint8_t oldEncoderStatus = 0;

	uint8_t statusCode;
	
	uint8_t direction = DIRECTION_NONE;
	uint8_t dir;

	encoderStatus = readEncoder();
	
	if( levelBuffer > 0 )
	{
		levelBuffer--;
		if( levelBuffer == 0 )
		{
			buffer = JITTER_BUFFER / 2;
		}
	}
	//nothing changed, so return the DIRECTION_NONE direction
	if( encoderStatus == oldEncoderStatus )
	{
		return;
	}
	
	statusCode = GRAY_ENCODE( oldEncoderStatus, encoderStatus );

	switch( statusCode )
	{
				 //reversed //nonreversed
		case 25: //011 001	//110 100
		case 19: //010 011	//010 110
		case 50: //110 010  //011 010
		case 38: //100 110	//001 011
		case 44: //101 100	//101 001
		case 13: //001 101  //100 101
			dir = 1;
			break;
		case 11: //001 011
		case 26: //011 010
		case 22: //010 110
		case 52: //110 100
		case 37: //100 101
		case 41: //101 001
			dir = 0;
			break;
		default:
			dir = 2;
			break;	
	}
	
	levelBuffer = LEVEL_BUFFER;
	if (dir == 1)
	{
		if( buffer == JITTER_BUFFER )
		{
			//next step
			direction = DIRECTION_FWD;
		}
		else
		{
			buffer++;
		}
	}
	else if( dir == 0 )
	{
		if( buffer == 0 )
		{
			//previous step
			direction = DIRECTION_BACK;
		}
		else
		{
			buffer--;
		}
	}
	
	oldEncoderStatus = encoderStatus;

	if( direction != DIRECTION_NONE )
	{
		RAISE_EVENT( ENCODER_STATUS_CHANGE, EVENT_PARAM( encoderStatus, direction ) )
	}
	
}

void encoderPortInit( void )
{
	ddrc.dd0 = IN;
	ddrc.dd1 = IN;
	ddrc.dd2 = IN;

	portc.port0 = PULLUP_ENABLED;
	portc.port1 = PULLUP_ENABLED;
	portc.port2 = PULLUP_ENABLED;	
}

/************************************************************************/
/* Button press handling code                                           */
/************************************************************************/
void setupButtonReading(void)
{
	ddrd.dd2 = IN;
	portd.port2 = PULLUP_ENABLED;

	ddrd.dd3 = IN;
	portd.port3 = PULLUP_ENABLED;

	MCUCR	&= ~( _BV( ISC01 ) | _BV( ISC00 ) ); //int0 on low level of pd2, with pullup this means the button is pressed
	MCUCR	&= ~( _BV( ISC10 ) | _BV( ISC11 ) ); //int1 on low level of pd3
	//MCUCR	|= _BV( ISC00 );
	GICR	|= _BV( INT0 ) | _BV( INT1 );
}

extern void encoderInit( void )
{
	encoderPortInit();
	setupButtonReading();

	REGISTER_EVENT_HANDLER( READ_BUTTON_HANDLER )
	ADD_EVENT_TASK( EVENT_CODE( READ_BUTTON_HANDLER ), 0, TIMING_READ_BUTTON_TASK );
	REGISTER_EVENT_HANDLER( READ_ENCODER_HANDLER )
	ADD_EVENT_TASK( EVENT_CODE( READ_ENCODER_HANDLER ), 0, TIMING_READ_ENCODER_TASK );
}

ISR( INT0_vect )
{
	button1Timeout = BUTTON_COUNT_TIMEOUT;
	disableButtonInt();
}

ISR( INT1_vect )
{
	button2Timeout = BUTTON_COUNT_TIMEOUT;
	disableButtonInt();
}
