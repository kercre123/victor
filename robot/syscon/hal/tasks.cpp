#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern "C" {
  #include "nrf.h" 
  #include "nrf_sdm.h"
  #include "nrf_soc.h"
}
#include "timer.h"

#include "aes.h"
#include "tasks.h"
#include "diffie.h"
#include "random.h"
#include "storage.h"

static volatile int fifoHead;
static volatile int fifoTail;
static Task fifoQueue[MAX_TASKS];
static bool fifoActive[MAX_TASKS];

void Tasks::init() {
  fifoHead = 0;
  fifoTail = 0;

  memset(fifoQueue, 0, sizeof(fifoQueue));
  memset(fifoActive, 0, sizeof(fifoActive));
  
  // Setup key
  if (Tasks::aes_key() != NULL) {
    return ;
  }
  
  uint8_t key[AES_KEY_LENGTH];
  gen_random(key, AES_KEY_LENGTH);
  Storage::set(STORAGE_AES_KEY, key, AES_KEY_LENGTH);
}

// Top 16 bytes of application space
const void* Tasks::aes_key() {
  return Storage::get_lazy(STORAGE_AES_KEY);
}

void Tasks::execute(const Task* task) {
  int index = fifoTail;

  if (fifoActive[fifoTail]) {
    return ;
  }
  
  fifoTail = (fifoTail+1) % MAX_TASKS;

  memcpy(&fifoQueue[index], task, sizeof(Task));
  fifoActive[index] = true;
}

void Tasks::manage(void) {
  Task* task = &fifoQueue[fifoHead];

  // We have no pending messages
  if (!fifoActive[fifoHead]) {
    return ;
  }

  int length = task->length;
  
  switch (task->op) {
    case TASK_GENERATE_RANDOM:
      gen_random((uint8_t*)task->state, task->length);
      break ;
    case TASK_ECB:
      aes_ecb((ecb_data_t*) task->state);
      break ;
    case TASK_AES_DECODE:
      {
        uint8_t* data = (uint8_t*) task->state;

        aes_cfb_decode(aes_key(), data, data + AES_KEY_LENGTH, data, task->length);
        length = task->length - AES_KEY_LENGTH;
        break ;
      }
    case TASK_AES_ENCODE:
      {
        uint8_t* data = (uint8_t*) task->state;
        
        aes_fix_block(data, task->length);
        aes_cfb_encode(aes_key(), data, data, data + AES_KEY_LENGTH, task->length);
        length = task->length + AES_KEY_LENGTH;
      }
      break ;
    case TASK_START_DIFFIE_HELLMAN:
      dh_start((DiffieHellman*) task->state);
      break ;
    case TASK_FINISH_DIFFIE_HELLMAN:
      dh_finish(aes_key(), (DiffieHellman*) task->state);
      break ;
  }

  if (task->callback) {
    task->callback(task->state, length);
  }

  // Dequeue message
  fifoActive[fifoHead] = false;
  fifoHead = (fifoHead+1) % MAX_TASKS;
}
