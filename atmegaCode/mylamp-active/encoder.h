/*
 * encoder.h
 *
 * Author: mmirev
 */ 


#ifndef ENCODER_H_
#define ENCODER_H_

#include "event.h"

//this makes for the direction of the movement of the encoder
#define DIRECTION_NONE 		0
#define DIRECTION_FWD 		1
#define DIRECTION_BACK 		2

#ifdef __cplusplus
extern "C" {
#endif

extern void encoderInit( void );

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_H_ */
