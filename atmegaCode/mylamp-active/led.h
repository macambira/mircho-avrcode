/*
 * led.h
 *
 * Author: mmirev
 */ 
#ifndef LED_H_
#define LED_H_

#define LEDON (PORTD |= _BV(PD0));
#define LEDOFF (PORTD &= ~_BV(PD0));
#define LEDINIT()  PORTD &= ~( _BV(PD0) | _BV( PD1 ) ); DDRD |= _BV(PD0) | _BV(PD1);

#endif /* LED_H_ */