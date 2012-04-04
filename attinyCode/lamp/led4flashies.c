//
// led4flashies - DIY High Power LED Driver Software
//                for the tinyAVR microcontroller ATtiny13A from ATMEL
//
// Based on work from Matthias Stonebrink <m.stonebrink@web.de>.
// Modified and extended by HD Stich <hd@palmtopia.de>.
//
// Ports used:
//
// PB1   | Output | PWM signal for the driver circuit board
// PB0   | Input  | Connected to the unpuffered supply voltage
//
// Version: 0.8
//


//
// Debugging
//
#ifdef DEBUG

#define DBG volatile

#else

#define DBG

#endif


//
// Standard includes.
//
#include <stdint.h>


//
// Load the configuration file.
//
#include "config.h"


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>


//
// Defines
//
//


//
// Typedefs
//


//
// States
//
typedef enum {
   StateUnknown = -1,
   StateInit,
   StateIdle,
   StatePowerDown,
   StatePowerDownDetected,
   StatePowerDownDebouncing,
   StatePowerUp,
   StatePowerUpDetected,
   StatePowerUpDebouncing
} State_t;


//
// Vars
//
static volatile uint16_t   ticks;
static volatile int8_t     newState = StateInit;

static DBG uint8_t currentPWMMode;
static DBG uint8_t percent;
static DBG uint8_t pwmValues[ NUM_PWM_VALUES ];


static DBG uint16_t debounceTimer;
static DBG uint8_t  powerDownDebouncing;
static DBG uint8_t  powerUpDebouncing;
static DBG uint16_t powerDownTimer;
static DBG uint8_t  powerDownTimerRunning;

static DBG uint8_t   clicks;
static DBG uint8_t   fastClicks;

static DBG uint8_t   programmingActive;
static DBG uint8_t   levelRampTime;
static DBG int8_t    levelSteps;

static DBG uint8_t   strobeTimer;
static DBG uint8_t   strobeTimerActive;
static DBG uint8_t   strobeFlag;


//
// ISR Watchdog Time-Out
// ~16ms ticks
//
ISR( WDT_vect )
{
   ticks++;
}


//
// ISR Pin Change Interrupt 0
//
ISR( PCINT0_vect )
{
   if ( (PINB & (1 << PB0)) )
   {
      newState = StatePowerUpDetected;
   }
   else
   {
      newState = StatePowerDownDetected;
   }
}


//
// PWM off
//
static void pwmOff( void )
{
        TCCR0A   &= ~(1 << COM0B1);
        PORTB      &= ~(1 << PB1);
}


//
// PWM on
//
static void pwmOn( void )
{
        TCCR0A   |= (1 << COM0B1);
}


//
// PWM control
//
static void pwm( uint8_t percent )
{
   OCR0B = (255L * percent) / 100;
}


//
// Read configuration from EEPROM
//
static void readConfig( void )
{
   uint8_t i;
   uint8_t pwmValue;

   for ( i = 0; i < NUM_PWM_VALUES; i++ )
   {
      pwmValue = eeprom_read_byte( (uint8_t *) (EEPROM_PWM_VALUES + i) );

      if ( 100 < pwmValue )
      {
         pwmValue = defaultPWMValues[i];

         eeprom_write_byte( (uint8_t *) (EEPROM_PWM_VALUES + i), pwmValue );
      }

      pwmValues[i] = pwmValue;
   }
}


//
// Go into idle or powerdown mode.
//
static void go2sleep( uint8_t sleepMode )
{
   set_sleep_mode( sleepMode );
   sleep_enable();
   sleep_cpu();
   sleep_disable();
}


