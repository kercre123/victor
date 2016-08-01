#ifndef __RNG_H
#define __RNG_H

#include "aes.h"
#include "bignum.h"
#include "diffie.h"

#ifndef MAX
#define MAX(x,y) ((x < y) ? (y) : (x))
#endif

static const int MAX_TASKS = 4;

struct RandomTask {
  uint8_t* data;
  int      length;
};

struct AESTask {
  uint8_t* data;
  int      data_length;
  uint8_t* nonce;
  int      nonce_length;
  bool     hmac_test;
};

struct DiffieHellmanTask {
  uint32_t          pin;
  uint8_t           local_secret[SECRET_LENGTH];
  uint8_t           remote_secret[SECRET_LENGTH];
  uint8_t           local_encoded[SECRET_LENGTH];
  uint8_t           remote_encoded[SECRET_LENGTH];
  uint8_t           diffie_result[SECRET_LENGTH];
  uint8_t           encoded_key[AES_KEY_LENGTH];
};

enum TaskOperation {
  // Generate random number
  TASK_GENERATE_RANDOM,
  
  // AES CFB block mode stuff
  TASK_AES_DECODE,
  TASK_AES_ENCODE,
  
  // Pin code stuff
  TASK_START_DIFFIE_HELLMAN,
  TASK_FINISH_DIFFIE_HELLMAN
};

typedef void (*task_callback)(const void *state);

struct Task {
  TaskOperation op;
  task_callback callback;
  const void *state;
};

namespace Tasks {
  void init();
  void manage();
  void execute(const Task* task);
  const void* aes_key();
}

#endif
