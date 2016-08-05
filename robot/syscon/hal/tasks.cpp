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

#include "packet.h"
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
  return Storage::get(STORAGE_AES_KEY);
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

static void fix_pin(uint32_t& pin) {
  for (int bit = 0; bit < sizeof(pin) * 8; bit += 4) {
    int nibble = (pin >> bit) & 0xF;
    if (nibble > 0x9) {
      pin += 0x06 << bit;
    }
  }
}

void Tasks::manage(void) {
  Task* task = &fifoQueue[fifoHead];

  // We have no pending messages
  if (!fifoActive[fifoHead]) {
    return ;
  }

  switch (task->op) {
    case TASK_GENERATE_RANDOM:
      {
        RandomTask *random = (RandomTask*) task->state;
        gen_random(random->data, random->length);
        break ;
      }

    case TASK_AES_DECODE:
      {
        AESTask* aes = (AESTask*) task->state;
        const uint8_t* key = (const uint8_t*) aes_key();

        aes->hmac_test = aes_message_decode(key, aes->nonce, aes->nonce_length, aes->data, aes->data_length);
        break ;
      }

    case TASK_AES_ENCODE:
      {
        AESTask* aes = (AESTask*) task->state;
        const uint8_t* key = (const uint8_t*) aes_key();

        aes_message_encode(key, aes->nonce, aes->nonce_length, aes->data, aes->data_length);
      }

      break ;
    case TASK_START_DIFFIE_HELLMAN:
      {
        DiffieHellmanTask* dh = (DiffieHellmanTask*) task->state;

        // Generate our secret
        gen_random(&dh->pin, sizeof(dh->pin));
        fix_pin(dh->pin);
        gen_random(dh->local_secret, SECRET_LENGTH);
        dh_encode_random(dh->local_encoded, dh->pin, dh->local_secret);
        dh_encode_random(dh->remote_encoded, dh->pin, dh->remote_secret);
      }

      break ;
    case TASK_FINISH_DIFFIE_HELLMAN:
      {
        DiffieHellmanTask* dh = (DiffieHellmanTask*) task->state;

        AES128_ECB_encrypt((uint8_t*)aes_key(), dh->diffie_result, dh->encoded_key);
      }
      break ;
  }

  if (task->callback) {
    task->callback(task->state);
  }

  // Dequeue message
  fifoActive[fifoHead] = false;
  fifoHead = (fifoHead+1) % MAX_TASKS;
}
