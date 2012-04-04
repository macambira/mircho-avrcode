/*
 * util.h
 *
 * Created: 10.3.2011 г. 16:08:48
 *  Author: mmirev
 */ 


#ifndef UTIL_H_
#define UTIL_H_

#define NUMBEROFELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

#define TRUE		(1)
#define FALSE		(0)

#define IN	(0)
#define OUT	(1)
#define ON	(1)
#define OFF	(0)
#define PULLUP_ENABLED (1)
#define PULLUP_DISABLED (0)



typedef struct {
  uint8_t pin0:1;
  uint8_t pin1:1;
  uint8_t pin2:1;
  uint8_t pin3:1;
  uint8_t pin4:1;
  uint8_t pin5:1;
  uint8_t pin6:1;
  uint8_t pin7:1;
} pin_t;

typedef struct {
  uint8_t dd0:1;
  uint8_t dd1:1;
  uint8_t dd2:1;
  uint8_t dd3:1;
  uint8_t dd4:1;
  uint8_t dd5:1;
  uint8_t dd6:1;
  uint8_t dd7:1;
} ddr_t;

typedef struct {
  uint8_t port0:1;
  uint8_t port1:1;
  uint8_t port2:1;
  uint8_t port3:1;
  uint8_t port4:1;
  uint8_t port5:1;
  uint8_t port6:1;
  uint8_t port7:1;
} port_t;

#define portb (*((volatile port_t*)&PORTB))
#define pinb (*((volatile pin_t*)&PINB))
#define ddrb (*((volatile ddr_t*)&DDRB))

#define portc (*((volatile port_t*)&PORTC))
#define pinc (*((volatile pin_t*)&PINC))
#define ddrc (*((volatile ddr_t*)&DDRC))

#define portd (*((volatile port_t*)&PORTD))
#define pind (*((volatile pin_t*)&PIND))
#define ddrd (*((volatile ddr_t*)&DDRD))

#endif /* UTIL_H_ */