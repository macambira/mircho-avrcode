#ifndef _EZ_LCD_DEVICE
#define _EZ_LCD_DEFINE

#include <avr/io.h>                     /* Device specific include file */

#define	IF_BUS       4                  /* Bus width */

#define	IF_INIT()    {PORTA = 0b00111111; DDRA = 0b00111111;} /* Initialize control port (can be blanked if initialized by other code) */
#define E1_HIGH()    PORTA |=  _BV( PA3 )		/* Set E high */
#define E1_LOW()     PORTA &= ~_BV( PA3 )		/* Set E low */
#define	RS_HIGH()    PORTA |=  _BV( PA2 )		/* Set RS high */
#define	RS_LOW()     PORTB &= ~_BV( PA2 )		/* Set RS low */
#define	OUT_DATA(d)  PORTB = (PORTB & 0x0F) | (d & 0xF0)  /* Put d on the data buf (upper 4 bits in 4-bit mode) */

#define	IF_DLY60()                      /* Delay >60ns for RS to E (can be blanked on most uC) */
#define	IF_DLY450()  {PINB; PINB;}      /* Delay >=450ns@3V, >=250ns@5V for E pulse */
#define DELAY_US(n)  lcd_delay_us(n)    /* Delay in unit of microsecond (defined below) */

static void lcd_delay_us (uint16_t n)
{
    do {   /* 8 clocks per loop for Atmel AVR/8MHz */
        PINB; PINB; PINB; PINB;
    } while (--n);
}

#endif	/* #ifndef _EZ_LCD_DEFINE */
