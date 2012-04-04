/*
 * main.c
 *
 * main routines for lcd terminal on ATTiny2313
 *
 * Copyright (C) 2005 Jos Hendriks <jos.hendriks@zonnet.nl>
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include "uart.h"
#include "ps2.h"
#include "lcd.h"
#include "servo.h"

void Delay_10ms(unsigned char t);
static const PROGMEM unsigned char *title =  "   ATTiny2313 AVR  ";
static const PROGMEM unsigned char *status = "      Ready...     ";

#define F_CPU 8000000
#define K_DELAY_10ms	F_CPU/600

void Delay_10ms(unsigned char t) {
  unsigned int i;
  if (t==0) return;
  while (t--) for(i=0;i<K_DELAY_10ms; i++);
}

int main( void )
{
	unsigned char mode, command, rcvdByte, column, pos, temp; 
	Delay_10ms(100);
	lcd_init(LCD_DISP_ON);
    lcd_clrscr();
	
	Delay_10ms(1);
	lcd_gotoxy(0,0);
	lcd_puts(title);
	Delay_10ms(1);
	lcd_gotoxy(0,1);
	lcd_puts(status);
	lcd_gotoxy(0,2);
	
	initServo();
	pos = 0xF0;
	setServo(pos);
	
	initps2();	
	
	initUART(0, 25); // 19200 bps at 8 MHz
	
	sei();
	
	mode = 0;
	command = 0;
	column = 10;
	
	//go to CGRAM
	lcd_command(_BV(LCD_CGRAM));
	// play
	lcd_data(0x10);
	lcd_data(0x18);
	lcd_data(0x1C);
	lcd_data(0x1E);
	lcd_data(0x1C);
	lcd_data(0x18);
	lcd_data(0x10);
	lcd_data(0x00);
	//pause
	lcd_data(0x1B);
	lcd_data(0x1B);
	lcd_data(0x1B);
	lcd_data(0x1B);
	lcd_data(0x1B);
	lcd_data(0x1B);
	lcd_data(0x1B);
	lcd_data(0x00);
	//stop
	lcd_data(0x00);
	lcd_data(0x1F);
	lcd_data(0x1F);
	lcd_data(0x1F);
	lcd_data(0x1F);
	lcd_data(0x1F);
	lcd_data(0x00);
	lcd_data(0x00);
	//return to DDRAM
	lcd_command(_BV(LCD_DDRAM));
	
	PORTB = PORTB | 0x80;
	
	while (1){
		if (UARTdataReceived()){
			rcvdByte = receiveByte();
			switch (rcvdByte)
			{
			case 0xFE :
				mode = 1; break;
			case 'I' :
				if (mode){command = 'I';}
				else{mode = 0;}
				break;
			case 'C' :
				if (mode){command = 'C';}
				else{mode = 0;}
				break;
			case 'E' :
				if (mode){
					lcd_clrscr();
					mode = 0;
				}
				break;
			case 'G' :
				if (mode){command = 'G';}
				else{mode = 0;}
				break;
			case 'L' :
				if (mode){command = 'L';}
				else{mode = 0;}
				break;
			case 'S' :
				if (mode){command = 'S';}
				else{mode = 0;}
				break;
			default :
				if (mode){
					if (command == 0){
						lcd_putc(rcvdByte);
						command = 0;
						mode = 0;
					}
					if (command == 'S'){
						setServo(rcvdByte);
						command = 0;
						mode = 0;
					}
					if (command == 'L'){
						if(rcvdByte == '1'){
							PORTB = PORTB | 0x80;
							command = 0;
							mode = 0;
						}else{
							PORTB = PORTB & 0x7F;
							command = 0;
							mode = 0;
						}
					}
					if (command == 'G'){
						if (column == 10){
							if((0x0 <= rcvdByte) && (rcvdByte < 0x14)){
								column = rcvdByte;
							}else{
								//lcd_putc(rcvdByte);
								column = 10;
								command = 0;
								mode = 0;
							}
						}else{
							if((0x0 <= rcvdByte) && (rcvdByte < 0x04)){
								lcd_gotoxy(column,rcvdByte);
								column = 10;
								command = 0;
								mode = 0;
							}else{
								//lcd_putc(rcvdByte);
								column = 10;
								command = 0;
								mode = 0;
							}
						}
					}
					if (command == 'I'){
						if ((0x08 == rcvdByte) | (0x0C == rcvdByte) | (0x0D == rcvdByte) | (0x0E == rcvdByte) | (0x0F == rcvdByte)){
							lcd_init(rcvdByte);
						}else{
							lcd_putc(rcvdByte);
							command = 0;
							mode = 0;
						}
					}
					if (command == 'C'){
						if ((0x08 == rcvdByte) | (0x0C == rcvdByte) | (0x0D == rcvdByte) | (0x0E == rcvdByte) | (0x0F == rcvdByte)){
							lcd_command(rcvdByte);
						}else{
							lcd_putc(rcvdByte);
							command = 0;
							mode = 0;
						}
					}
				}else{
					lcd_putc(rcvdByte);
					while(!(UCSRA & _BV(UDRE)));	// Wait for empty transmit buffer
					UDR = rcvdByte;
				}
				break;
			}
			
		}		
		
		if (ps2dataReceived()){
			while(!(UCSRA & _BV(UDRE)));	// Wait for empty transmit buffer
			temp = getps2char();
			UDR = temp;
			if (temp == 0x75){
				pos = pos + 0x0A;
				setServo(pos);
			}
			if (temp == 0x72){
				pos = pos - 0x0A;
				setServo(pos);
			}
		}
	}
}
