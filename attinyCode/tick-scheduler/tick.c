#include "tick.h"
#include <util/delay.h>

sTask TS_tasks_G[ TS_MAX_TASKS ];
uint8_t TS_tasks_count;

#define TASK_LIST_FULL() (TS_tasks_count==TS_MAX_TASKS)

void TS_emptyTask( uint8_t index )
{
	TS_tasks_G[ index ].task = NULL_TASK;
	TS_tasks_G[ index ].delay = 0;
	TS_tasks_G[ index ].period = 0;
	TS_tasks_G[ index ].run = TS_DONOTRUN_TASK;
}

void TS_init( void )
{
	uint8_t index = 0;
	while( index < TS_MAX_TASKS ){
		TS_emptyTask( index );
		index++;
	};
}

void TS_start( void )
{
	//start timer, enable interrupt
	TS_TIMER_SET_START_VALUE();
	TS_TIMER_ENABLE();
	TS_TIMER_START();
}

void TS_sleep( void )
{
	//stop timer, disable interrupt
	TS_TIMER_DISABLE();
}

uint8_t TS_addTask( TS_TASK task, uint16_t delay, uint16_t period )
{
	uint8_t index;

	if( TASK_LIST_FULL() )
	{
		return TSAddTaskErrorFull;
	}

	for( index = 0; index < TS_MAX_TASKS; index++ )
	{
		if( TS_tasks_G[ index ].task == NULL_TASK )
		{
			break;
		}
	}
	
	TS_tasks_G[ index ].task = task;
	TS_tasks_G[ index ].delay = delay;
	TS_tasks_G[ index ].period = period;
	TS_tasks_G[ index ].run = ( delay == 0 ) ? TS_RUN_TASK : TS_DONOTRUN_TASK;

	TS_tasks_count++;

	return index;
}

uint8_t TS_removeTask( uint8_t index )
{
	if( ( index > TS_MAX_TASKS - 1 ) || ( TS_tasks_count == 0 ) )
	{
		return TSRemoveTaskErrorInvalidTask;
	}
	
	if( ( TS_tasks_G[ index ].run == TS_RUN_TASK ) && TS_PREVENT_UNRUN_TASK_REMOVAL )
	{
		TS_tasks_G[ index ].period = 0;
		return TSRemoveTaskErrorPreventedRemoval;
	}
	
	TS_emptyTask( index );
	
	TS_tasks_count--;
	return TSRemoveTaskSuccess;
}

uint8_t TS_dispatchTasks( void )
{
	uint8_t index = 0;

	while( index < TS_MAX_TASKS ){
		if( TS_tasks_G[ index ].run == TS_RUN_TASK )
		{
			(TS_tasks_G[ index ].task)();
			TS_tasks_G[ index ].run = TS_DONOTRUN_TASK;
			if( TS_tasks_G[ index ].period == 0 )
			{
				TS_removeTask( index );
			}
		}
		index++;
	};

	return TSDispatchSuccess;

}

uint8_t TS_getTaskCount(void)
{
	return TS_tasks_count;
}

ISR( TIMER0_OVF0_vect )
{
	uint8_t index = 0;

	TS_TIMER_SET_START_VALUE();

	while( index < TS_MAX_TASKS ){
		if( TS_tasks_G[ index ].task != NULL_TASK )
		{
			
			if( TS_tasks_G[ index ].delay == 0 )
			{
				TS_tasks_G[ index ].run = TS_RUN_TASK;
				if( TS_tasks_G[ index ].period > 0 )
				{
					//non zero period
					TS_tasks_G[ index ].delay = TS_tasks_G[ index ].period;
				}
			}
			TS_tasks_G[ index ].delay--;
		}
		index++;
	};

}
