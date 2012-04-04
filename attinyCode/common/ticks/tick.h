#ifndef __AVR_TICK_SCHEDULER
#define __AVR_TICK_SCHEDULER

#include <avr/interrupt.h>
#include <inttypes.h>

#ifndef F_CPU
	#define F_CPU 8000000UL
#endif

#define TS_TIMER TCCR0
#define TS_TIMER_COUNTER TCNT0
#define TS_TIMER_PRESCALER	(_BV(CS00) | _BV(CS02))  //1024, at 8Mhz means 7812.5 ticks per second
#define TS_TIMER_ENABLE() (TIMSK |= _BV( TOIE0 ))
#define TS_TIMER_DISABLE() (TIMSK &= ~_BV( TOIE0 ))
#define TS_TIMER_START() (TS_TIMER=TS_TIMER_PRESCALER)
#define TS_TIMER_STOP() (TS_TIMER=0)
#define TS_TIMER_INIT_VALUE	(255-8) //almost 1ms timer
#define TS_TIMER_SET_START_VALUE() (TS_TIMER_COUNTER=TS_TIMER_INIT_VALUE)

#define TS_MAX_TASKS	5

#define NULL_TASK 0x00

#define TS_RUN_TASK		1
#define TS_DONOTRUN_TASK	0

#define TS_PREVENT_UNRUN_TASK_REMOVAL 1

enum {
	TSAddTaskErrorFull = 200,
	TSAddTaskErrorInvalidTask,
	TSAddTaskSuccess,
	TSRemoveTaskErrorInvalidTask,
	TSRemoveTaskErrorPreventedRemoval,
	TSRemoveTaskSuccess,
	TSDispatchSuccess
};

typedef void ( *TS_TASK )( void );

typedef struct data{
	TS_TASK	task;
	uint16_t	delay;
	uint16_t	period;
	uint8_t	run;
} sTask;

#ifdef __cplusplus
extern "C" {
#endif

void TS_init( void );
void TS_start( void );
void TS_sleep(void);

uint8_t TS_addTask( TS_TASK task, uint16_t delay, uint16_t period );
uint8_t TS_removeTask( uint8_t index );
uint8_t	TS_dispatchTasks( void );
uint8_t TS_getTaskCount( void );

#ifdef __cplusplus
}
#endif

#endif //__AVR_TICK_SCHEDULER