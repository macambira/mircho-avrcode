/*
 * spilcd.h
 *
 * Created: 28.3.2011 г. 15:11:50
 *  Author: mmirev
 */ 
#ifndef SPILCD_H_
#define SPILCD_H_

//SPI LCD routines
typedef enum _command_set {
		NONE = 0x00,
		CLEAR = 0x01,                                   // clear screen and set cursor to 0x00
		MOVE_HOME = 0x02,                               // set cursor to 0x00
		EM_DECREMENT_OFF = 0x04,                        // move cursor left after write to DDRAM
		EM_DECREMENT_ON = 0x05,                         // move text right after write to DDRAM
		EM_INCREMENT_OFF = 0x06,                        // move cursor left after write to DDRAM
		EM_INCREMENT_ON = 0x07,                         // move text right after write to DDRAM
		BLANK_DISPLAY = 0x08,                           // display on/off
		CURSOR_INVISIBLE = 0x0C,                        // cursor is not visible
		CURSOR_VISIBLE_ALT = 0x0D,                      // cursor visible as alternating block/underline
		CURSOR_VISIBLE_UNDERLINE = 0x0E,				// cursor visible as underline
		CURSOR_VISIBLE_BLOCK = 0x0F,					// cursor visible as blinking block
		MOVE_LEFT = 0x10,                               // move cursor left
		MOVE_RIGHT = 0x14,                              // move cursor right
		SCROLL_LEFT = 0x18,                             // scroll DDRAM left
		SCROLL_RIGHT = 0x1E,                            // scroll DDRAM right
		SET_POSITION = 0x80,                             // sets cursor position on DDRAM (or with function)
		MOVE_TO = 0x81
} command;

extern void printChar( uint8_t );
extern void printStr( const uint8_t* );
extern void clearScreen( void );
extern void moveScreenHome( void );
extern void moveCursorTo( uint8_t, uint8_t );
extern void hideCursor( void );
extern void showCursor( void );


#endif /* SPILCD_H_ */
