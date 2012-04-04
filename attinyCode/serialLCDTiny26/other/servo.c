/*
 * servo.c
 *
 * servo routines for lcd terminal on ATTiny2313
 *
 * Copyright (C) 2005 Jos Hendriks <jos.hendriks@zonnet.nl>
 *
 */
 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

// Timer Compare Match ISR
SIGNAL(SIG_TIMER1_COMPB)
{
	PORTD = PORTD & 0xBF; // Clear PortD PIN6
}

SIGNAL(SIG_TIMER1_COMPA)
{
	PORTD = PORTD | 0x40; // Set PortD PIN6
}

void setServo(unsigned char pos)
{
	OCR1B = pos;
}

void initServo(void)
{
	// Initiate Port
	DDRD = 0x40;	// PortD PIN6 as output

	// Initiate Timer
	TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);	// clk/64 and Clear on match
	OCR1B = 150; // 0-255.
	OCR1A = ((8000000/64)/1000)*15; // 15 msec
	
	TIMSK = _BV(OCIE1B) | _BV(OCIE1A);	// Enable Compare match interrupt
}