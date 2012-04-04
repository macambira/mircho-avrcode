#include "event.h"

#define E_MAX_EVENTS 32 //must be a power of 2
#define E_MAX_EVENTS_MASK (E_MAX_EVENTS-1)

static struct _event_data
{
	uint8_t eventQueueStart;
	uint8_t eventQueueEnd;
	uint8_t eventCount;
	event E_Event_Queue[ E_MAX_EVENTS ];
} events = { 0,0,0 };

typedef struct _event_handler
{
	uint8_t eventType;
	E_handler handler;
} eventHandler;

#define E_MAX_HANDLERS 32

static struct _event_handlers
{
	uint8_t eventHandlersCount;
	eventHandler handlers[ E_MAX_HANDLERS ];	
} eventHandlers = { 0, 0 };

uint8_t E_addEvent( uint8_t eventType, uint8_t loParam, uint8_t hiParam )
{
	return E_addEventW( eventType, ( hiParam << 8 | loParam ) );
}

uint8_t E_addEventW( uint8_t eventType, uint16_t wParam )
{
	if( events.eventCount == E_MAX_EVENTS )
	{
		return EAddEventErrorMaxEvents;
	}

	events.E_Event_Queue[ events.eventQueueEnd ].eventType = eventType;
	events.E_Event_Queue[ events.eventQueueEnd ].params.wParam = wParam;
	
	events.eventQueueEnd++;
	events.eventQueueEnd &= E_MAX_EVENTS_MASK;
	events.eventCount++;
	
	return EAddEventSuccess;	
}

uint8_t E_getEventCount( void )
{
	return events.eventCount;
}

void E_dispatchEvents( void )
{
	uint8_t ehIndex;
	event currentEvent;

	while( events.eventCount )
	{
		currentEvent = events.E_Event_Queue[ events.eventQueueStart ];
		events.eventQueueStart++;
		events.eventQueueStart &= E_MAX_EVENTS_MASK;
		events.eventCount--;

		for( ehIndex = 0; ehIndex < E_MAX_HANDLERS; ehIndex++ )
		{
			if( eventHandlers.handlers[ ehIndex ].eventType == currentEvent.eventType )
			{
				eventHandlers.handlers[ ehIndex ].handler( currentEvent );
			}
		}
	}
}


uint8_t E_addEventHandler( E_handler handler, uint8_t eventType )
{
	uint8_t ehIndex;

	if( eventHandlers.eventHandlersCount == E_MAX_HANDLERS )
	{
		return EAddEventHandlerErrorMaxHandlers;
	}
	
	for( ehIndex = 0; ehIndex < E_MAX_HANDLERS; ehIndex++ )
	{
		if( eventHandlers.handlers[ ehIndex ].eventType == 0 )
		{
			eventHandlers.handlers[ ehIndex ].eventType = eventType;
			eventHandlers.handlers[ ehIndex ].handler = handler;
			eventHandlers.eventHandlersCount++;
			break;
		}
	}
	
	return EAddEventHandlerSuccess;
}


uint8_t E_removeEventHandler( E_handler handler, uint8_t eventType )
{
	uint8_t ehIndex;
	
	for( ehIndex = 0; ehIndex < E_MAX_HANDLERS; ehIndex++ )
	{
		if( eventHandlers.handlers[ ehIndex ].eventType == eventType && eventHandlers.handlers[ ehIndex ].handler == handler )
		{
			eventHandlers.handlers[ ehIndex ].eventType = 0;
			eventHandlers.handlers[ ehIndex ].handler = 0;
			eventHandlers.eventHandlersCount--;
			return ERemoveEventHandlerSuccess;
		}
	}
	return ERemoveEventHandlerNoSuchEventHandler;
}
