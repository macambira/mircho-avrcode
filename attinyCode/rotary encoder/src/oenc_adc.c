/* Optical encoder from CDROM motor
 * Uses RC oscillator @ 9.6 MHz
 *		       +-----+
 * Pin usage:	reset -|     |- vcc
 *		 adc3 -|     |- adc1
 *		 adc2 -|     |- txd
 *		  gnd -|     |- clk_out
 *		       +-----+
 */

#undef	VERBOSE				/* Define to display adc values	*/
#define	USE_SLEEP

#define	F_CPU	9600000UL		/* Need this for "util/delay.h"	*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#define TXD		_BV(PB1)
#define BAUD_RATE	9600
#if	INVERTED
#define	SET_TX 		PORTB |= TXD
#define	CLEAR_TX 	PORTB &= (unsigned char)~TXD
#else
#define	CLEAR_TX 	PORTB |= TXD
#define	SET_TX 		PORTB &= (unsigned char)~TXD
#endif

#define	INITIAL_AVERAGE	425	/* Based on measurements		*/
#define	THRESHOLD	20	/* 20 mV @ Vref = 1.1 & 10 bit ADC	*/

volatile unsigned char txState;
volatile unsigned char txByte;

volatile unsigned char conversionReady;

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
			break;
	}
	if(txState)
		txState--;

	// Toggle PB0. Frequency on PB0 is BAUDRATE / 2
	PORTB ^= _BV(PB0);
}

ISR(ADC_vect) {
	conversionReady = 1;
#if	0
	ADCSRA &= (unsigned char)~_BV(ADEN);
#endif
}

void sendChar(unsigned char c) {
#ifdef	USE_SLEEP
	while(txState) {
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_cpu();	// wait for TX to finish
	}
#endif
	txByte = c;
	txState = 10;
}

char hexchars[] = {'0','1','2','3','4','5','6','7',
			'8','9','a','b','c','d','e','f'};
void
sendHexByte(unsigned char b) {
	sendChar(hexchars[(b>>4)]);
	sendChar(hexchars[(b&0xf)]);
}

#if	1
/* Send hex word (16 bit) */
void sendHexWord(unsigned int num) {
	sendHexByte(num >> 8);
	sendHexByte(num & 0xff);
	sendChar(' ');
}
#endif

uint16_t
getADC(unsigned char channel) {
	uint16_t adcval;

	ADMUX = _BV(REFS0) | channel;
	ADCSRA |= _BV(ADSC);

	conversionReady = 0;
#ifdef	USE_SLEEP
	while(!conversionReady) {
		ADCSRA |= _BV(ADEN);
		set_sleep_mode(SLEEP_MODE_ADC);
		sleep_cpu();
	}
#endif

	adcval = ADCL;
	adcval |= (ADCH << 8);
	return adcval;
}

int main() {
	uint16_t adc;
	volatile uint16_t avgADC1, avgADC2, avgADC3;
	volatile unsigned char oldbits, newbits, bits;

	TCCR0A = _BV(WGM01);		// Clear timer on compare match mode
	TCCR0B = _BV(CS01);		// Clock prescale by 8
	OCR0A = F_CPU/8/BAUD_RATE - 1; // Compare match at baud-rate
	TIMSK0 = _BV(OCIE0A);		// Enable compare match interrupts
	
	DDRB = _BV(PB1) | _BV(PB0);	// PB1 and PB0 are outputs
	PORTB = TXD;	// idle, disable pullups on input pins.

	// ADC1/PB2, ADC2/PB4 and ADC3/PB3 as input, left adjust, Internal ref
	ADMUX = _BV(REFS0);
	// enable ADC with interrupt notification and /64 prescale
	ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1);	
	// Disable digital input buffers
	DIDR0 = _BV(ADC3D) | _BV(ADC2D) | _BV(ADC1D);

#ifdef	USE_SLEEP
	sleep_enable();
#endif

	txState = 0;	// idle

	avgADC1 = avgADC2 = avgADC3 = INITIAL_AVERAGE;

	sei();

	while(1) {
#ifdef	USE_SLEEP
		while(txState) {
			set_sleep_mode(SLEEP_MODE_IDLE);
			sleep_cpu();
		}
#endif

		newbits = oldbits & 7;

		adc = getADC(1);
		if (adc > (avgADC1 + THRESHOLD)) newbits |= 0x01;
		if (adc < (avgADC1 - THRESHOLD)) newbits &= ~0x01;
		// avgADC1 = (7 * avgADC1 + adc ) / 8;
#ifdef	VERBOSE
		sendHexWord(adc);
#endif

		adc = getADC(2);
		if (adc > (avgADC2 + THRESHOLD)) newbits |= 0x02;
		if (adc < (avgADC2 - THRESHOLD)) newbits &= ~0x02;
		// avgADC2 = (7 * avgADC2 + adc ) / 8;
#ifdef	VERBOSE
		sendHexWord(adc);
#endif

		adc = getADC(3);
		if (adc > (avgADC3 + THRESHOLD)) newbits |= 0x04;
		if (adc < (avgADC3 - THRESHOLD)) newbits &= ~0x04;
		// avgADC3 = (7 * avgADC3 + adc ) / 8;
#ifdef	VERBOSE
		sendHexWord(adc);
#endif

		bits = (newbits << 3) | oldbits;

		if ((bits == 064) || (bits == 026) || (bits == 032) ||
		    (bits == 013) || (bits == 051) || (bits == 045)) {
			sendChar('0' + newbits);
			sendChar('0' + oldbits);
			sendChar('u');
			sendChar('\r');
			sendChar('\n');
		} else if ((bits == 046) || (bits == 054) || (bits == 015) ||
		           (bits == 031) || (bits == 023) || (bits == 062)) {
			sendChar('0' + newbits);
			sendChar('0' + oldbits);
			sendChar('d');
			sendChar('\r');
			sendChar('\n');
		}
#ifdef	VERBOSE
		else {
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