//
// Init system
//
static void init( void )
{
   //
   // Configure Watchdog timer interrupt:
   //
   cli();
   wdt_reset();

   //
   // Clear WDRF in MCUSR:
   //
   MCUSR &= ~(1 << WDRF);

   //
   // Start timed sequence
   //
   WDTCR |= (1 << WDCE) | (1 << WDE);

   //
   // Set timer interrupt mode and
   // new prescaler (time-out) value = 2K cycles (~16ms):
   //
#if defined( __AVR_ATtiny13A__ )
   WDTCR = (1 << WDTIE);
#endif // __AVR_ATtiny13A__

#if defined( __AVR_ATtiny25__ ) \
 || defined( __AVR_ATtiny45__ ) \
 || defined( __AVR_ATtiny85__ )
   WDTCR = (1 << WDIE);
#endif // __AVR_ATtiny13A__

   readConfig();

   //
   // PWM:
   //

        DDRB    |= (1 << PB1);

        pwm( pwmValues[ currentPWMMode ] );

   TCNT0 = 0x00;
 
        TCCR0A |= (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
        TCCR0B |= (1 << CS00);

   //
   // Interrupts:
   //

   // Enable the interrupt from changes of pin PCINT0.
   //
   PCMSK |= (1 << PCINT0);
   GIMSK |= (1 << PCIE);

   sei();   
}


//
// main
//
int main( void )
{
   // Endless loop
   //
        while ( 1 )
        {
      // Switch states
      //

      switch ( newState )
      {
         case StateInit:

            // Init system
            //
            init();
           
            newState = StateIdle;

            break;


         case StateIdle:

            if ( programmingActive )
            {
               if ( (ticks - levelRampTime) >= LEVEL_RAMP_TIME )
               {
                  levelRampTime = ticks;

                  percent += levelSteps;

                  if ( 100 < percent )
                  {
                     levelSteps  = -1;
                     percent     = 99;
                  }
               
                  if ( 1 > percent )
                  {
                     levelSteps  = 1;
                     percent     = 2;
                  }

                  pwm( percent );
               }
            }

            else if ( (strobeTimerActive) && (ticks >= strobeTimer)  )
            {
               strobeTimer = ticks + STROBE_TIME;

               if ( strobeFlag )
               {
                  strobeFlag = 0;

                  pwm( percent );
               }
               else
               {
                  strobeFlag = 1;

                  pwm( 0 );
               }
            }

            go2sleep( SLEEP_MODE_IDLE );
            break;


         case StatePowerDownDetected:

            if ( powerUpDebouncing )
            {
               // Discard state change.
               //
               newState = StatePowerUpDebouncing;
            }
            else
            {
               if ( 1 == programmingActive )
               {
                  eeprom_write_byte( (uint8_t *) (EEPROM_PWM_VALUES + currentPWMMode), percent );

                  pwmValues[currentPWMMode]  = percent;
                  programmingActive          = 0;
               }

               powerDownDebouncing  = 1;
               debounceTimer        = ticks + DEBOUNCE_TIME;
               newState             = StatePowerDownDebouncing;

               pwmOff();
            }

            break;


         case StatePowerDownDebouncing:

            if ( ticks >= debounceTimer )
            {
               powerDownDebouncing  = 0;

               if ( !(PINB & (1 << PB0)) )
               {
                  newState = StatePowerDown;
               }
               else
               {
                  newState = StateIdle;
               }
            }
            else
            {
               go2sleep( SLEEP_MODE_PWR_DOWN );
            }

            break;


         case StatePowerDown:

            if ( 0 == powerDownTimerRunning )
            {
               powerDownTimerRunning   = 1;
               powerDownTimer          = ticks;
            }

            go2sleep( SLEEP_MODE_PWR_DOWN );

            break;


         case StatePowerUpDetected:

            if ( powerDownDebouncing )
            {
               // Discard state change.
               //
               newState = StatePowerDownDebouncing;
            }
            else
            {
               powerUpDebouncing    = 1;
               debounceTimer        = ticks + DEBOUNCE_TIME;
               newState             = StatePowerUpDebouncing;
            }

            break;


         case StatePowerUpDebouncing:

            if ( ticks >= debounceTimer )
            {
               powerUpDebouncing = 0;

               if ( (PINB & (1 << PB0)) )
               {
                  newState = StatePowerUp;
               }

               else
               {
                  newState = StateIdle;
               }
            }

            else
            {
               go2sleep( SLEEP_MODE_IDLE );
            }

            break;


         case StatePowerUp:

            pwmOn();

            if ( programmingActive )
            {
               // Do nothing!
            }

            else
            {
               if ( (ticks - powerDownTimer) <= MAX_PROG_SWITCH_TIME )
               {
                  fastClicks++;

                  if ( CLICKS_TO_PROG_MODE <= fastClicks )
                  {
                     fastClicks              = 0;
                     powerDownTimerRunning   = 0;
                     powerDownTimer          = 0;

                     percent                 = 1;
                     levelSteps              = 1;
                     levelRampTime           = ticks;
                     programmingActive       = 1;


                     pwm( percent );
                  }

                  newState = StateIdle;
               }

               else if ( (ticks - powerDownTimer) <= MAX_MODE_SWITCH_TIME )
               {
                  clicks++;

                  if ( CLICKS_TO_SWITCH_MODE <= clicks )
                  {
                     clicks                  = 0;
                     powerDownTimerRunning   = 0;
                     powerDownTimer          = 0;

                     currentPWMMode++;

                     if ( NUM_PWM_VALUES <= currentPWMMode )
                     {
                        currentPWMMode = 0;
                     }

                     percent = pwmValues[ currentPWMMode ];

                     strobeTimerActive = percent & 0x80;

                     if ( strobeTimerActive )
                     {
                        strobeTimer = ticks + STROBE_TIME;
                     }

                     percent &= 0x7F;

                     pwm( percent );
                  }

                  newState = StateIdle;
               }

               else
               {
                  clicks                  = 0;
                  fastClicks              = 0;
                  powerDownTimerRunning   = 0;
                  powerDownTimer          = 0;
                  newState                = StateIdle;
               }
            }

            break;

         default:
            break;
      }

        }
}