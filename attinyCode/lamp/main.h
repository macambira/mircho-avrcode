#ifndef MAIN_H
#define MAIN_H

#include <avr/pgmspace.h>

#define NUMBEROFELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

static uint8_t ledFadePWMValues[] PROGMEM = {
	0,
	1,
	2,
	2,
	2,
	3,
	3,
	4,
	5,
	6,
	7,
	8,
	10,
	12,
	14,
	17,
	20,
	24,
	28,
	34,
	40,
	48,
	57,
	68,
	81,
	97,
	116,
	138,
	164,
	196,
	234,
	255
};

#define GetLedPwmValueByIndex(index) (pgm_read_byte(&ledFadePWMValues[index]))

#define PWMValuesLength NUMBEROFELEMENTS(ledFadePWMValues)
#define PWMValuesMAX (PWMValuesLength-1)

#endif //MAIN_H