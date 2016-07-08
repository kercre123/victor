#ifndef __RNG_H
#define __RNG_H

#include "aes.h"
#include "bignum.h"
#include "diffie.h"

#ifndef MAX
#define MAX(x,y) ((x < y) ? (y) : (x))
#endif

static const int MAX_TASKS = 4;

enum TaskOperation {
  // Generate random number
  TASK_GENERATE_RANDOM,
  
  // AES CFB block mode stuff
  TASK_ECB,
  TASK_AES_DECODE,
  TASK_AES_ENCODE,
  
  // Pin code stuff
  TASK_START_DIFFIE_HELLMAN,
  TASK_FINISH_DIFFIE_HELLMAN
};

typedef void (*task_callback)(const void *state, int length);

struct Task {
  TaskOperation op;
  task_callback callback;
  const void *state;
  int length;
};

namespace Tasks {
  void init();
  void manage();
  void execute(const Task* task);
  const void* aes_key();
}

#endif
