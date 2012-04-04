#include "tick.h"
#include "event.h"
#include <util/delay.h>

#define F_CPU 8000000UL

#define ADIntDisable()	( ADCSR = ADCSR & ~( _BV( ADIF ) | _BV( ADIE ) ) )
#define ADIntEnable()	( ADCSR = (ADCSR & ~( _BV(ADIF) ) | _BV(ADIE) ) )


#define LED_TIMER TCCR1B
#define LED_TIMER_COUNTER TCNT0
#define LED_TIMER_PRESCALER	(_BV(CS10))  //256
#define LED_TIMER_START() (LED_TIMER=LED_TIMER_PRESCALER|_BV(CTC1))
#define LED_TIMER_STOP() (LED_TIMER=0)
#define LED_TIMER_PWM(value) OCR1B=value
#define LED_DDR_INIT()	(DDRB |= _BV(PB3) | _BV(PB2) | _BV(PB1) | _BV(PB0) )
#define LED_TIMER_INIT() TCCR1A|=_BV(COM1B1)|_BV(PWM1B);OCR1C=200;

#define TS_LED_DDRPORT DDRA
#define TS_LED_PORT PORTA

typedef union
{
	uint8_t portByte;
	struct {
		uint8_t pin0:1;
		uint8_t pin1:1;
		uint8_t pin2:1;
		uint8_t pin3:1;
		uint8_t pin4:1;
		uint8_t pin5:1;
		uint8_t pin6:1;
		uint8_t pin7:1;
	} portPinMap;
} portMap;

#define INITDDR(ddrport) ((volatile portMap*)_SFR_MEM_ADDR(ddrport))->portByte = _BV(PA0)
#define INITPORT(port) ((volatile portMap*)_SFR_MEM_ADDR(port))->portByte = 0
#define LED_PIN ((volatile portMap*)_SFR_MEM_ADDR(PORTA))->portPinMap.pin0
#define LED_PIN_TOGGLE() (PORTA ^= _BV(PA0))

#define E_LED_TOGGLE 100
#define E_PAUSE_LED 101
#define E_START_LED 102

uint8_t nLedState = 0;
void LED_Flash_Init(void);
void LED_Flash_Update(void);
void Pause_task(void);

void pause_Led_Fader_task(void);
void start_Walk_Led_Fader_Task( void );
void stop_Walk_Led_Fader_Task( void );
void Walk_Led_Fader_Task( void );

event currentEvent;
uint8_t eventResult;

uint8_t pauseLed = 0;
uint8_t ledTask = 0, pauseTask = 0, lf=0;
uint8_t tc;


uint8_t patternPlaying = 0;
uint8_t patternPointer = 0;
uint8_t patternPointerCounter = 0;
int8_t patternPointerDir = 1;
uint8_t F_PATTERN[] = { 
			5, 10,
			5, 12,
			5, 14,
			5, 16,
			5, 18,
			10, 20,
			10, 30,
			10, 40,
			10, 50,
			12, 60,
			14, 70,
			16, 80,
			18, 90,
			20, 100
		};
#define F_PATTERN_SIZE (sizeof(F_PATTERN)/2)

int main( void )
{
	LED_DDR_INIT();
	LED_TIMER_INIT();
	
	INITDDR( TS_LED_DDRPORT );
	INITPORT( TS_LED_PORT );
	LED_PIN = nLedState;

	TS_init();

	//ledTask = TS_addTask( Led_toggle_timer, 100, 200 );

	//2s
	pauseTask = TS_addTask( Pause_task, 2000, 0 );
	
	tc = TS_getTaskCount();
	reportNumber( tc );

	TS_start();
	LED_TIMER_START();


	LED_TIMER_PWM( 180 );
	_delay_ms( 500 );
	LED_TIMER_PWM( 0 );
	_delay_ms( 2000 );

	//enable interrupts
	sei();	

	for( ;; )
	{
		if( E_getEventCount() > 0 )
		{
			eventResult = E_getNextEvent( &currentEvent );
			
			//there was something returned
			if( eventResult != 0xFF )
			{
				switch( currentEvent.eventType )
				{
					case E_LED_TOGGLE:
						LED_PIN_TOGGLE();	
					break;
					case E_START_LED:
						start_Walk_Led_Fader_Task();
					break;
					case E_PAUSE_LED:
						stop_Walk_Led_Fader_Task();
						pauseTask = TS_addTask( Pause_task, 5000, 0 );
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

void reportNumber( uint8_t num )
{
	uint8_t i;

	LED_PIN = 1;
	_delay_ms( 50 );
	LED_PIN = 0;
	_delay_ms( 50 );
	LED_PIN = 1;
	_delay_ms( 50 );
	LED_PIN = 0;
	_delay_ms( 50 );
	LED_PIN = 1;
	_delay_ms( 50 );
	LED_PIN = 0;
	_delay_ms( 50 );
	_delay_ms( 1500 );

	for( i = 0; i < num; i++ )
	{
		LED_PIN = 1;
		_delay_ms( 100 );
		LED_PIN = 0;
		_delay_ms( 100 );
	}
	_delay_ms( 1500 );
}

void Pause_task( void )
{
	E_addEvent( E_START_LED, 10, 0 );
}

void start_Walk_Led_Fader_Task( void )
{
	patternPlaying = 1;
	patternPointer = 0;
	patternPointerDir = 1;

	patternPointerCounter = F_PATTERN[ ( patternPointer * 2 ) ];
	LED_TIMER_PWM( F_PATTERN[ ( patternPointer * 2 ) + 1 ] * 2 );

	if( !lf )
	{
		lf = TS_addTask( Walk_Led_Fader_Task, 10, 2 );
	}
}

void stop_Walk_Led_Fader_Task( void )
{
	TS_removeTask( lf );
	lf = 0;
	LED_TIMER_PWM(0);
}

void Walk_Led_Fader_Task(void)
{	
	if( patternPointerCounter == 0 )
	{
		LED_TIMER_PWM( F_PATTERN[ ( patternPointer * 2 ) + 1 ] * 2 );
		patternPointer += patternPointerDir;
		if( patternPointer == F_PATTERN_SIZE - 1 )
		{
			patternPointerDir = -1;
		}
		if( patternPointer == 0 )
		{
			E_addEvent( E_PAUSE_LED, 10, 0 );
			return;
		}
		patternPointerCounter = F_PATTERN[ ( patternPointer * 2 ) ];
	}
	else
	{
		patternPointerCounter--;
	}
}