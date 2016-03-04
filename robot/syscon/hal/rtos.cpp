	#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "hardware.h"
#include "rtos.h"
#include "timer.h"

static RTOS_Task *current_task;
static RTOS_Task *free_task;
static RTOS_Task task_pool[MAX_TASKS];

static int last_counter;

void RTOS::init(void) {
  // Clear out our task management pool
  current_task = NULL;
  free_task = &task_pool[0];

  memset(&task_pool, 0, sizeof(task_pool));
  for (int i = 1; i < MAX_TASKS; i++) {
    task_pool[i-1].next = &task_pool[i];
  }

  // Setup the watchdog
  NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
  NRF_WDT->CRV = 0x8000; // .5s
  NRF_WDT->RREN = wdog_channel_mask;
  //NRF_WDT->TASKS_START = 1;

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

void RTOS::release(RTOS_Task* task) {
  task->next = free_task;
  free_task = task; 
}

static void insert(RTOS_Task* newTask) {
  RTOS_Task *cursor = current_task;
  RTOS_Task *last   = NULL;
  
  // Locate an element we should fire before
  while (cursor && cursor->target <= newTask->target) {
    newTask->target -= cursor->target;

    last = cursor;
    cursor = cursor->next;
  }

  // Insert our task into the chain
  newTask->next = cursor;
  if (last != NULL) {
    last->next = newTask;
  } else {
    current_task = newTask;
  }
 
  // Decrement the tasks after this target
  int ticks = newTask->target;
  while (cursor) {
    cursor->target -= ticks;
    
    if (cursor->target >= 0) {
      break ;
    }
    
    ticks = -cursor->target;
    cursor->target = 0;
  }
}

void RTOS::manage(void) {
	NVIC_SetPendingIRQ(SWI0_IRQn);
}

static inline int queue_length() {
  RTOS_Task *cursor = current_task;
  int length = 0;
  
  while (cursor) {
    cursor = cursor->next;
    length ++;
  }

  return length;
}

RTOS_Task* RTOS::create(RTOS_TaskProc task, bool repeating) {
  RTOS_Task* newTask = allocate();

  if (newTask == NULL) {
    return NULL;
  }

  newTask->task = task;
  newTask->repeating = repeating;
  
  return newTask;
}

void RTOS::start(RTOS_Task* task, int period, void* userdata) {
  stop(task);
  
  task->period = period;
  task->target = period;

  if (task->repeating) 
    task->target += CYCLES_MS(0.5f) * queue_length();

  task->userdata = userdata;

  insert(task);
}

void RTOS::stop(RTOS_Task* task) {
  RTOS_Task** cursor = &current_task;

  while (*cursor) {
    if (*cursor != task) {
      cursor = &(*cursor)->next;
      continue ;
    }

    RTOS_Task* next = (*cursor)->next;
    if (next) {
      next->target += (*cursor)->target;
    }
    *cursor = next;

    return ;
  }
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

  while (current_task) {
    current_task->target -= ticks;

    // Current task has not yet fired
    if (current_task->target > 0) {
      break ;
    }

    // Ticks are the underflow
    ticks = -current_task->target;

    // Return from task queue and fire callback
    RTOS_Task *fired = current_task;
    current_task = current_task->next;

		int start = GetCounter();
    fired->task(fired->userdata);
		fired->time = GetCounter() - start;
		
    // Either release the task slice, or reinsert it with the period
    if (fired->repeating) {
      fired->target = fired->period;
      insert(fired);
    } else {
      RTOS::release(fired);
    }
  }

  RTOS::kick(WDOG_RTOS);
}
