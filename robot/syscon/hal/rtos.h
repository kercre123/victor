#ifndef __RTOS_H
#define __RTOS_H

#include <stddef.h>
#include <stdint.h>

#define CYCLES_MS(ms) (int)(32768 * 256.0f * ms / 1000.0f)
#ifndef MAX
#define MAX(a,b) ((a > b) ? a : b)
#endif

static const int MAX_TASKS = 32;

typedef void (*RTOS_TaskProc)(void* userdata);

enum RTOS_Priority {
  RTOS_RADIO_PRIORITY,
  RTOS_HIGH_PRIORITY,
  RTOS_DEFAULT_PRIORITY,
  RTOS_LOW_PRIORITY
};

struct RTOS_Task {
  RTOS_Priority priority;
  RTOS_TaskProc task;
  void* userdata;
  
  int time;
	int target;
  int period;
  bool repeating;
  bool active;

  RTOS_Task *prev;
  RTOS_Task *next;
};

namespace RTOS {
  void init(void);
  void run(void);
  void manage(void);
  void kick(uint8_t channel);
  void EnterCritical(void);
  void LeaveCritical(void);
  
  RTOS_Task* allocate(void);
  void remove(RTOS_Task* task);
  void release(RTOS_Task* task);

  RTOS_Task* create(RTOS_TaskProc task, bool repeating = true);
  void start(RTOS_Task* task, int period = CYCLES_MS(5.0f), void* userdata = NULL);
  void stop(RTOS_Task* task);
	void delay(RTOS_Task* task, int delay);
  void setPriority(RTOS_Task* task, RTOS_Priority priority);
  
  RTOS_Task* schedule(RTOS_TaskProc task, int period = CYCLES_MS(5.0f), void* userdata = NULL, bool repeating = true);
};

#endif
