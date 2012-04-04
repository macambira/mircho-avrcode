#ifndef MAIN_H
#define MAIN_H

#define NUMBEROFELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

typedef struct _bitfield
{
	unsigned char bit0:1;
	unsigned char bit1:1;
	unsigned char bit2:1;
	unsigned char bit3:1;
	unsigned char bit4:1;
	unsigned char bit5:1;
	unsigned char bit6:1;
	unsigned char bit7:1;
} bitfield;

#define REGISTER_BIT(rg,bt) ((volatile bitfield*)&rg)->bit##bt 

typedef struct _feed
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} feed;


//128 is convinient ~60Hz. one can move up/down this overflow boundary to get more flexible results.
//#define SOFTPWM_OCR 128
//#define SOFTPWM_OCR 80
#define SOFTPWM_OCR 132
//prescaler is 4
#define SOFTPWM_PRESCALER ( _BV( CS10 ) | _BV( CS11 ) )

//16 exponential PWM values
uint8_t ledFadePWMValues[] = {
	0,
	1,
	2,
	3,
	4,
	6,
	9,
	13,
	19,
	27,
	40,
	58,
	84,
	122,
	176,
	255,
	176,
	122,
	84,
	58,
	40,
	27,
	19,
	13,
	9,
	6,
	4,
	3,
	2,
	1,
	0
};

#define PWMValuesLength NUMBEROFELEMENTS(ledFadePWMValues)
#define PWMValuesMAX (PWMValuesLength-1)

#endif
