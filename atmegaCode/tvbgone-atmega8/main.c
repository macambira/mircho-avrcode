/*
Super TV-B-Gone Firmware
for use with ATmega8
modiffied to work with ATmega8 by Florin C. www.youritronics.com 10-oct-2008
original design Mitch Altman + Limor Fried 7-Oct-07


Distributed under Creative Commons 2.5 -- Attib & Share Alike
*/

#include <avr/io.h>             // this contains all the IO port definitions
#include <avr/interrupt.h>      // definitions for interrupts
#include <avr/sleep.h>          // definitions for power-down modes
#include <avr/pgmspace.h>       // definitions or keeping constants in program memory
#include <avr/wdt.h>
#include "main.h"

#define LED PB2         //  visible LED
#define IRLED1 PB3      //  IR LED
#define IRLED2 PB1      //  IR LED



#define NOP __asm__ __volatile__ ("nop")
// This function delays the specified number of 10 microseconds
#define DELAY_CNT 11
void delay_ten_us(uint16_t us) {
  uint8_t timer;
  while (us != 0) {
    for (timer=0; timer <= DELAY_CNT; timer++) {
      NOP;
      NOP;
    }
    NOP;
    us--;
  }
}


// This function quickly pulses the visible LED (connected to PB0, pin 5)
void quickflashLED( void ) {
  // pulse LED on for 30ms

  PORTB &= ~_BV(LED);         // turn on visible LED at PB0 by pulling pin to ground
  delay_ten_us(3000);         // 30 millisec delay
  PORTB |= _BV(LED);          // turn off visible LED at PB0 by pulling pin to +3V
}


// This function quickly pulses the visible LED (connected to PB0, pin 5) 4 times
void quickflashLED4x( void ) {
  quickflashLED();
  delay_ten_us(15000);        // 150 millisec delay
  quickflashLED();
  delay_ten_us(15000);        // 150 millisec delay
  quickflashLED();
  delay_ten_us(15000);        // 150 millisec delay
  quickflashLED();
}


// This function transmits one Code Element of a POWER code to the IR emitter, 
//   given offTime and onTime for the codeElement
//     If offTime = 0 that signifies the last Code Element of the POWER code
//     and the delay_ten_us function will have no delay for offTime 
//     (but we'll delay for 250 milliseconds in the main function)
void xmitCodeElement(uint16_t ontime, uint16_t offtime ) {
  // start Timer1 outputting the carrier frequency to IR emitters on OC1A (PB1, pin 6) and OC0A (PB0, pin 5)
  TCNT1 = 0; // reset the timers so they are aligned
  TCNT2 = 0;
  TIFR = 0;  // clean out the timer flags
  
/* 
1. set timer0 with toggle on compare match and CTC mode 
2. set timer1 with toggle on compare match and CTC mode, divider1 and start timer 1
3. start timer0
4. stop timer1
5. stop timer0
*/

TCCR1A =_BV(COM1A0) | _BV(WGM12); //set up timer 1
TCCR2 =_BV(COM20) | _BV(WGM21) | _BV(CS20); // set up and turn on timer 2
TCCR1B =_BV(CS10); // turn on timer 1 exactly 1 instruction later


  // keep transmitting carrier for onTime
  delay_ten_us(ontime);

 
TCCR2 = 0;	// stop timer 2
TCCR1A = 0;	// stop timer 1
	
  PORTB &= ~_BV(IRLED1) & ~_BV(IRLED2);           // turn off IR LED

  delay_ten_us(offtime);
}


void gotosleep(void) {
  // Shut down everything and put the CPU to sleep
  // put CPU into Power Down Sleep Mode

 // TCCR1 = 0;                      // turn off frequency generator (should be off already)
 // TCCR0B = 0;
TCCR2 = 0;	// stop timer 2
TCCR1A = 0;	// stop timer 1
  
  PORTB |= _BV(LED); // turn on the button pullup, turn off visible LED
  PORTB &= ~_BV(IRLED1) & ~_BV(IRLED2);           // turn off IR LED
  delay_ten_us(1000);             // wait 10 millisec second

  wdt_disable();

  MCUCR = _BV(SM1) |  _BV(SE);    // power down mode,  SE=1 (bit 5) -- enables Sleep Modes
  sleep_cpu(); 
}

//extern const struct powercode powerCodes[] PROGMEM;
extern const PGM_P *powerCodes[] PROGMEM;

extern uint8_t num_codes;

int main(void) {
  uint8_t i, j;
  uint16_t ontime, offtime;

//  TCCR1 = 0;                      // turn off frequency generator (should be off already)
//  TCCR0B = 0;
TCCR2 = 0;	// stop timer 2
TCCR1A = 0;	// stop timer 1

  i = MCUCSR;  // find out why we reset
  MCUCSR = 0;  // clear reset flags immediately

  // turn on watchdog timer immediately, this protects against
  // a 'stuck' system by resetting it
  wdt_enable(WDTO_1S); // 1 second long timeout

  // Set the inputs and ouputs
  PORTB &= ~_BV(IRLED1) & ~_BV(IRLED2);        // IR LED is off when pin is low
  DDRB = _BV(LED) | _BV(IRLED1) | _BV(IRLED2);    // set the visible and IR LED pins to outputs
  PORTB = _BV(LED);  //  visible LED is off when pin is high

  // check the reset flags
  if ((i & _BV(PORF)) ||   // batteries were inserted
      (i & _BV(BORF))) {    // brownout reset
    gotosleep(); // we only want to do something when the reset button is pressed
  }

  for (i=0; i<num_codes; i++) {   // for every POWER code in our collection
    wdt_reset();        // make sure we dont get 'stuck' in a code

    quickflashLED(); // visible indication that a code is being output
    PGM_P thecode_p = pgm_read_word(powerCodes+i);     // point to next POWER code

    uint8_t freq = pgm_read_byte(thecode_p);
    // set OCR for Timer1 and Timer0 to output this POWER code's carrier frequency
    OCR1A = OCR2 = freq; 
    
    // transmit all codeElements for this POWER code (a codeElement is an onTime and an offTime)
    // transmitting onTime means pulsing the IR emitters at the carrier frequency for the length of time specified in onTime
    // transmitting offTime means no output from the IR emitters for the length of time specified in offTime
    j = 0;  // index into codeElements of this POWER code
    do {
      // read the onTime and offTime from the program memory
      ontime = pgm_read_word(thecode_p+(j*4)+1);
      offtime = pgm_read_word(thecode_p+(j*4)+3);

      xmitCodeElement(ontime, offtime);  // transmit this codeElement (ontime and offtime)
      j++;
    } while ( offtime != 0 );  // offTime = 0 signifies last codeElement for a POWER code

    PORTB &= ~_BV(IRLED1) & ~_BV(IRLED2);           // turn off IR LED

    // delay 250 milliseconds before transmitting next POWER code
    delay_ten_us(25000);
  }
  

  // flash the visible LED on PB0  4 times to indicate that we're done
  delay_ten_us(65500); // wait maxtime 
  quickflashLED4x();

  gotosleep();
}
