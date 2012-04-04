#ifndef _AVR_IO_BIT_H_INCLUDED   
#define _AVR_IO_BIT_H_INCLUDED

#include <avr/io.h>

/*
 * Struct Definition
 */
typedef struct {
   unsigned char   bit0:1;
   unsigned char   bit1:1;
   unsigned char   bit2:1;
   unsigned char   bit3:1;
   unsigned char   bit4:1;
   unsigned char   bit5:1;
   unsigned char   bit6:1;
   unsigned char   bit7:1;
} Bit_Field;

/*
 * Union Definition
 */
typedef union {
   unsigned char   byte;
   Bit_Field      bit;
} IO_BYTE;

/*
 * DDRA I/O BIT Definition
 */
#if defined (DDRA)
#define     _DDRA       (*( volatile IO_BYTE* )&DDRA)   
#define       byteDDRA   _DDRA.byte
#define      bitDDRA0   _DDRA.bit.bit0
#define      bitDDRA1   _DDRA.bit.bit1
#define      bitDDRA2   _DDRA.bit.bit2
#define      bitDDRA3   _DDRA.bit.bit3
#define      bitDDRA4   _DDRA.bit.bit4
#define      bitDDRA5   _DDRA.bit.bit5
#define      bitDDRA6   _DDRA.bit.bit6
#define      bitDDRA7   _DDRA.bit.bit7
#endif
/*
 * PORTA I/O BIT Definition
 */
#if defined (PORTA)
#define     _PORTA       (*( volatile IO_BYTE* )&PORTA)   
#define       bytePORTA   _PORTA.byte
#define      bitPORTA0   _PORTA.bit.bit0
#define      bitPORTA1   _PORTA.bit.bit1
#define      bitPORTA2   _PORTA.bit.bit2
#define      bitPORTA3   _PORTA.bit.bit3
#define      bitPORTA4   _PORTA.bit.bit4
#define      bitPORTA5   _PORTA.bit.bit5
#define      bitPORTA6   _PORTA.bit.bit6
#define      bitPORTA7   _PORTA.bit.bit7
#endif
/*
 * PINA I/O BIT Definition
 */
#if defined (PINA)
#define     _PINA       (*( const volatile IO_BYTE* )&PINA)   
#define       bytePINA   _PINA.byte
#define      bitPINA0   _PINA.bit.bit0
#define      bitPINA1   _PINA.bit.bit1
#define      bitPINA2   _PINA.bit.bit2
#define      bitPINA3   _PINA.bit.bit3
#define      bitPINA4   _PINA.bit.bit4
#define      bitPINA5   _PINA.bit.bit5
#define      bitPINA6   _PINA.bit.bit6
#define      bitPINA7   _PINA.bit.bit7
#endif
/*
 * DDRB I/O BIT Definition
 */
#if defined (DDRB)
#define     _DDRB       (*( volatile IO_BYTE* )&DDRB)   
#define       byteDDRB   _DDRB.byte
#define      bitDDRB0   _DDRB.bit.bit0
#define      bitDDRB1   _DDRB.bit.bit1
#define      bitDDRB2   _DDRB.bit.bit2
#define      bitDDRB3   _DDRB.bit.bit3
#define      bitDDRB4   _DDRB.bit.bit4
#define      bitDDRB5   _DDRB.bit.bit5
#define      bitDDRB6   _DDRB.bit.bit6
#define      bitDDRB7   _DDRB.bit.bit7
#endif
/*
 * PORTB I/O BIT Definition
 */
#if defined (PORTB)
#define     _PORTB    (*( volatile IO_BYTE* )&PORTB)   
#define       bytePORTB   _PORTB.byte
#define      bitPORTB0   _PORTB.bit.bit0
#define      bitPORTB1   _PORTB.bit.bit1
#define      bitPORTB2   _PORTB.bit.bit2
#define      bitPORTB3   _PORTB.bit.bit3
#define      bitPORTB4   _PORTB.bit.bit4
#define      bitPORTB5   _PORTB.bit.bit5
#define      bitPORTB6   _PORTB.bit.bit6
#define      bitPORTB7   _PORTB.bit.bit7
#endif
/*
 * PINB I/O BIT Definition
 */
