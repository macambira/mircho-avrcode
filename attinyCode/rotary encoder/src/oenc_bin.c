/* Optical encoder from CDROM motor, Output signals are RS232 and an
 * emulated mechanical encoder output.
 *
 *		         +-----+
 * Pin usage:	  reset -|     |- vcc
 *		lm393_0 -|     |- lm393_2
 *		lm393_1 -|     |- txd
 *		    gnd -|     |- pulse
 *		         +-----+
 *
 * Signals: RS232 signal on PB1, pulse on PB0. Serial characters are
 *	'01000010' and '01111110', pulse is durings bits 4 and 5.
 *
 * 	RS232          -- ---- -           (up, 'w')
 *		-------  -    - ----------
 * 		       ---    --           (down, 'B')
 *		-------   ----  ----------
 *	Pulse	           --
 *		-----------  -------------
 * Uses RC oscillator @ 9.6 MHz
 */

#undef	VERBOSE				/* Define to display adc values	*/
#define	USE_SLEEP
// Define INVERTED if real RS232 driver, e.g. MAX232, is used
#undef INVERTED

#define	F_CPU	9600000UL		/* Need this for "util/delay.h"	*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#define LM393_0		PB3
#define LM393_1		PB4
#define LM393_2		PB2

#define TXD		_BV(PB1)
#define BAUD_RATE	9600
#if	INVERTED
#define	SET_TX 		PORTB |= TXD
#define	CLEAR_TX 	PORTB &= (unsigned char)~TXD
#else
#define	CLEAR_TX 	PORTB |= TXD
#define	SET_TX 		PORTB &= (unsigned char)~TXD
#endif

#define	PULSE		_BV(PB0)
#define	SET_PULSE 	PORTB |= PULSE
#define	CLEAR_PULSE 	PORTB &= (unsigned char)~PULSE

volatile unsigned char txState;
volatile unsigned char txByte;

ISR(TIM0_COMPA_vect) {
	switch(txState) {
		case 0:
		case 1:
			// idle or stop bit
			SET_TX;
			break;
		case 10:
			// start bit
			CLEAR_TX;
			break;
		default:
			// data bit
			if (txByte & 0x01)
				SET_TX;
			else
				CLEAR_TX;
			txByte = txByte >> 1;
			if (txState == 4) {
				SET_PULSE;
			} else if (txState == 6) {
				CLEAR_PULSE;
			}
			break;
	}
	if(txState)
		txState--;
}

ISR(PCINT0_vect) {
	// Just wakeup CPU
}

void sendChar(unsigned char c) {
#ifdef	USE_SLEEP
	while(txState) {
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_cpu();	// wait for TX to finish
	}
#else
	while(txState) {
	}
#endif
	txByte = c;
	txState = 10;
}

int main() {
	volatile unsigned char oldbits, newbits, bits;

	TCCR0A = _BV(WGM01);		// Clear timer on compare match mode
	TCCR0B = _BV(CS01);		// Clock prescale by 8
	OCR0A = F_CPU/8/BAUD_RATE - 1;	// Compare match at baud-rate
	TIMSK0 = _BV(OCIE0A);		// Enable  compare match interrupts
	
	DDRB = _BV(PB1) | _BV(PB0);	// PB1 and PB0 are outputs
	PORTB = TXD | _BV(PB3) | _BV(PB4) |
			_BV(PB2);	// TxD idle, enable pullups on inputs
	MCUCR &= ~_BV(PUD);		// Global pullup enable
	PCMSK = _BV(PCINT2) | _BV(PCINT3) |
		_BV(PCINT4);	// Enable pin change interrupt on inputs
	GIMSK |= _BV(PCIE);

#ifdef	USE_SLEEP
	sleep_enable();
#endif

	txState = 0;	// idle

	sei();

	while(1) {
#ifdef	USE_SLEEP
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_cpu();
#endif

		newbits = (PINB >> 2) & 7;
		bits = (newbits << 3) | oldbits;

		if ((bits == 064) || (bits == 026) || (bits == 032) ||
		    (bits == 013) || (bits == 051) || (bits == 045)) {
#ifdef	VERBOSE
			sendChar('0' + newbits);
			sendChar('0' + oldbits);
#endif
			sendChar('<');
#ifdef	VERBOSE
			sendChar('\r');
			sendChar('\n');
#endif
		} else if ((bits == 046) || (bits == 054) || (bits == 015) ||
		           (bits == 031) || (bits == 023) || (bits == 062)) {
#ifdef	VERBOSE
			sendChar('0' + newbits);
			sendChar('0' + oldbits);
#endif
			sendChar('B');
#ifdef	VERBOSE
			sendChar('\r');
			sendChar('\n');
#endif
		}
#ifdef	VERBOSE
		else if (oldbits != newbits) {
			sendChar('0' + newbits);
			sendChar('0' + oldbits);
			sendChar('=');
			sendChar('\r');
			sendChar('\n');
		}
#endif
		oldbits = newbits;
	}

	return 0;  // unreached
}

