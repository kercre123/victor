#include <stdint.h>

extern "C" {
  #include "nrf.h" 
  #include "nrf_sdm.h"
  #include "nrf_soc.h"
}

#include "sha1.h"
#include "aes.h"

#include "crypto.h"
#include "timer.h"
#include "rtos.h"
#include "bignum.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static volatile int fifoHead;
static volatile int fifoTail;
static volatile int fifoCount;
static CryptoTask fifoQueue[MAX_CRYPTO_TASKS];

// Step to convert a pin + random to a hash
static void dh_encode_random(big_num_t& result, int pin, const uint8_t* random) {
  nrf_ecb_hal_data_t ecb;

  {
    // Hash our pin (keeping only lower portion
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (uint8_t*)&pin, sizeof(pin));

    uint8_t sig[SHA1_BLOCK_SIZE];
    sha1_final(&ctx, sig);
  
    // Setup ECB
    memcpy(ecb.cleartext, random, AES_BLOCK_LENGTH);
    memcpy(ecb.key, sig, sizeof(ecb.key));
  }

  result.negative = false;
  result.used = AES_BLOCK_LENGTH / sizeof(big_num_cell_t);

  aes_ecb(&ecb);

  memcpy(result.digits, ecb.ciphertext, AES_BLOCK_LENGTH);
}

static void dh_start(DiffieHellman* dh) {
  // Generate our secret
  Crypto::random(&dh->pin, sizeof(dh->pin));
  Crypto::random(dh->local_secret, SECRET_LENGTH);

  // Encode our secret as an exponent
  big_num_t secret;

  dh_encode_random(secret, dh->pin, dh->local_secret);
  mont_power(*dh->mont, dh->state, *dh->gen, secret);
}

static void dh_finish(DiffieHellman* dh) {
  // Encode their secret for exponent
  big_num_t temp;

  temp.negative = false;
  temp.used = sizeof(dh->remote_secret) / sizeof(big_num_cell_t);
  memcpy(temp.digits, dh->remote_secret, sizeof(dh->remote_secret));

  mont_power(*dh->mont, dh->state, dh->state, temp);
  mont_from(*dh->mont, temp, dh->state);

  // Override the secret with 
  nrf_ecb_hal_data_t ecb;
  
  memcpy(ecb.key, temp.digits, AES_KEY_LENGTH);
  memcpy(ecb.cleartext, aes_key(), AES_BLOCK_LENGTH);
  
  aes_ecb(&ecb);

  memcpy(dh->encoded_key, ecb.ciphertext, AES_BLOCK_LENGTH);
}


void Crypto::init() {
  fifoHead = 0;
  fifoTail = 0;
  fifoCount = 0;

  memset(fifoQueue, 0, sizeof(fifoQueue));
  
  // Setup key
  aes_key_init();
}

static inline void sd_rand(void* ptr, int length) {
  uint8_t* data = (uint8_t*)ptr;
  
  while (length > 0) {
    uint8_t avail = 0;
    
    sd_rand_application_bytes_available_get(&avail);

    if (avail == 0) {
      MicroWait(1);
      continue;
    }

    if (avail > length) {
      avail = length;
    }

    sd_rand_application_vector_get((uint8_t*)data, avail);
    data += avail;
    length -= avail;
  }
}

void Crypto::random(void* ptr, int length) {
  uint8_t softdevice_is_enabled;
  sd_softdevice_is_enabled(&softdevice_is_enabled);

  if (softdevice_is_enabled) {
    sd_rand(ptr, length);
  } else {
    uint8_t* data = (uint8_t*)ptr;
    while (length-- > 0) {
      NRF_RNG->EVENTS_VALRDY = 0;
      NRF_RNG->TASKS_START = 1;
      
      while (!NRF_RNG->EVENTS_VALRDY) ;
      *(data++) = NRF_RNG->VALUE;
    }
  }
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

  switch (task->op) {
    case CRYPTO_GENERATE_RANDOM:
      random((uint8_t*)task->state, *task->length);
      break ;
    case CRYPTO_ECB:
      aes_ecb((nrf_ecb_hal_data_t*) task->state);
      break ;
    case CRYPTO_AES_DECODE:
      *task->length = aes_decode((uint8_t*) task->state, *task->length);
      break ;
    case CRYPTO_AES_ENCODE:
      *task->length = aes_encode((uint8_t*) task->state, *task->length);
      break ;
    case CRYPTO_START_DIFFIE_HELLMAN:
      dh_start((DiffieHellman*) task->state);
      break ;
    case CRYPTO_FINISH_DIFFIE_HELLMAN:
      dh_finish((DiffieHellman*) task->state);
      break ;
  }

  if (task->callback) {
    task->callback(task->state);
  }

  // Dequeue message
  fifoHead = (fifoHead+1) % MAX_CRYPTO_TASKS;
}
