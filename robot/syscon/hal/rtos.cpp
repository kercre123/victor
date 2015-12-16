#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

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
}

void RTOS::run(void) {
  last_counter = GetCounter();

  for (;;) {
    __asm { WFI };
    manage();
  }
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

    fired->task(fired->userdata);

    // Either release the task slice, or reinsert it with the period
    if (fired->repeating) {
      fired->target = fired->period;
      insert(fired);
    } else {
      fired->next = free_task;
      free_task = fired;
    }
  }
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

bool RTOS::schedule(RTOS_TaskProc task, int period, void* userdata, bool repeating) {
  if (free_task == NULL) {
    return false;
  }

  // Allocate a task
  RTOS_Task *newTask = free_task;
  free_task = free_task->next;

  newTask->task = task;
  newTask->period = period;
  newTask->userdata = userdata;
  newTask->repeating = repeating;
  newTask->target = period + repeating;

  // This attempts to auto-spread OS tasks
  if (repeating) {
    newTask->target += CYCLES_MS(0.5f) * queue_length();
  }
  
  insert(newTask);
  
  return true;
}
