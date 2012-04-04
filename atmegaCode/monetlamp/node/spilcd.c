/*
 * spilcd.c
 *
 * Created: 28.3.2011 г. 15:13:02
 *  Author: mmirev
 */ 

#include <inttypes.h>

#include "spi.h"
#include "spilcd.h"

/***
 *
 * SPI Code
 *
 ***/

//define protocol to communicate
#define CMD_PREFIX	0xFE
#define CMD_SUFFIX	0xFD

void printChar( uint8_t chr )
{
	spi_putc( chr );
}

void printStr(const uint8_t *s)
{
    uint8_t c;
    while ((c=*s++)) 
      spi_putc(c);
}

void clearScreen( void )
{
	spi_putc( CMD_PREFIX );
	spi_putc( CLEAR );
	spi_putc( CMD_SUFFIX );
}

void moveScreenHome( void )
{
	spi_putc( CMD_PREFIX );
	spi_putc( MOVE_HOME );
	spi_putc( CMD_SUFFIX );
}

void moveCursorTo( uint8_t row, uint8_t col )
{
	spi_putc( CMD_PREFIX );
	spi_putc( MOVE_TO );
	spi_putc( row );
	spi_putc( col );
	spi_putc( CMD_SUFFIX );
}

void hideCursor( void )
{
	spi_putc( CMD_PREFIX );
	spi_putc( CURSOR_INVISIBLE );
	spi_putc( CMD_SUFFIX );
}

void showCursor( void )
{
	spi_putc( CMD_PREFIX );
	spi_putc( CURSOR_VISIBLE_UNDERLINE );
	spi_putc( CMD_SUFFIX );
}

#ifdef LCD_ATTACHED
void printToLCDAt( uint8_t row, uint8_t col, const uint8_t *str )
{
    moveCursorTo( row, col );
    printStr( str );
}
#endif
