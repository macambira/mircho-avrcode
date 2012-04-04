/*
 * brightness.h
 *
 * Created: 10.3.2011 г. 16:07:12
 *  Author: mmirev
 */ 
#ifndef BRIGHTNESS_H_
#define BRIGHTNESS_H_

#include "event.h"

//the queue manipulation routines
#define MAX_QUEUE	64
#define QUEUE_FULL	(brightnessQueue.queueSize == MAX_QUEUE)
#define QUEUE_SIZE	(brightnessQueue.queueSize)
#define QUEUE_EMPTY	(QUEUE_SIZE == 0)

//extern uint16_t ledFadePWMValues[];

typedef union _brightnessVal{
	struct
	{
		uint8_t brightness;
		uint8_t delay;
	};
	uint16_t bd;
} brightVal;

typedef struct _bQueue
{
	uint8_t queueStart;
	uint8_t queueEnd;
	uint8_t queueSize;
	brightVal queue[ MAX_QUEUE ];
} bQueue;

extern bQueue brightnessQueue;

typedef struct 
{
	union
	{
		struct 
		{
			uint8_t maxBrightnessReached : 1;
			uint8_t inProgress : 1;
			uint8_t increasing : 1;
			uint8_t flag3 : 1;	
			uint8_t flag4 : 1;	
			uint8_t flag5 : 1;	
			uint8_t flag6 : 1;	
			uint8_t flag7 : 1;	
		};
		uint8_t statusValue;
	} status;
	//repeats of the current brightness
	uint8_t repeats;
	//the index of the current brightness value
	uint8_t brightnessIndex;	
} bsState;

extern bsState brightnessSystemState;

#ifdef __cplusplus
extern "C" {
#endif

#define GetLedPwmValueByIndex(index) (pgm_read_word(&ledFadePWMValues[index]))
extern uint8_t getSizeOfPWMValuesArray( void );
extern uint8_t getMaxPWMValuesIndex( void );

//read brightnessSystemState and use the values there 
extern void brightnessInit( void );
extern void addToBrightnessQueue( uint8_t brightness, uint8_t time );
extern uint8_t getFromBrightnessQueue( brightVal* );
extern void emptyQueue();

extern void incBrightnessIdx( uint8_t /* delay */ );
extern void decBrightnessIdx( uint8_t /* delay */ );
extern void setBrightnessIdx( uint8_t /* brightnessIndex */, uint8_t /* delay */  );
extern void insertBrightnessRange( uint8_t /* brightnessIndexStart */, uint8_t /* brightnessIndexEnd */ );
extern void processQueue( void );

#ifdef __cplusplus
}
#endif

#endif /* BRIGHTNESS_H_ */
