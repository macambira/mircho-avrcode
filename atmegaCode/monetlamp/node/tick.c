#include <string.h>
#include <avr/interrupt.h>

#include "tick_def.h"
#include "tick.h"
#include "util.h"
#include "event.h"

volatile static uint8_t doDispatch = FALSE;

#define SET_TICK_DISPATCH()\
doDispatch = TRUE

#define CLEAR_TICK_DISPATCH()\
doDispatch = FALSE

//static uint8_t phaseAccumulator = 0;
//calculation of phase accumulation
/**
* http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=84036&start=0
*

phase accumulation is simple. You know your input frequency, and you know your target output frequency. You also know that in a single cycle you cannot achieve exactly the desired output frequency. However, you can calculate the error in the actual output vs the desired output, and adjust the cycle time in order to achieve an overall average that is much closer to the target frequency than is otherwise possible.

For example. Given your 4MHz input you would need to be able to divide by 105.26 in order to get 38KHz. But as we can only divide by integer factors, we can use 105, for an output of 38095Hz, or 106 for an output of 37735Hz. Obviously if we use a static divisor, we would pick 105, as it is the closest. But as we can be dynamic, we could use 105 for say 3/4 cycles, and use 106 for one cycle. This would give us an average frequency of 38005Hz over the 4 cycles.

You could keep it that simple, or use a larger, and complete, error accumulator to get even more accurate over longer periods.

For that, you would use fixed point math. The accuracy of your accumulator will depend on the error you need to accumulate, and the number of bits you devote to it. For this example, we'll keep it a simple 8 bit value, with the 7 (least significant) bits devoted to the fractional component. So in this case our error is 0.26 cycles per cycle (obtained by subtracting our actual divisor, from the calculated ideal value above). We then take that and multiply it by the weight of the most significant bit, thus 128 * 0.26 giving us 33 (rounded off). Now we simply add 33 to our accumulator variable on each pass, if the result is > 128, we use the 106 divisor, otherwise we use 105. Also, when we exceed the 128 value, we subtract 128 from it, and continue on. This way any residual error is maintained and used in the next round. By accumulating the error, our average frequency will be even closer to the ideal frequency over long periods of time.

*
*
*
**/

inline static void setTimerStartValue() __attribute__((always_inline));
inline static void setTimerStartValue()
{
	static uint8_t phaseAccumulator = 0;
	if( phaseAccumulator & 0b10000000 )
	{
		TS_TIMER_LOAD_CORRECTED_VALUE();
		phaseAccumulator &= 0b01111111; //same as phaseAccumulator-128
	}
	else
	{
		TS_TIMER_LOAD_VALUE();
	}
	phaseAccumulator += TS_TIMER_PHASE_ACCUMULATION;
}

//#define TS_TIMER_SET_START_VALUE() (TS_TIMER_COUNTER=TS_TIMER_INIT_VALUE)
#define TS_TIMER_SET_START_VALUE() setTimerStartValue()


/**
 * tick scheduler part
 */

//define the maximum number of tasks to be run
#define TS_MAX_TASKS	(12)
//the "null" task
#define NULL_TASK (TS_TASK)(0x00)
//whether not to remove tasks that have not run yet
#define TS_PREVENT_UNRUN_TASK_REMOVAL TRUE

static sTask TS_tasks_G[ TS_MAX_TASKS ];
static uint8_t TS_tasks_count;

#define TASK_LIST_FULL() ( TS_tasks_count == TS_MAX_TASKS )

void TS_initTimer( void )
{
}

void TS_emptyTask( uint8_t taskIndex )
{
	memset( &(TS_tasks_G[ taskIndex ]), 0, sizeof( sTask ) );
}


void TS_initTasks( void )
{
	memset( TS_tasks_G, 0, sizeof( TS_tasks_G ) );
	TS_tasks_count = 0;
}

void TS_init( void )
{
	TS_initTasks();
	TS_initTimer();
}

void TS_start( void )
{
	//start timer, enable interrupt
	TS_TIMER_SET_START_VALUE();
	TS_TIMER_INT_ENABLE();
	TS_TIMER_START();
}

void TS_sleep( void )
{
	//disable interrupt
	TS_TIMER_INT_DISABLE();
}

uint8_t TS_findEmptyTaskSlot()
{
	uint8_t index;
	for( index = 0; index < TS_MAX_TASKS; index++ )
	{
		if( ( TS_tasks_G[ index ].task == NULL_TASK ) && ( TS_tasks_G[ index ].status.event != TRUE ) )
		{
			break;
		}
	}
	return index;
}

uint8_t TS_addTask( TS_TASK task, uint16_t delay, uint16_t period )
{
	uint8_t index;
	sTask *currentTask = NULL;

	if( TASK_LIST_FULL() )
	{
		return TSAddTaskErrorFull;
	}

	index = TS_findEmptyTaskSlot();
	
	if( index < TS_MAX_TASKS )
	{
		currentTask = &TS_tasks_G[ index ];
		currentTask->taskIndex = index;
		currentTask->task = task;
		currentTask->delay = delay ? delay : period;
		currentTask->period = period;
		currentTask->status.active = TRUE;
		TS_tasks_count++;
	}

	return index;
}

