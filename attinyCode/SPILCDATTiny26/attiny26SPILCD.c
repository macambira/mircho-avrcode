//master example http://www.newhavendisplay.com/app_notes/Serial_LCD.txt
//more on AVR_SPI http://www.rocketnumbernine.com/2009/04/26/using-spi-on-an-avr-1/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "hd44780.h"
#include "spi_via_usi_driver.h"

#define TRUE 1
#define FALSE 0

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

#define bTimerTick ( (volatile PackedBool*) (&EEDR) )->f0

//define protocol to communicate
#define CMD_PREFIX	0xFE
#define CMD_SUFFIX	0xFD

typedef enum _command_set {
		NONE = 0x00,
		CLEAR = 0x01,                                   // clear screen and set cursor to 0x00
		MOVE_HOME = 0x02,                               // set cursor to 0x00
		EM_DECREMENT_OFF = 0x04,                        // move cursor left after write to DDRAM
		EM_DECREMENT_ON = 0x05,                         // move text right after write to DDRAM
		EM_INCREMENT_OFF = 0x06,                        // move cursor left after write to DDRAM
		EM_INCREMENT_ON = 0x07,                         // move text right after write to DDRAM
		BLANK_DISPLAY = 0x08,                           // display on/off
		CURSOR_INVISIBLE = 0x0C,                        // cursor is not visible
		CURSOR_VISIBLE_ALT = 0x0D,                      // cursor visible as alternating block/underline
		CURSOR_VISIBLE_UNDERLINE = 0x0E,				// cursor visible as underline
		CURSOR_VISIBLE_BLOCK = 0x0F,					// cursor visible as blinking block
		MOVE_LEFT = 0x10,                               // move cursor left
		MOVE_RIGHT = 0x14,                              // move cursor right
		SCROLL_LEFT = 0x18,                             // scroll DDRAM left
		SCROLL_RIGHT = 0x1E,                            // scroll DDRAM right
		SET_POSITION = 0x80,                             // sets cursor position on DDRAM (or with function)
		MOVE_TO = 0x81,
		INIT_LCD = 0x82
} command;

static uint8_t inByte;

struct _command
{
	uint8_t cmdMode;
	uint8_t paramIdx;
	union
	{
		struct
		{
			uint8_t cmdCode;
			union
			{
				uint16_t cmdParam;
				uint8_t cmdParams[ 2 ];
			};
		} cmdStruct;
		uint8_t cmd[ 3 ];
	};
} commandBlock = {0};


uint8_t isAllowedCommand( void );
uint8_t execCommand( void );
void clearCommand( void );
uint8_t getCommandParamLen( uint8_t );
void lcd_puts(const char *);


volatile uint8_t timerCounter = 0;
uint8_t nextCounter = 25;
uint8_t	displayTickMark = 1;
uint8_t displayTickMarkCharacter = '.';

#define TIMER_LOAD_VALUE (255-78)  //this and the 1024 prescaler provides for 100 ticks per second

static void startTimer0( void )
{
	TCNT0	 = TIMER_LOAD_VALUE;
	TCCR0 	 = _BV( CS02 ) | _BV( CS00 ); //clock div 1024
	TIMSK 	|= _BV( TOIE0 );
}

static void stopTimer0( void )
{
	TIMSK	&= ~_BV( TOIE0 );
	TCCR0	 = 0;
}

void TIMER0_OVF0_vect(void) __attribute__((signal,naked));
void TIMER0_OVF0_vect(void)
{
	asm volatile( "sbi	0x1d, 0" );
    asm volatile( "reti" );
	//bTimerTick = TRUE;
}



int main(void)
{
	ACSR |= _BV(ACD);

	spiX_initslave( 0 );

	lcd_init();

	lcd_locate( 0, 0 );
	lcd_puts( " SPI LCD" );
	lcd_locate( 1, 0 );
	lcd_puts( "ATTiny26" );

	sei();
	startTimer0();
	clearCommand();

	if( displayTickMarkCharacter )
	{
		//lcd_putc( displayTickMarkCharacter );
	}

    while(1)
    {
		/*
		if( spi_buffer_not_empty() )
		{
			inByte = spi_buffer_read();
			lcd_putc( inByte );
		}
		*/

		if( bTimerTick )
		{
			bTimerTick = 0;
			TCNT0 = TIMER_LOAD_VALUE - TCNT0;
			timerCounter++;
		}

		/*
		if( displayTickMark && ( timerCounter == nextCounter ) )
		{
			timerCounter = 0;
			lcd_locate( 0, 0 );
			lcd_putc( displayTickMarkCharacter );
			if( displayTickMarkCharacter == '.' )
			{
				displayTickMarkCharacter = 'o';
				nextCounter = 10;
			}
			else if( displayTickMarkCharacter == 'o' )
			{
				displayTickMarkCharacter = 'O';
			}
			else
			{
				displayTickMarkCharacter = '.';
				nextCounter = 100;
			}
		}
		*/

		if( spi_receive_buffer_not_empty() )
		{
			//displayTickMark = 0;
			//stopTimer0();

			inByte = spi_receive_buffer_read();
			if( !commandBlock.cmdMode )
			{
				if( inByte == CMD_PREFIX )
				{
					commandBlock.cmdMode = 1;
				}
				else
				if( inByte > 0x20 && inByte < 0xFD )
				{
					lcd_putc( inByte );
				}
			}
			else
			{
				if( inByte == CMD_SUFFIX )
				{
					if( commandBlock.cmdMode )
					{
						execCommand();
					}
				}
				else
				//something went wrong and we received more params than needed
				if( commandBlock.paramIdx == 3 )
				{
					clearCommand();
				}
				else
				{
					commandBlock.cmd[ commandBlock.paramIdx++ ] = inByte;
				}
			}

		}

    }
}

uint8_t isAllowedCommand( void )
{
	switch( commandBlock.cmdStruct.cmdCode )
	{
		case MOVE_TO:
		case CLEAR:
		case MOVE_HOME:
		case CURSOR_INVISIBLE:                        // cursor is not visible
		case CURSOR_VISIBLE_ALT:                      // cursor visible as alternating block/underline
		case CURSOR_VISIBLE_UNDERLINE:				// cursor visible as underline
		case CURSOR_VISIBLE_BLOCK:
				return TRUE;
				break;
		default:
				return FALSE;
				break;
	}
	return FALSE;
}

uint8_t execCommand( void )
{
//	if( isAllowedCommand() )
	{
		if( commandBlock.cmdStruct.cmdCode == MOVE_TO )
		{
			lcd_locate ( commandBlock.cmdStruct.cmdParams[ 0 ], commandBlock.cmdStruct.cmdParams[ 1 ] );
		}
		else
		if( commandBlock.cmdStruct.cmdCode == INIT_LCD )
		{
				lcd_init();
		}
		else
		{
			lcd_command( 0, commandBlock.cmdStruct.cmdCode );
		}
	}
	clearCommand();
	return 1;
}

void clearCommand( void )
{
	commandBlock.cmdMode = 0;
	commandBlock.paramIdx = 0;
	commandBlock.cmd[0] = 0;
	commandBlock.cmd[1] = 0;
	commandBlock.cmd[2] = 0;
	//memset( &commandBlock, 0, sizeof( commandBlock) );
}

/*************************************************************************
Display string
Input:    string to be displayed
Returns:  none
*************************************************************************/
void lcd_puts(const char *s)
{
    register char c;

    while ((c=*s++))
      lcd_putc(c);
}
