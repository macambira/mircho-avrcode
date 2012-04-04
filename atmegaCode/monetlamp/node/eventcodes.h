/*
 * eventcodes.h
 *
 * Created: 09.11.2011 г. 16:45:43
 *  Author: mmirev
 */ 


#ifndef EVENTCODES_H_
#define EVENTCODES_H_

#include "event.h"

/* monetlamp.c */
DECLARE_EVENT_CODE( SECONDS_TIMER_HANDLER )
DECLARE_EVENT_CODE( UART_DATA_AVAILABLE )

DECLARE_EVENT_CODE( UART_DATA_SEND_TO_SLAVE1 )

DECLARE_EVENT_CODE( STOP_LED_IN_ONE_SECOND )


#endif /* EVENTCODES_H_ */