#if defined (PINB)
#define     _PINB       (*( const volatile IO_BYTE* )&PINB)   
#define       bytePINB   _PINB.byte
#define      bitPINB0   _PINB.bit.bit0
#define      bitPINB1   _PINB.bit.bit1
#define      bitPINB2   _PINB.bit.bit2
#define      bitPINB3   _PINB.bit.bit3
#define      bitPINB4   _PINB.bit.bit4
#define      bitPINB5   _PINB.bit.bit5
#define      bitPINB6   _PINB.bit.bit6
#define      bitPINB7   _PINB.bit.bit7
#endif
/*
* DDRC I/O BIT Definition
*/
#if defined (DDRC)
#define     _DDRC       (*( volatile IO_BYTE* )&DDRC)   
#define       byteDDRC   _DDRC.byte
#define      bitDDRC0   _DDRC.bit.bit0
#define      bitDDRC1   _DDRC.bit.bit1
#define      bitDDRC2   _DDRC.bit.bit2
#define      bitDDRC3   _DDRC.bit.bit3
#define      bitDDRC4   _DDRC.bit.bit4
#define      bitDDRC5   _DDRC.bit.bit5
#define      bitDDRC6   _DDRC.bit.bit6
#define      bitDDRC7   _DDRC.bit.bit7
#endif
/*
 * PORTC I/O BIT Definition
 */
#if defined (PORTC)
#define     _PORTC    (*( volatile IO_BYTE* )&PORTC)   
#define       bytePORTC   _PORTC.byte
#define      bitPORTC0   _PORTC.bit.bit0
#define      bitPORTC1   _PORTC.bit.bit1
#define      bitPORTC2   _PORTC.bit.bit2
#define      bitPORTC3   _PORTC.bit.bit3
#define      bitPORTC4   _PORTC.bit.bit4
#define      bitPORTC5   _PORTC.bit.bit5
#define      bitPORTC6   _PORTC.bit.bit6
#define      bitPORTC7   _PORTC.bit.bit7
#endif
/*
 * PINC I/O BIT Definition
 */
#if defined (PINC)
#define     _PINC       (*( const volatile IO_BYTE* )&PINC)   
#define       bytePINC   _PINC.byte
#define      bitPINC0   _PINC.bit.bit0
#define      bitPINC1   _PINC.bit.bit1
#define      bitPINC2   _PINC.bit.bit2
#define      bitPINC3   _PINC.bit.bit3
#define      bitPINC4   _PINC.bit.bit4
#define      bitPINC5   _PINC.bit.bit5
#define      bitPINC6   _PINC.bit.bit6
#define      bitPINC7   _PINC.bit.bit7
#endif
/*
 * DDRD I/O BIT Definition
 */
#if defined (DDRD)
#define     _DDRD       (*( volatile IO_BYTE* )&DDRD)   
#define       byteDDRD   _DDRD.byte
#define      bitDDRD0   _DDRD.bit.bit0
#define      bitDDRD1   _DDRD.bit.bit1
#define      bitDDRD2   _DDRD.bit.bit2
#define      bitDDRD3   _DDRD.bit.bit3
#define      bitDDRD4   _DDRD.bit.bit4
#define      bitDDRD5   _DDRD.bit.bit5
#define      bitDDRD6   _DDRD.bit.bit6
#define      bitDDRD7   _DDRD.bit.bit7
#endif
/*
 * PORTD I/O BIT Definition
 */
#if defined (PORTD)
#define     _PORTD    (*( volatile IO_BYTE* )&PORTD)   
#define       bytePORTD      _PORTD.byte
#define      bitPORTD0   _PORTD.bit.bit0
#define      bitPORTD1   _PORTD.bit.bit1
#define      bitPORTD2   _PORTD.bit.bit2
#define      bitPORTD3   _PORTD.bit.bit3
#define      bitPORTD4   _PORTD.bit.bit4
#define      bitPORTD5   _PORTD.bit.bit5
#define      bitPORTD6   _PORTD.bit.bit6
#define      bitPORTD7   _PORTD.bit.bit7
#endif
/*
 * PIND I/O BIT Definition
 */
#if defined (PIND)
#define     _PIND       (*( const volatile IO_BYTE* )&PIND)   
#define       bytePIND   _PIND.byte
#define      bitPIND0   _PIND.bit.bit0
#define      bitPIND1   _PIND.bit.bit1
#define      bitPIND2   _PIND.bit.bit2
#define      bitPIND3   _PIND.bit.bit3
#define      bitPIND4   _PIND.bit.bit4
#define      bitPIND5   _PIND.bit.bit5
#define      bitPIND6   _PIND.bit.bit6
#define      bitPIND7   _PIND.bit.bit7
#endif
/*
 * DDRE I/O BIT Definition
 */
#if defined (DDRE)
#define     _DDRE       (*( volatile IO_BYTE* )&DDRE)   
#define       byteDDRE   _DDRE.byte
#define      bitDDRE0   _DDRE.bit.bit0
#define      bitDDRE1   _DDRE.bit.bit1
#define      bitDDRE2   _DDRE.bit.bit2
#define      bitDDRE3   _DDRE.bit.bit3
#define      bitDDRE4   _DDRE.bit.bit4
#define      bitDDRE5   _DDRE.bit.bit5
#define      bitDDRE6   _DDRE.bit.bit6
#define      bitDDRE7   _DDRE.bit.bit7
#endif
/*
 * PORTE I/O BIT Definition
 */
