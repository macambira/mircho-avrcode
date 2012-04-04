/*
 * led.h
 *
 * Author: mmirev
 */ 
#ifndef LED_H_
#define LED_H_

#include "monetlamp.h"

#if defined(ATMEGA8)

	#define LEDOFF (PORTD |= _BV(PD5));
	#define LEDON (PORTD &= ~_BV(PD5));
	#define LEDINIT()  PORTD |= _BV(PD5); DDRD |= _BV(PD5);

#elif defined(ATMEGA328)

	#define LEDOFF (PORTC &= ~_BV(PC5));
	#define LEDON (PORTC |= _BV(PC5));
	#define LEDINIT()  PORTC &= ~_BV(PC5); DDRC |= _BV(PC5);

#else
	#error "NO DEFINED LED"
#endif
/*
#define LEDON (PORTD |= _BV(PD5));
#define LEDOFF (PORTD &= ~_BV(PD5));
#define LEDINIT()  PORTD &= ~_BV(PD5); DDRD |= _BV(PD5);
*/
//My test led is with reverse logic

#endif /* LED_H_ */
