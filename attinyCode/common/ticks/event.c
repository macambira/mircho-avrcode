#include "event.h"

struct _event_data
{
	uint8_t eventQueueStart;
	uint8_t eventQueueEnd;
	uint8_t eventCount;
	event E_Event_Queue[ E_MAX_EVENTS ];
} events = { 0,0,0 };

uint8_t E_addEvent( uint8_t eventType, uint8_t loParam, uint8_t hiParam )
{

	if( events.eventCount == E_MAX_EVENTS )
	{
		return EAddEventErrorMaxEvents;
	}

	events.E_Event_Queue[ events.eventQueueEnd ].eventType = eventType;
	events.E_Event_Queue[ events.eventQueueEnd ].params.loParam = loParam;
	events.E_Event_Queue[ events.eventQueueEnd ].params.hiParam = hiParam;
	
	events.eventQueueEnd++;
	events.eventQueueEnd = events.eventQueueEnd % E_MAX_EVENTS;
	events.eventCount++;
	
	return EAddEventSuccess;

}

uint8_t E_getEventCount( void )
{
	return events.eventCount;
}


//never call getNextEvent without checking the eventCount before that
//you will get what you have at pEvent otherwise
uint8_t E_getNextEvent( event * pEvent )
{
	if( events.eventCount != 0 )
	{
	
		memcpy( pEvent, &events.E_Event_Queue[ events.eventQueueStart ], sizeof( event ) );
		//pEvent->eventType = events.E_Event_Queue[ events.eventQueueStart ].eventType;
		//pEvent->params.wParam = events.E_Event_Queue[ events.eventQueueStart ].params.wParam;
		
		events.eventQueueStart++;
		events.eventQueueStart = events.eventQueueStart % E_MAX_EVENTS;
		events.eventCount--;
	
	}
	
	return events.eventCount;
}