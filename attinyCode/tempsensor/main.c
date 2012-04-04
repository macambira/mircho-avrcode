#define F_CPU		8000000     // Set Clock Frequency

#include <string.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include <hd44780/hd44780.h>
#define LCD_AVAILABLE 1

#include "adc.h"

#define ADC_Light 5
#define ADC_Temp 6

#define READ_TEMP 0
#define READ_LIGHT 1
#define WHAT_TO_READ READ_TEMP

#define REF_VOLTAGE 266

#define LED_PORT PORTB
#define LED_DDR  DDRB
#define LED_PORT_MASK _BV(PB6)|_BV(PB5)|_BV(PB4)|_BV(PB1)|_BV(PB0)

#define LED_1		((volatile io_reg*)_SFR_MEM_ADDR(LED_PORT))->bit0
#define LED_2		((volatile io_reg*)_SFR_MEM_ADDR(LED_PORT))->bit1
#define LED_3		((volatile io_reg*)_SFR_MEM_ADDR(LED_PORT))->bit4
#define LED_4		((volatile io_reg*)_SFR_MEM_ADDR(LED_PORT))->bit5
#define LED_5		((volatile io_reg*)_SFR_MEM_ADDR(LED_PORT))->bit6


#define LED_TIMER TCCR1B
#define LED_TIMER_COUNTER TCNT1
#define LED_TIMER_PRESCALER	(_BV(CS10)|_BV(CS11)|_BV(CS13))  //1024
#define LED_TIMER_START() LED_TIMER=LED_TIMER_PRESCALER|_BV(CTC1);TIMSK|=_BV(OCIE1B)|_BV(TOIE1)
#define LED_TIMER_STOP() LED_TIMER=0;TIMSK&=~(_BV(OCIE1B)|_BV(TOIE1))
#define LED_TIMER_PWM(value) LED_TIMER_COUNTER=0;OCR1B=value
#define LED_TIMER_INIT() TCCR1A|=_BV(COM1B1);LED_TIMER&=~_BV(CTC1);OCR1C=0


void toggleNextLed()
{
	static uint8_t led;
	
	switch(led)
	{
		case 0 : LED_1 = 0;
			break;
		case 1 : LED_2 = 0;
			break;
		case 2 : LED_3 = 0;
			break;
		case 3 : LED_4 = 0;
			break;
		case 4 : LED_5 = 0;
			break;
	}
	
	led++;
	led = led % 5;

	switch(led)
	{
		case 0 : LED_1 = 1;
			break;
		case 1 : LED_2 = 1;
			break;
		case 2 : LED_3 = 1;
			break;
		case 3 : LED_4 = 1;
			break;
		case 4 : LED_5 = 1;
			break;
	}
}



//start from 18 deg C
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

#define startTemp 18
#define lowTemp -100
#define highTemp 100
#define tempRangesCount (sizeof(tempRanges)/sizeof(uint16_t))

uint16_t getTempADC(void);
uint16_t getLightADC(void);
int8_t getTempInCelsius( uint16_t adcReading );


#if (LCD_AVAILABLE==1)
uint8_t sec_str[10];
#endif

uint16_t displayR = 0;
uint16_t lastR;
int8_t temp;

uint8_t currentLed;

int main( void ) __attribute__( ( noreturn ) ) __attribute__( ( OS_main ) );

int main( void )
{
	LED_DDR |= LED_PORT_MASK;
	LED_PORT &= ~(LED_PORT_MASK);
	
	DDRA &= ~_BV(PA7);
	PORTA &= ~_BV(PA7);
	
	//DDRB = _BV( PB4 )|_BV( PB5 );
	//
	//PORTB |= _BV( PB4 );
	//_delay_ms( 500 );
	//PORTB &= ~_BV( PB4 );
	//
	//PORTB |= _BV( PB5 );
	//_delay_ms( 10 );
	//PORTB &= ~_BV( PB5 );


#if (LCD_AVAILABLE==1)
	uint8_t pad;
	lcd_init();
	lcd_clrscr();
	lcd_command( ( _BV(LCD_DISPLAYMODE) | _BV(LCD_DISPLAYMODE_ON) ) & ~(_BV(LCD_DISPLAYMODE_CURSOR)|_BV(LCD_DISPLAYMODE_BLINK)) );
#endif

#if (LCD_AVAILABLE==1)

	lcd_clrscr();
	lcd_home();

	#if(WHAT_TO_READ==ADC_Temp)
		lcd_puts( "TEMP" );
	#endif

	#if(WHAT_TO_READ==ADC_Light)
		lcd_puts( "LIGHT" );
	#endif

	lcd_goto( 0x40 );
	
#endif

	displayR = getTempADC();
	ADC_init();
	
	//enable interrupts
	//sei();
		
	for(;;)
	{
		lastR = getTempADC();
		displayR = ( 50 * displayR ) + ( 14 * lastR );
		displayR >>= 6;
	
		temp = getTempInCelsius( displayR );

		/*		
		if( temp < 21 )
		{
			currentLed = PB0;
		} else if( temp < 23 )
		{
			currentLed = PB1;
		} else if( temp < 26 )
		{
			currentLed = PB4;
		} else if( temp < 27 )
		{
			currentLed = PB5;
		} else 
		{
			currentLed = PB6;
		}
		*/
	
		#if (LCD_AVAILABLE==1)
				lcd_goto( 0x0 );
				itoa( lastR, (char *)sec_str, 10 );
				pad = 4 - strlen( (char *)sec_str );
				for( uint8_t i = 0; i < pad; i++ ) lcd_putc( '0' );
				lcd_puts( (char *)sec_str );

				lcd_goto( 0x40 );
				itoa( temp, (char *)sec_str, 10 );
				pad = 4 - strlen( (char *)sec_str );
				for( uint8_t i = 0; i < pad; i++ ) lcd_putc( '0' );
				lcd_puts( (char *)sec_str );
		#endif
		
		_delay_ms( 100 );
			
		toggleNextLed();				
	}
}


uint16_t getTempADC(void)
{
	ADC_setPin( ADC_Temp );
	return ADC_readOnce();
}


uint16_t getLightADC(void)
{
	ADC_setPin( ADC_Light );
	return ADC_readOnce();
}


int8_t getTempInCelsius( uint16_t adcReading )
{
	if( adcReading < tempRanges[ 0 ] )
	{
		return lowTemp;
	}

	for( uint8_t i = 0; i < tempRangesCount; i = i + 2 )
	{
		if( adcReading >= tempRanges[ i ] && adcReading <= tempRanges[ i+1 ] )
		{
			return ( startTemp + i/2 );
		}
	}

	return highTemp;
}
