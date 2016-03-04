	#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "hardware.h"
#include "rtos.h"
#include "timer.h"

static RTOS_Task *task_list;
static RTOS_Task *free_task;
static RTOS_Task task_pool[MAX_TASKS];

static int last_counter;

void RTOS::init(void) {
  // Clear out our task management pool
  task_list = NULL;
  free_task = &task_pool[0];

  memset(&task_pool, 0, sizeof(task_pool));
  for (int i = 1; i < MAX_TASKS; i++) {
    task_pool[i-1].next = &task_pool[i];
  }

  // Setup the watchdog
  NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
  NRF_WDT->CRV = 0x8000; // .5s
  NRF_WDT->RREN = wdog_channel_mask;
  NRF_WDT->TASKS_START = 1;

	// Manage trigger set
	NVIC_EnableIRQ(SWI0_IRQn);
	NVIC_SetPriority(SWI0_IRQn, 3);
}

void RTOS::kick(uint8_t channel) {
  NRF_WDT->RR[channel] = WDT_RR_RR_Reload;
}

RTOS_Task* RTOS::allocate(void) {
  if (free_task == NULL) {
    return NULL;
  }
  
  RTOS_Task *task = free_task;
  free_task = free_task->next;
  task->next = NULL;

  return task;
}

void RTOS::release(RTOS_Task** task) {
  RTOS_Task* released = *task;
	
	*task = (*task)->next;
	released->next = free_task;
  free_task = released; 
}

static void insert(RTOS_Task* newTask) {
  RTOS_Task **cursor = &task_list;
  
	while (*cursor != NULL) {
		cursor = &(*cursor)->next;
	}
	
	*cursor = newTask;	
}

void RTOS::manage(void) {
	NVIC_SetPendingIRQ(SWI0_IRQn);
}

RTOS_Task* RTOS::create(RTOS_TaskProc task, bool repeating) {
  RTOS_Task* newTask = allocate();

  if (newTask == NULL) {
    return NULL;
  }

  newTask->task = task;
  newTask->repeating = repeating;
	newTask->active = false;

  insert(newTask);
 
	return newTask;
}

void RTOS::start(RTOS_Task* task, int period, void* userdata) {
  task->period = period;
  task->target = period;
  task->userdata = userdata;
	task->active = true;
}

void RTOS::stop(RTOS_Task* task) {
	task->active = false;
}

void RTOS::EnterCritical(void) {
	NVIC_DisableIRQ(SWI0_IRQn);
}

void RTOS::LeaveCritical(void) {
	NVIC_EnableIRQ(SWI0_IRQn);
}

RTOS_Task* RTOS::schedule(RTOS_TaskProc task, int period, void* userdata, bool repeating) {
  RTOS_Task* newTask = create(task, repeating);
  
  if (free_task == NULL) {
    return NULL;
  }

  start(newTask, period, userdata);
  
  return newTask;
}

extern "C" void SWI0_IRQHandler(void) {
  int new_count = GetCounter();
  int ticks = new_count - last_counter;

  last_counter = new_count;
	
  for (RTOS_Task** cursor = &task_list; *cursor; cursor = &(*cursor)->next) {
		RTOS_Task* task = *cursor;

		// Resume execution
		if (!task->active) {
			continue ; 
		}		
    task->target -= ticks;

    // Current task has not yet fired
    if (task->target > 0) {
      continue ;
    }
		
    // Ticks are the underflow
    ticks = -task->target;
		task->target = 0;

		task->task(task->userdata);

    // Either release the task slice, or reinsert it with the period
    if (task->repeating) {
      task->target += task->period;
    } else {
      RTOS::release(cursor);
    }
  }

  RTOS::kick(WDOG_RTOS);
}
