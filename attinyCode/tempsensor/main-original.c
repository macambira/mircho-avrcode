#define F_CPU                    8000000     // Set Clock Frequency

#include <string.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include <ticks/event.h>
#include <ticks/tick.h>

#include <hd44780/hd44780.h>
#define LCD_AVAILABLE 1

#include "adc.h"

#define ADC_Light 5
#define ADC_Temp 6

#define READ_TEMP 0
#define READ_LIGHT 1
#define WHAT_TO_READ READ_TEMP

#define REF_VOLTAGE 266

//16 exponential PWM values
uint8_t PWM_led[] = {
	0,
	1,
	2,
	3,
	4,
	6,
	9,
	13,
	19,
	27,
	40,
	58,
	84,
	122,
	176,
	255
};
#define MAX_FADE_VALUES sizeof(PWM_led)

uint8_t fadeValue, fadeDir;

void start_Walk_Led_Fader_Task( void );
void stop_Walk_Led_Fader_Task( void );
void Walk_Led_Fader_Task( void );
//handle for the Walk_Led_Fader_Task
uint8_t hLedFaderTask = NULL_TASK, hPauseTask = NULL_TASK;

#define E_LED_TOGGLE 100
#define E_PAUSE_LED 101
#define E_START_LED 102
#define E_MONITOR_TEMP 105

event currentEvent;
uint8_t eventResult;

void pauseTask(void);
void monitorTemperature(void);
void monitorTemperatureTask(void);

#define LED_TIMER TCCR1B
#define LED_TIMER_COUNTER TCNT1
#define LED_TIMER_PRESCALER	(_BV(CS10)|_BV(CS11)|_BV(CS13))  //1024
#define LED_TIMER_START() LED_TIMER=LED_TIMER_PRESCALER|_BV(CTC1);TIMSK|=_BV(OCIE1B)|_BV(TOIE1)
#define LED_TIMER_STOP() LED_TIMER=0;TIMSK&=~(_BV(OCIE1B)|_BV(TOIE1))
#define LED_TIMER_PWM(value) LED_TIMER_COUNTER=0;OCR1B=value
#define LED_TIMER_INIT() TCCR1A|=_BV(COM1B1);LED_TIMER&=~_BV(CTC1);OCR1C=0

volatile uint8_t currentLed;

#define LED_PORT PORTB
#define LED_DDR  DDRB
#define LED_PORT_MASK _BV(PB6)|_BV(PB5)|_BV(PB4)|_BV(PB3)|_BV(PB2)

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

int main( void ) __attribute__( ( noreturn ) ) __attribute__( ( OS_main ) );

int main( void )
{
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

	TS_init();
	TS_start();
	TS_addTask( monitorTemperatureTask, 0, 5000 );

	LED_DDR |= LED_PORT_MASK;
	
	LED_TIMER_INIT();
	LED_TIMER_START();
	
	//enable interrupts
	sei();
		
	for(;;)
	{
		if( E_getEventCount() > 0 )
		{
			eventResult = E_getNextEvent( &currentEvent );
			
			//there was something returned
			if( eventResult != 0xFF )
			{
				switch( currentEvent.eventType )
				{
					case E_START_LED:
						start_Walk_Led_Fader_Task();
					break;
				
					case E_MONITOR_TEMP:

						lastR = getTempADC();
						displayR = ( 50 * displayR ) + ( 14 * lastR );
						displayR >>= 6;
					
						temp = getTempInCelsius( displayR );
						
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

	
						#if (LCD_AVAILABLE==1)
								lcd_goto( 0x40 );
								itoa( temp, (char *)sec_str, 10 );
								pad = 4 - strlen( (char *)sec_str );
								for( uint8_t i = 0; i < pad; i++ ) lcd_putc( '0' );
								lcd_puts( (char *)sec_str );
						#endif	
						
						E_addEvent( E_START_LED, 0, 0 );					
					break;
					
				
					default:
					break;
				}

				currentEvent.eventType = 0;		
			}
		}

		TS_dispatchTasks();
	
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

void monitorTemperatureTask(void)
{
	E_addEvent( E_MONITOR_TEMP, 0, 0 );	
}

void start_Walk_Led_Fader_Task( void )
{
	if( hLedFaderTask != NULL_TASK )
	{
		return;
	}
	fadeDir = 1;
	fadeValue = 0;
	LED_TIMER_PWM( 0 );
	LED_TIMER_START();
	if( !hLedFaderTask )
	{
		hLedFaderTask = TS_addTask( Walk_Led_Fader_Task, 0, 30 );
	}
}

void stop_Walk_Led_Fader_Task( void )
{
	if( hLedFaderTask == NULL_TASK )
	{
		return;
	}
	TS_removeTask( hLedFaderTask );
	hLedFaderTask = NULL_TASK;
	fadeValue = 0;
	LED_TIMER_STOP();
	LED_PORT &= ~_BV(currentLed);
}


void Walk_Led_Fader_Task(void)
{
	fadeValue += fadeDir;
	
	if( fadeValue == 0 )
	{
		stop_Walk_Led_Fader_Task();
	}

	//LED_TIMER_PWM( PWM_led[ fadeValue ] );
	LED_TIMER_PWM( 10 );

	if( fadeValue == MAX_FADE_VALUES - 1 )
	{
		fadeDir = -1;
	}
}


ISR( TIMER1_CMPB_vect )
{
	LED_PORT &= ~_BV( currentLed );
}

ISR( TIMER1_OVF1_vect )
{
	LED_PORT |= _BV( currentLed );
}

void sleep( void ) 
{
  ADMUX	= 0x00;             	//Disable bandgap reference
  ACSR	&= ~_BV( ACD );
  ADCSR &= ~_BV( ADEN );   //Disable ADC
  DDRB  = 0x00;             	//all pins input
  PORTB = 0x00;             	//all pins tristate
#ifdef PRR
  PRR   = 0xFF;					//turn off all clocks
#endif
  set_sleep_mode( SLEEP_MODE_PWR_DOWN );
  sleep_mode();
}
