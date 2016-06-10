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
#include "rtos.h"

#include "aes.h"
#include "crypto.h"
#include "diffie.h"
#include "random.h"
#include "storage.h"

static volatile int fifoHead;
static volatile int fifoTail;
static CryptoTask fifoQueue[MAX_CRYPTO_TASKS];

void Crypto::init() {
  fifoHead = 0;
  fifoTail = 0;

  memset(fifoQueue, 0, sizeof(fifoQueue));
  
  // Setup key
  if (Crypto::aes_key() != NULL) {
    return ;
  }
  
  uint8_t key[AES_KEY_LENGTH];
  gen_random(key, AES_KEY_LENGTH);
  Storage::set(STORAGE_AES_KEY, key, AES_KEY_LENGTH);
}

// Top 16 bytes of application space
const void* Crypto::aes_key() {
  return Storage::get_lazy(STORAGE_AES_KEY);
}

void Crypto::execute(const CryptoTask* task) {
  if (fifoTail + 1 == fifoHead) {
    return ;
  }

  memcpy(&fifoQueue[fifoTail], task, sizeof(CryptoTask));
  fifoTail = (fifoTail+1) % MAX_CRYPTO_TASKS;
}

void Crypto::manage(void) {
  CryptoTask* task = &fifoQueue[fifoHead];

  // We have no pending messages
  if (fifoHead == fifoTail) {
    return ;
  }

  int length = task->length;
  
  switch (task->op) {
    case CRYPTO_GENERATE_RANDOM:
      gen_random((uint8_t*)task->state, task->length);
      break ;
    case CRYPTO_ECB:
      aes_ecb((ecb_data_t*) task->state);
      break ;
    case CRYPTO_AES_DECODE:
      length = aes_decode(aes_key(), (uint8_t*) task->state, task->length);
      break ;
    case CRYPTO_AES_ENCODE:
      length = aes_encode(aes_key(), (uint8_t*) task->state, task->length);
      break ;
    case CRYPTO_START_DIFFIE_HELLMAN:
      dh_start((DiffieHellman*) task->state);
      break ;
    case CRYPTO_FINISH_DIFFIE_HELLMAN:
      dh_finish(aes_key(), (DiffieHellman*) task->state);
      break ;
  }

  if (task->callback) {
    task->callback(task->state, length);
  }

  // Dequeue message
  fifoHead = (fifoHead+1) % MAX_CRYPTO_TASKS;
}
