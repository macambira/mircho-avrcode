/*
 * ps2.c
 *
 * ps2 routines for lcd terminal on ATTiny2313
 *
 * Copyright (C) 2005 Jos Hendriks <jos.hendriks@zonnet.nl>
 *
 * This code adapted from:
 *   Program:    PC Keyboard
 *   Created:    07.09.99 21:05
 *   Author:     V. Brajer - vlado.brajer@kks.s-net.net
 *               http://www.sparovcek.net/bray
 *               based on Atmel's AVR313 application note
 *   Comments:   AVRGCC port - original for IAR C
 *
 * Original copyrights:
 * Copyright (C) V. Brajer <vlado.brajer@kks.s-net.net>
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include "ps2.h"
#define BUFF_SIZE 16

volatile unsigned char edge, bitcount;                // 0 = neg.  1 = pos.
unsigned char ps2_buffer[BUFF_SIZE];
unsigned char *inpt, *outpt;
volatile unsigned char buffcnt;

void initps2(void)
{
    DDRD = 0x00;
	
	MCUCR = 0x02;							// INT0 interrupt on falling edge
	GIMSK = (1<<INT0);	            		// enable external int0
    edge = 0;                               // 0 = falling edge  1 = rising edge
    bitcount = 11;

    inpt =  ps2_buffer;                      // Initialize buffers
    outpt = ps2_buffer;
	
    buffcnt = 0;
}

SIGNAL(SIG_INT0)
{
    static unsigned char scancode;              // Holds the received scan code
    if (!edge)                                // Routine entered at falling edge
    {
        if(bitcount < 11 && bitcount > 2)    // Bit 3 to 10 is data. Parity bit,
        {                                     // start and stop bits are ignored.
            scancode = (scancode >> 1);
            if(bit_is_set(PIND,PD4)) scancode = scancode | 0x80;
        }
        MCUCR = 0x03;                  		  // Set interrupt on rising edge
        edge = 1;
    }
    else
    {                                         // Routine entered at rising edge
        MCUCR = 0x02;                         // Set interrupt on falling edge
        edge = 0;
        if(--bitcount == 0)                  // All bits received
        {
            decode(scancode);
            bitcount = 11;
        }
    }
}

void decode(unsigned char sc)
{
    static unsigned char is_up=0;

    if (!is_up)                // Last data received was the up-key identifier
    {
        switch (sc)
        {
          case 0xF0 : is_up = 1; break;      		  // The up-key identifier
		  // Misc characters used for the protocol
		  case 0xE0 : break;
		  case 0x11 : break;
		  case 0x12 : break;
		  /*
		  case 0x4D : put_kbbuff(0x01); break;      // Power
		  case 0x78 : put_kbbuff(0x02); break;      // MENU
		  case 0x07 : put_kbbuff(0x03); break;      // MEMO
		  case 0x09 : put_kbbuff(0x04); break;      // RUN/STOP
		  case 0x01 : put_kbbuff(0x05); break;      // MODE
		  case 0x3B : put_kbbuff(0x06); break;      // A
		  case 0x31 : put_kbbuff(0x07); break;      // B
		  case 0x35 : put_kbbuff(0x08); break;      // C
		  case 0x1D : put_kbbuff(0x09); break;      // D	  
		  case 0x16 : put_kbbuff(0x10); break;      // 1
		  case 0x1E : put_kbbuff(0x11); break;      // 2
		  case 0x26 : put_kbbuff(0x12); break;      // 3
		  case 0x25 : put_kbbuff(0x13); break;      // 4
		  case 0x2E : put_kbbuff(0x14); break;      // 5
		  case 0x36 : put_kbbuff(0x15); break;      // 6
		  case 0x3D : put_kbbuff(0x16); break;      // 7
		  case 0x3E : put_kbbuff(0x17); break;      // 8
		  case 0x46 : put_kbbuff(0x18); break;      // 9
		  case 0x05 : put_kbbuff(0x19); break;      // ?
		  case 0x45 : put_kbbuff(0x20); break;      // 0
		  case 0x0A : put_kbbuff(0x21); break;      // GOTO
		  case 0x03 : put_kbbuff(0x22); break;      // VOL+
		  case 0x0B : put_kbbuff(0x23); break;      // VOL-
		  case 0x83 : put_kbbuff(0x24); break;      // MUTE
		  case 0x75 : put_kbbuff(0x25); break;      // PROG+
		  case 0x72 : put_kbbuff(0x26); break;      // PROG-
		  case 0x06 : put_kbbuff(0x27); break;      // REW
		  case 0x74 : put_kbbuff(0x28); break;      // STOP
		  case 0x0C : put_kbbuff(0x29); break;      // FWD
		  case 0x04 : put_kbbuff(0x30); break;      // PLAY
		  case 0x22 : put_kbbuff(0x31); break;      // REC
		  case 0x6B : put_kbbuff(0x31); break;      // PAUSE
		  */

		  default:
		  // old code
		    put_kbbuff(sc);
		  // new code
		  //  while(!(UCSRA & _BV(UDRE)));	// Wait for empty transmit buffer
			//UDR = sc;
		  // end new code
			break;
        }
    } else {
        is_up = 0;                            // Two 0xF0 in a row not allowed
    }
}

void put_kbbuff(unsigned char c)
{
    if (buffcnt<BUFF_SIZE)                        // If buffer not full
    {
        *inpt=c;                                // Put character into buffer
        inpt++;                                    // Increment pointer
        buffcnt++;
        if (inpt >= (ps2_buffer + BUFF_SIZE)) inpt = ps2_buffer;
    }
}

unsigned char getps2char(void)
{
    unsigned char byte;

    while(buffcnt==0);                        // Wait for data

    byte = *outpt;                                // Get byte
    outpt++;                                    // Increment pointer
    if (outpt >= (ps2_buffer + BUFF_SIZE)) outpt = ps2_buffer;
    buffcnt--;                                    // Decrement buffer count
	return byte;
}

unsigned char ps2dataReceived(void)
{
	if (buffcnt==0)
	{
		return 0;
	}else{
		return 1;
	}
}
