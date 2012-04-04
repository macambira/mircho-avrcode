/*
 * ps2.h
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

void initps2(void);
void decode(unsigned char sc);
void put_kbbuff(unsigned char c);
unsigned char getps2char(void);
unsigned char ps2dataReceived(void);
