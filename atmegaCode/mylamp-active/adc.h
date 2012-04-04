/*
 * adc.h
 *
 * Created: 29.3.2011 г. 15:53:48
 *  Author: mmirev
 */ 


#ifndef ADC_ATMEGA8_H_
#define ADC_ATMEGA8_H_

#include "eventcodes.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void adcInit( void );
extern void adcEnableAndRun( void );
extern void adcDisable( void );
extern void comparatorCharge( void );
extern void comparatorEnableAndRun( void );
extern void comparatorDisable( void );
extern uint16_t adcGetLastResult( void );
extern uint16_t adcGetMovingAverage( void );

#ifdef __cplusplus
}
#endif

#endif /* ADC_ATMEGA8_H_ */