uint8_t TS_addTaskThatRaisesEvent( uint8_t eventCode, uint16_t delay, uint16_t period )
{
	uint8_t index;
	sTask *currentTask = NULL;

	if( TASK_LIST_FULL() )
	{
		return TSAddTaskErrorFull;
	}

	index = TS_findEmptyTaskSlot();

	if( index < TS_MAX_TASKS )
	{
		currentTask = &TS_tasks_G[ index ];
		currentTask->taskIndex = index;
		currentTask->task = NULL_TASK;
		currentTask->delay = delay ? delay : period;
		currentTask->period = period;
		currentTask->eventCode = eventCode;
		currentTask->status.event = TRUE;
		currentTask->status.active = TRUE;
		TS_tasks_count++;
	}

	return index;
}

uint8_t TS_addTaskEvent( uint8_t taskIndex, uint8_t eventCode )
{
	sTask *currentTask = NULL;
	currentTask = &TS_tasks_G[ taskIndex ];
	currentTask->eventCode = eventCode;
	currentTask->status.event = TRUE;
	return taskIndex;
}

extern uint8_t TS_setTaskAutofreeze( uint8_t taskIndex, uint8_t autoFreeze )
{
	sTask *currentTask = NULL;
	currentTask = &TS_tasks_G[ taskIndex ];
	currentTask->status.autofreeze = autoFreeze;
	return taskIndex;
}

extern uint8_t TS_addTaskThatRaisesEventAutofreeze( uint8_t eventCode, uint16_t delay, uint16_t period )
{
	uint8_t taskId;
	taskId = TS_addTaskThatRaisesEvent( eventCode, delay, period );
	TS_setTaskAutofreeze( taskId, TRUE );
	TS_freezeTask( taskId, TS_FREEZE_TASK_FLAG );
	return taskId;
}

uint8_t TS_removeTask( uint8_t taskIndex )
{
	sTask * currentTask;

	if( ( taskIndex > TS_MAX_TASKS - 1 ) || ( TS_tasks_count == 0 ) )
	{
		return TSRemoveTaskErrorInvalidTask;
	}

	currentTask = &TS_tasks_G[ taskIndex ];

	if( ( currentTask->delay > 0 ) && TS_PREVENT_UNRUN_TASK_REMOVAL )
	{
		currentTask->period = 0;
		return TSRemoveTaskErrorPreventedRemoval;
	}

	TS_emptyTask( taskIndex );

	TS_tasks_count--;
	return TSRemoveTaskSuccess;
}

uint8_t TS_delayTask( uint8_t taskIndex, uint16_t delay )
{
	sTask *currentTask;

	if( ( taskIndex > TS_MAX_TASKS - 1 ) || ( TS_tasks_count == 0 ) )
	{
		return TSDelayTaskErrorInvalidTask;
	}

	currentTask = &TS_tasks_G[ taskIndex ];
	currentTask->delay = delay;

	return TSDelayTaskSuccess;
}

uint8_t TS_freezeTask( uint8_t taskIndex, uint8_t freeze )
{
	sTask *currentTask;

	if( ( taskIndex > TS_MAX_TASKS - 1 ) || ( TS_tasks_count == 0 ) )
	{
		return TSFreezeTaskErrorInvalidTask;
	}

	currentTask = &TS_tasks_G[ taskIndex ];
	currentTask->status.frozen = freeze;

	return TSFreezeTaskSuccess;
}

uint8_t TS_dispatchTasks( void )
{
	static uint8_t taskIndex;
	sTask *currentTask;
	if( doDispatch == TRUE )
	{
		for( taskIndex = 0; taskIndex < TS_MAX_TASKS; taskIndex++ )
		{
			currentTask = &TS_tasks_G[ taskIndex ];
			if( ( currentTask->status.active == TRUE ) && ( !currentTask->status.frozen ) )
			{
				if( currentTask->delay == 0 )
				{
					//execute the task here
					if( currentTask->status.event == TRUE )
					{
						E_addEvent( currentTask->eventCode, taskIndex, 0 );
					}
					else
					{
						(currentTask->task)( currentTask );
					}

					if( currentTask->period == 0 )
					{
						TS_removeTask( taskIndex );
					}
					else
					{
						if( currentTask->status.autofreeze )
						{
							TS_freezeTask( taskIndex, TS_FREEZE_TASK_FLAG );
						}
						currentTask->delay = currentTask->period;
					}
				}
				currentTask->delay--;
			}
		};
		CLEAR_TICK_DISPATCH();
		return TSDispatchSuccess;
	};
	return TSDispatchSkip;
}

uint8_t TS_getTaskCount(void)
{
	return TS_tasks_count;
}

ISR( TS_INT_VECTOR )
{
	SET_TICK_DISPATCH();
	TS_TIMER_SET_START_VALUE();
}
