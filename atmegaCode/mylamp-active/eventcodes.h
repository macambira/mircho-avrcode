﻿/*
 * eventcodes.h
 *
 * Created: 09.11.2011 г. 16:45:43
 *  Author: mmirev
 */ 


#ifndef EVENTCODES_H_
#define EVENTCODES_H_

#include "event.h"

/* main.c */
DECLARE_EVENT_CODE( ADC_HANDLER )
DECLARE_EVENT_CODE( TURN_LAMP_ON_WITH_DELAY_HANDLER )
DECLARE_EVENT_CODE( TURN_LAMP_OFF_WITH_DELAY_HANDLER )
DECLARE_EVENT_CODE( SAVE_BRIGHTNESS_VALUE_HANDLER )
DECLARE_EVENT_CODE( SECONDS_TIMER_HANDLER )
DECLARE_EVENT_CODE( RESET_MANUAL_STATE )

/* encoder.h */
DECLARE_EVENT_CODE( READ_ENCODER_HANDLER )
DECLARE_EVENT_CODE( READ_BUTTON_HANDLER )

DECLARE_EVENT_CODE( BUTTON1_PRESS )
DECLARE_EVENT_CODE( BUTTON2_PRESS )
DECLARE_EVENT_CODE( ENCODER_STATUS_CHANGE )

/* brightness.h */
DECLARE_EVENT_CODE( BRIGHTNESS_CHANGE )
DECLARE_EVENT_CODE( PROCESS_QUEUE_HANDLER )

/* adc.h */
DECLARE_EVENT_CODE( ADC_RUN_HANDLER )
DECLARE_EVENT_CODE( COMPARATOR_FINISH )
DECLARE_EVENT_CODE( TOUCH_PIN_HANDLER )
DECLARE_EVENT_CODE( PIN_TOUCH )

/* indicator.h */
DECLARE_EVENT_CODE( PWM_INDICATOR_HANDLER )
DECLARE_EVENT_CODE( BLINK_HANDLER )


#endif /* EVENTCODES_H_ */
