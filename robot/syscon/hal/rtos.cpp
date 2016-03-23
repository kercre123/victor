#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "hardware.h"
#include "rtos.h"
#include "timer.h"

static RTOS_Task task_pool[MAX_TASKS];

static int last_counter;

void RTOS::init(void) {
  // Clear out our task management pool
  memset(&task_pool, 0, sizeof(task_pool));

  // Setup the watchdog
  NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
  NRF_WDT->CRV = 0x10000 * 60; // 1 minute before watchdog murders your fam
  NRF_WDT->RREN = wdog_channel_mask;
  NRF_WDT->TASKS_START = 1;

  // Manage trigger set
  NVIC_EnableIRQ(SWI0_IRQn);
  NVIC_SetPriority(SWI0_IRQn, RTOS_PRIORITY);
}

void RTOS::kick(uint8_t channel) {
  NRF_WDT->RR[channel] = WDT_RR_RR_Reload;
}

void RTOS::delay(RTOS_Task* task, int delay) {
  task->target += delay;
}

static RTOS_Task* allocate(void) {
  for (int i = 0; i < MAX_TASKS; i++) {
    if (task_pool[i].allocated) {
      continue ;
    }
    
    task_pool[i].allocated = true;
    return &task_pool[i];
  }
  
  return NULL;
}

void RTOS::manage(void) {
  NVIC_SetPendingIRQ(SWI0_IRQn);
}

RTOS_Task* RTOS::create(RTOS_TaskProc func, bool repeating) {
  RTOS_Task* task = allocate();

  if (task == NULL) {
    return NULL;
  }

  task->task = func;
  task->repeating = repeating;
  task->active = false;
 
  return task;
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

RTOS_Task* RTOS::schedule(RTOS_TaskProc func, int period, void* userdata, bool repeating) {
  RTOS_Task* task = create(func, repeating);
  
  if (task == NULL) {
    return NULL;
  }

  start(task, period, userdata);
  
  return task;
}

extern "C" void SWI0_IRQHandler(void) {
  int new_count = GetCounter();
  int ticks = new_count - last_counter;

  last_counter = new_count;
  
  for (int i = 0 ; i < MAX_TASKS; i++) {
    RTOS_Task* const task = &task_pool[i];

    // Resume execution
    if (!task->active || !task->allocated) {
      continue ; 
    }

    task->target -= ticks;

    // Current task has not yet fired
    if (task->target > 0) {
      continue ;
    }

    task->task(task->userdata);

    // Either release the task slice, or reinsert it with the period
    if (task->repeating) {
      do {
        task->target += task->period;
      } while (task->target <= 0);
    } else {
      task->allocated = false;
    }
  }

  RTOS::kick(WDOG_RTOS);
}
