#ifndef __AVR_TICK_SCHEDULER
#define __AVR_TICK_SCHEDULER

#include <inttypes.h>

#define TIMING_MILLISECOND_IN_TICKS			(2)
#define TIMING_SECOND_IN_TICKS				(TIMING_MILLISECOND_IN_TICKS * 1000)

#define TS_FREEZE_TASK_FLAG 1
#define TS_UNFREEZE_TASK_FLAG 0

enum {
	TSAddTaskErrorFull = 200,
	TSAddTaskErrorInvalidTask,
	TSAddTaskSuccess,
	TSRemoveTaskErrorInvalidTask,
	TSRemoveTaskErrorPreventedRemoval,
	TSRemoveTaskSuccess,
	TSDelayTaskErrorInvalidTask,
	TSDelayTaskSuccess,
	TSFreezeTaskErrorInvalidTask,
	TSFreezeTaskSuccess,	
	TSDispatchSuccess,
	TSDispatchSkip
};

struct _task;

typedef void (*TS_TASK)( struct _task * );

typedef struct _task {
	TS_TASK	task;
	uint8_t		taskIndex;
	uint16_t	delay;
	uint16_t	period;
	uint8_t		eventCode;
	struct
	{
	  uint8_t active:1;
	  uint8_t frozen:1;
	  uint8_t event:1;
	  uint8_t autofreeze:1;
	  uint8_t flag4:1;
	  uint8_t flag5:1;
	  uint8_t flag6:1;
	  uint8_t flag7:1;		
	} status;
} sTask;

#ifdef __cplusplus
extern "C" {
#endif

extern void TS_init( void );
extern void TS_start( void );
extern void TS_sleep(void);

extern uint8_t TS_addTask( TS_TASK task, uint16_t delay, uint16_t period );
extern uint8_t TS_addTaskThatRaisesEvent( uint8_t eventCode, uint16_t delay, uint16_t period );
extern uint8_t TS_addTaskThatRaisesEventAutofreeze( uint8_t eventCode, uint16_t delay, uint16_t period );
extern uint8_t TS_addTaskEvent( uint8_t taskIndex, uint8_t eventCode );
extern uint8_t TS_setTaskAutofreeze( uint8_t taskIndex, uint8_t autoFreeze );
extern uint8_t TS_removeTask( uint8_t taskIndex );
extern uint8_t TS_delayTask( uint8_t taskIndex, uint16_t delay );
//freezes and unfreezes a task
extern uint8_t TS_freezeTask( uint8_t taskIndex, uint8_t freeze );
extern uint8_t TS_dispatchTasks( void );
extern uint8_t TS_getTaskCount( void );

#define ADD_TASK( task, delay, period ) \
TS_addTask( task, delay, period )

#define ADD_EVENT_TASK( eventCode, delay, period ) \
TS_addTaskThatRaisesEvent( eventCode, delay, period )

#define ADD_EVENT_TASK_AUTOFREEZE( eventCode, delay, period ) \
TS_addTaskThatRaisesEventAutofreeze( eventCode, delay, period )

#define AUTOFREEZE_TASK( task ) \
do{ \
TS_setTaskAutofreeze( task, TRUE ); \
TS_freezeTask( task, TS_FREEZE_TASK_FLAG ); \
} while( 0 );

#define FREEZE_TASK( task ) TS_freezeTask( task, TS_FREEZE_TASK_FLAG )
#define FREEZE_THIS_TASK() FREEZE_TASK( task )
#define UNFREEZE_TASK( task ) TS_freezeTask( task, TS_UNFREEZE_TASK_FLAG )

#ifdef __cplusplus
}
#endif

#endif //__AVR_TICK_SCHEDULER
