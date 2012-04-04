#include <hd44780.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define ADIntEnable()	( ADCSR |= (_BV( ADEN ) | _BV( ADIF ) ) | _BV( ADIE ) )
#define ADIntDisable()	( ADCSR &= ~( _BV(ADEN) | _BV( ADIF ) | _BV( ADIE ) ) )

//void shiftOut(uint8_t bitOrder, uint8_t val);
void main( void ) __attribute__( ( noreturn ) );

uint16_t seconds;
uint8_t sec_str[10];

#define GetNextReadingsPointer() ( (readingsPointer+1) & 0x07 )
#define GetPrevReadingsPointer() ( (readingsPointer-1) & 0x07 )

uint16_t readings[ 8 ];
uint8_t  readingsPointer = 0;
uint16_t xchngReading;
uint16_t lastReading;
uint16_t currentReading;

volatile uint16_t lightResult = 0;
volatile uint8_t lrAvailable = 0;
volatile uint16_t lastR = 0;
uint8_t pad;

void ADCInit(void);

void main( void )
{
	lcd_init();
	lcd_clrscr();
	lcd_command( ( _BV(LCD_DISPLAYMODE) | _BV(LCD_DISPLAYMODE_ON) ) & ~(_BV(LCD_DISPLAYMODE_CURSOR)|_BV(LCD_DISPLAYMODE_BLINK)) );

	//lcd_puts( "LIGHT" );
	lcd_puts( "TEMP" );

	ADCInit();
	ADIntEnable();

	//lcd_command( ( _BV(LCD_DISPLAYMODE) | _BV(LCD_DISPLAYMODE_ON) ) & ~(_BV(LCD_DISPLAYMODE_CURSOR)|_BV(LCD_DISPLAYMODE_BLINK)) );
	//lcd_command(_BV(LCD_DISPLAYMODE) | _BV(LCD_DISPLAYMODE_ON) );
	//lcd_goto( 0 );

	sei();

	ADCSR |= _BV( ADSC );

	while(1)
	{

		//lcd_clrscr();
		//lcd_home();
		if( !( 0x0F ^ lrAvailable ) )
		{
			lightResult = lightResult >> 4;
			readings[ readingsPointer ] = lightResult;
			//currentReading = currentReading + lightResult - readings[ GetPrevReadingsPointer() ];
			//currentReading = currentReading*7/9 + lightResult*2/9;
			currentReading = currentReading*14/20 + lightResult*6/20;
			readingsPointer = GetNextReadingsPointer();

			lcd_goto( 0x40 );
			//itoa( currentReading, sec_str, 10 );
			itoa( lastR, sec_str, 10 );
			pad = 4 - strlen( sec_str );
			//pad left
			for( uint16_t i = 0; i < pad; i++ ) lcd_putc( '0' );
			lcd_puts( sec_str );

			lightResult = 0;
			lrAvailable = 0;
		}
		/*
		if( seconds & 0b1 )
		{
			lcd_puts( "SLON    " );
		}
		else
		{
			lcd_puts( "KROKODIL" );
		}
		*/
		//_delay_ms( 1000 );
		//seconds++;
	}
}

void ADCInit(void)
{
	//ADMUX |= _BV( MUX0 ) | _BV( MUX1 ); //_BV( REFS0 ) | _BV( ADLAR ) |
	ADMUX |= _BV( MUX1 ) | _BV( MUX2 ) | _BV( REFS1 );//_BV( REFS0 ) | _BV( ADLAR ) |
	ADCSR |= _BV( ADSC ) | _BV( ADFR ) | _BV( ADPS2 ) | _BV( ADPS1 ) | _BV( ADPS0 ); //
	DDRA &= ~_BV(PA7);
	PORTA &= ~_BV(PA7);
	//ADMUX &= ~( _BV( REFS0 ) | _BV( REFS1 ) );
	//ADMUX |= _BV( REFS0 );
}


ISR( ADC_vect )			//Interrupt subrutine, this sub runs automaticly every time ADC conversion is complete
{
	register uint16_t tmpr = 0;
	tmpr = ADCL;
	tmpr |= (0b00000011 & ADCH)<<8;
	lastR = tmpr;
	lightResult += tmpr;
	lrAvailable++;
}

/*
void shiftOut(uint8_t bitOrder, uint8_t val)
{
  uint8_t i,sh;

  for (i = 0; i < 8; i++)  {
    //7-i == 0b111^i
    sh = ( bitOrder == LSBFIRST ) ? ( 1<<i ) : ( 0b111 ^ i );

    LCD_DATA = 1&( ( val & ( 1 << sh ) ) >> sh );
    LCD_CLK = 1;
    LCD_CLK = 0;
  }
}
*/
