#ifndef __AVR_EVENTS
#define __AVR_EVENTS

#include <inttypes.h>

#define E_MAX_EVENTS 8

enum{
	EAddEventErrorMaxEvents = 200,
	EAddEventSuccess,
	EGetNextEventEmptyError
};

typedef struct{
	uint8_t eventType;
	union {
		uint8_t loParam;
		uint8_t hiParam;
		uint16_t wParam;
	} params;
} event;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t E_addEvent( uint8_t eventType, uint8_t loParam, uint8_t hiParam );
uint8_t E_getEventCount( void );
uint8_t E_getNextEvent( event * pEvent );

#ifdef __cplusplus
}
#endif

#endif