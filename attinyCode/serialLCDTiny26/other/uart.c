/*
 * uart.c
 *
 * uart routines for lcd terminal on ATTiny2313
 *
 * Copyright (C) 2005 Jos Hendriks <jos.hendriks@zonnet.nl>
 *
 */

/* Includes */
#include <avr/io.h>
#include <avr/signal.h>

/* UART Buffer Defines */
#define UART_RX_BUFFER_SIZE 16     /* 2,4,8,16,32,64,128 or 256 bytes */

#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1 )
#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
	#error RX buffer size is not a power of 2
#endif

/* Static Variables */
static unsigned char UART_RxBuf[UART_RX_BUFFER_SIZE];
static volatile unsigned char UART_RxHead;
static volatile unsigned char UART_RxTail;

/* Initialize UART */
void initUART(unsigned char ubrrh, unsigned char ubrrl)
{
	unsigned char x;
	
	UBRRH = ubrrh;	
	UBRRL = ubrrl;

	/* Enable UART receiver and transmitter, and receive interrupt */
	UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);	// Rx Complete Interrupt & Enable Rx/Tx
	UCSRC = _BV(UCSZ1) | _BV(UCSZ0);	// 8N1
	
	x = 0; 			    /* Flush receive buffer */

	UART_RxTail = x;
	UART_RxHead = x;
}

SIGNAL(SIG_USART0_RX)
{
	unsigned char data;
	unsigned char tmphead;

	data = UDR;                 // Read the received data
	// Calculate buffer index
	tmphead = ( UART_RxHead + 1 ) & UART_RX_BUFFER_MASK;
	UART_RxHead = tmphead;      // Store new index

	if ( tmphead == UART_RxTail )
	{
		// ERROR! Receive buffer overflow
	}
	
	UART_RxBuf[tmphead] = data; // Store received data in buffer 
}

/* Read and write functions */
unsigned char receiveByte( void )
{
	unsigned char tmptail;
	
	while ( UART_RxHead == UART_RxTail );  /* Wait for incomming data */
	
	tmptail = ( UART_RxTail + 1 ) & UART_RX_BUFFER_MASK;/* Calculate buffer index */
	
	UART_RxTail = tmptail;                /* Store new index */
	
	return UART_RxBuf[tmptail];           /* Return data */
}

unsigned char UARTdataReceived( void )
{
	if ( UART_RxHead == UART_RxTail )
	{
		return 0;
	}else{
		return 1;
	}
}
