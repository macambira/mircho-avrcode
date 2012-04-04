/***
 * Copyright (c) 2003 Jan Klötzke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *
 * Link layer for uart interface.
 *
 * TODO: maybe implement some sort of collision avoidance scheme
 */ 
 
#include <avr/io.h>
#include <avr/signal.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include "snap_def.h"
#include "snap_lnk.h"

// function from network layer handling received bytes
extern void snap_lnk_recv(uint8_t value, uint8_t err);

SIGNAL(SIG_UART_RECV)
{
	uint8_t err, value;
	
	if ((USR & _BV(FE)) != 0) err = SNAP_LNK_ERR_FRAMING; else err = 0;
	value = UDR;
	if ((USR & _BV(DOR)) != 0) err |= SNAP_LNK_ERR_OVERRUN;
	snap_lnk_recv(value, err);
}

void snap_lnk_init(void)
{
	UBRR = SNAP_F_CLK/(SNAP_BAUD_RATE*16l) - 1;	// set baud rate
	sbi(SEND_PORT, SEND_PIN);			// configure send pin
	sbi(SEND_DDR, SEND_PIN);
	UCR |= _BV(RXCIE) | _BV(RXEN);			// enable receiver+interrupt
}

void snap_lnk_send_start(void)
{
	cbi(UCR, RXEN);			// disable receiver
	sbi(UCR, TXEN);			// enable transmitter
	cbi(SEND_PORT, SEND_PIN);	// enable driver
	USR = _BV(TXC);			// clear txc flag
}

void snap_lnk_send(uint8_t value)
{
	loop_until_bit_is_set(USR, UDRE);
	UDR = value;
}

void snap_lnk_send_done(void)
{
	loop_until_bit_is_set(USR, TXC);	// wait for end of transmission
	sbi(SEND_PORT, SEND_PIN);		// disable driver
	cbi(UCR, TXEN);				// disable transmitter
	sbi(UCR, RXEN);				// enable receiver
}
