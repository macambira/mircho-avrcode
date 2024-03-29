#ifndef USI_UART_H
#define USI_UART_H

/*****************************************************************************
*
* Copyright (C) 2003 Atmel Corporation
*
* File          : USI_UART_config.h
* Compiler      : GCC
* Created       : 18.07.2002 by JLL
* Modified      : 02-10-2003 by LTA
*
* Support mail  : avr@atmel.com
*
* AppNote       : AVR307 - Half duplex UART using the USI Interface
*
* Description   : Header file for USI_UART driver
*
*
****************************************************************************/

//********** USI UART Defines **********//

//#define SYSTEM_CLOCK             14745600
//#define SYSTEM_CLOCK             11059200
//#define SYSTEM_CLOCK              8000000
//#define SYSTEM_CLOCK              7372800
#define SYSTEM_CLOCK          (3686400UL)
//#define SYSTEM_CLOCK              2000000
//#define SYSTEM_CLOCK              1843200
//#define SYSTEM_CLOCK              1000000

//#define BAUDRATE                   115200
//#define BAUDRATE                    57600
//#define BAUDRATE                    28800
#define BAUDRATE                (19200UL)
//#define BAUDRATE                    14400
//#define BAUDRATE                     9600
//#define BAUDRATE                     4800
//#define BAUDRATE 2400
//#define BAUDRATE 1200

#define TIMER_PRESCALER           (1UL)
//#define TIMER_PRESCALER           (8UL)

#define UART_RX_BUFFER_SIZE        (8)    /* 2,4,8,16,32,64,128 or 256 bytes */
#define UART_TX_BUFFER_SIZE        (16)


//********** USI_UART Prototypes **********//

unsigned char		Bit_Reverse( unsigned char );

extern void			USI_UART_Flush_Buffers( void );
extern void			USI_UART_Initialise_Receiver( void );

extern void			USI_UART_Initialise_Transmitter( void );
extern void			USI_UART_Transmit_Byte( unsigned char );

extern unsigned char	USI_UART_Receive_Byte( void );
extern unsigned char	USI_UART_Data_In_Receive_Buffer( void );

#endif
