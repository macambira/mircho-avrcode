/*
 * indicator.h
 *
 *  Author: mmirev
 */ 
#ifndef INDICATOR_H_
#define INDICATOR_H_


#ifdef __cplusplus
extern "C" {
#endif

extern void indicatorInit( void );
extern void indicatorPlay( void );
extern void indicatorStopPlay( void );
extern void indicatorBlink( uint8_t times );

/*
extern void indicatorStartTimer( void );
extern void indicatorStopTimer( void );
extern void indicatorConnectPWMPin( void );
extern void indicatorDisconnectPWMPin( void );
*/

#ifdef __cplusplus
}
#endif

#endif /* INDICATOR_H_ */
