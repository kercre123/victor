#ifndef __RTOS_H
#define __RTOS_H

#include <stddef.h>
#include <stdint.h>

#define CYCLES_MS(ms) (int)(32768 * 256.0f * ms / 1000.0f)
#define MAX(a,b) ((a > b) ? a : b)

static const int MAX_TASKS = 32;

typedef void (*RTOS_TaskProc)(void* userdata);

struct RTOS_Task {
  RTOS_TaskProc task;
  void* userdata;
  
  int target;
  int period;
  bool repeating;

  RTOS_Task *next;
};

namespace RTOS {
  void init(void);
  void run(void);
  void manage(void);
  bool schedule(RTOS_TaskProc task, int period = CYCLES_MS(5.0f), void* userdata = NULL, bool repeating = true);
};

#endif