#if defined (PORTE)
#define     _PORTE    (*( volatile IO_BYTE* )&PORTE)   
#define       bytePORTE   _PORTE.byte
#define      bitPORTE0   _PORTE.bit.bit0
#define      bitPORTE1   _PORTE.bit.bit1
#define      bitPORTE2   _PORTE.bit.bit2
#define      bitPORTE3   _PORTE.bit.bit3
#define      bitPORTE4   _PORTE.bit.bit4
#define      bitPORTE5   _PORTE.bit.bit5
#define      bitPORTE6   _PORTE.bit.bit6
#define      bitPORTE7   _PORTE.bit.bit7
#endif
/*
 * PINE I/O BIT Definition
 */
#if defined (PINE)
#define     _PINE       (*( const volatile IO_BYTE* )&PINE)   
#define       bytePINE   _PINE.byte
#define      bitPINE0   _PINE.bit.bit0
#define      bitPINE1   _PINE.bit.bit1
#define      bitPINE2   _PINE.bit.bit2
#define      bitPINE3   _PINE.bit.bit3
#define      bitPINE4   _PINE.bit.bit4
#define      bitPINE5   _PINE.bit.bit5
#define      bitPINE6   _PINE.bit.bit6
#define      bitPINE7   _PINE.bit.bit7
#endif
/*
 * DDRF I/O BIT Definition
 */
#if defined (DDRF)
#define     _DDRF       (*( volatile IO_BYTE* )&DDRF)   
#define       byteDDRF   _DDRF.byte
#define      bitDDRF0   _DDRF.bit.bit0
#define      bitDDRF1   _DDRF.bit.bit1
#define      bitDDRF2   _DDRF.bit.bit2
#define      bitDDRF3   _DDRF.bit.bit3
#define      bitDDRF4   _DDRF.bit.bit4
#define      bitDDRF5   _DDRF.bit.bit5
#define      bitDDRF6   _DDRF.bit.bit6
#define      bitDDRF7   _DDRF.bit.bit7
#endif
/*
 * PORTF I/O BIT Definition
 */
#if defined (PORTF)
#define     _PORTF    (*( volatile IO_BYTE* )&PORTF)   
#define       bytePORTF   _PORTF.byte
#define      bitPORTF0   _PORTF.bit.bit0
#define      bitPORTF1   _PORTF.bit.bit1
#define      bitPORTF2   _PORTF.bit.bit2
#define      bitPORTF3   _PORTF.bit.bit3
#define      bitPORTF4   _PORTF.bit.bit4
#define      bitPORTF5   _PORTF.bit.bit5
#define      bitPORTF6   _PORTF.bit.bit6
#define      bitPORTF7   _PORTF.bit.bit7
#endif
/*
 * PINF I/O BIT Definition
 */
#if defined (PINF)
#define     _PINF       (*( const volatile IO_BYTE* )&PINF)     
#define       bytePINF   _PINF.byte
#define      bitPINF0   _PINF.bit.bit0
#define      bitPINF1   _PINF.bit.bit1
#define      bitPINF2   _PINF.bit.bit2
#define      bitPINF3   _PINF.bit.bit3
#define      bitPINF4   _PINF.bit.bit4
#define      bitPINF5   _PINF.bit.bit5
#define      bitPINF6   _PINF.bit.bit6
#define      bitPINF7   _PINF.bit.bit7
#endif
/*
 * DDRG I/O BIT Definition
 */
#if defined (DDRG)
#define     _DDRG       (*( volatile IO_BYTE* )&DDRG)   
#define       byteDDRG   _DDRG.byte
#define      bitDDRG0   _DDRG.bit.bit0
#define      bitDDRG1   _DDRG.bit.bit1
#define      bitDDRG2   _DDRG.bit.bit2
#define      bitDDRG3   _DDRG.bit.bit3
#define      bitDDRG4   _DDRG.bit.bit4
#endif
/*
 * PORTG I/O BIT Definition
 */
#if defined (PORTG)
#define     _PORTG    (*( volatile IO_BYTE* )&PORTG)   
#define       bytePORTG   _PORTG.byte
#define      bitPORTG0   _PORTG.bit.bit0
#define      bitPORTG1   _PORTG.bit.bit1
#define      bitPORTG2   _PORTG.bit.bit2
#define      bitPORTG3   _PORTG.bit.bit3
#define      bitPORTG4   _PORTG.bit.bit4
#endif
/*
 * PING I/O BIT Definition
 */
#if defined (PING)
#define     _PING       (*( const volatile IO_BYTE* )&PING)   
#define       bytePING   _PING.byte
#define      bitPING0   _PING.bit.bit0
#define      bitPING1   _PING.bit.bit1
#define      bitPING2   _PING.bit.bit2
#define      bitPING3   _PING.bit.bit3
#define      bitPING4   _PING.bit.bit4
#endif

#endif /* _AVR_IO_BIT_H_INCLUDED */ 