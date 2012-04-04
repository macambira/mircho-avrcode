/*
 * uart.h
 *
 * uart routines for lcd terminal on ATTiny2313
 *
 * Copyright (C) 2005 Jos Hendriks <jos.hendriks@zonnet.nl>
 *
 */

void initUART( unsigned char ubrrh, unsigned char ubrrl);
unsigned char receiveByte( void );
unsigned char UARTdataReceived( void );



