#ifndef __AVR_EVENTS
#define __AVR_EVENTS

#include <inttypes.h>

enum{
	EAddEventErrorMaxEvents = 200,
	EAddEventSuccess,
	EGetNextEventEmptyError,
	EAddEventHandlerErrorMaxHandlers,
	EAddEventHandlerSuccess,
	ERemoveEventHandlerSuccess,
	ERemoveEventHandlerNoSuchEventHandler
};

struct _event;

typedef void (*E_handler)( struct _event );

typedef struct _event {
	uint8_t eventType;
	union {
		struct {
			uint8_t loParam;
			uint8_t hiParam;
		};
		uint16_t wParam;
	} params;
} event;

#define EVENT_PARAM( hiParam, loParam ) ( hiParam << 8 | loParam )

#ifdef __cplusplus
extern "C" {
#endif


uint8_t E_addEvent( uint8_t eventType, uint8_t loParam, uint8_t hiParam );
uint8_t E_addEventW( uint8_t eventType, uint16_t wParam );
uint8_t E_getEventCount( void );

void E_dispatchEvents( void );

uint8_t E_addEventHandler( E_handler handler, uint8_t eventType );
uint8_t E_removeEventHandler( E_handler handler, uint8_t eventType );

#define EVENT_CODE( e ) EVENT_CODE_##e

#define DECLARE_EVENT_CODE( e ) \
extern const uint8_t EVENT_CODE_##e;

#define DEFINE_EVENT_CODE( e ) \
const uint8_t EVENT_CODE_##e = (100+__COUNTER__);

#define DEFINE_EVENT_CODE_CONST( e, const ) \
static const uint8_t EVENT_CODE_##e = (100+(const));

#define DEFINE_EVENT_HANDLER( e ) \
void E_handler_##e( event eventObject )

#define DEFINE_EVENT_CODE_AND_HANDLER( e ) \
static const uint8_t EVENT_CODE_##e = (100+__COUNTER__); \
void E_handler_##e( event eventObject )

#define REGISTER_EVENT_HANDLER( e ) \
E_addEventHandler( E_handler_##e, EVENT_CODE_##e );

#define RAISE_EVENT( e, wparam ) \
E_addEventW( EVENT_CODE_##e, wparam );

#ifdef __cplusplus
}
#endif

#endif
