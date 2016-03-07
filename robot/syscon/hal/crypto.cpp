#include <stdint.h>

extern "C" {
  #include "nrf.h" 
  #include "nrf_ecb.h"

  #include "nrf_sdm.h"
  #include "nrf_soc.h"
}

#include "sha1.h"

#include "crypto.h"
#include "timer.h"
#include "rtos.h"
#include "bignum.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Top 16 bytes of application space
static uint32_t* AES_KEY = (uint32_t*) 0x1EFF0;

static volatile int fifoHead;
static volatile int fifoTail;
static volatile int fifoCount;
static CryptoTask fifoQueue[MAX_CRYPTO_TASKS];

static void aes_key_init() {
  for (int i = 0; i < 4; i++) {
    if (AES_KEY[i] != 0xFFFFFFFF) {
      return ;
    }
  }

  uint32_t key[4];
  Crypto::random(&key, sizeof(key));
  
  // Write AES key to the designated flash sector
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  for (int i = 0; i < sizeof(key) / sizeof(uint32_t); i++) {
    AES_KEY[i] = key[i];
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  }
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
}

static inline void delay() {
  __asm { WFI };
}

static inline void aes_setup(ecb_data_t& ecb, const void* key = AES_KEY) {
  memcpy(&ecb.key, key, sizeof(ecb.key));

  // Setup NRF_ECB
  NRF_ECB->ECBDATAPTR = (uint32_t)&ecb;
}

static inline void aes_ecb(ecb_data_t& ecb, const void* in, void* out) {
  memcpy(ecb.cleartext, in, AES_BLOCK_LENGTH);
  NRF_ECB->EVENTS_ENDECB = 0;
  NRF_ECB->TASKS_STARTECB = 1;

  while(NRF_ECB->EVENTS_ENDECB == 0) delay();
  memcpy(out, ecb.ciphertext, AES_BLOCK_LENGTH);
}

// Output must be the length of data + 1 block rounded up (16 bytes)
static inline void aes_encrypt(uint8_t* in, uint8_t* result, int size) {
  ecb_data_t ecb;

  aes_setup(ecb);
    
  Crypto::random(result, AES_BLOCK_LENGTH); // Get IV

  // Generate AES OFB mode stream
  uint8_t* output = result + AES_BLOCK_LENGTH;
  int block = 0;

  while (block < size) {
    aes_ecb(ecb, result + block, output + block);
    block += AES_BLOCK_LENGTH;
  }
  
  // Feed in bits
  while(size-- > 0) {
    *(output++) ^= *(in++);
  }
}

static inline void aes_decrypt(uint8_t* in, uint8_t* out, int size) {
  ecb_data_t ecb;

  aes_setup(ecb);

  int block = 0;

  // Create feedback
  uint8_t feedback[AES_BLOCK_LENGTH]; 
  memcpy(feedback, in, AES_BLOCK_LENGTH);
  in += AES_BLOCK_LENGTH;

  while (block < size) {
    uint8_t *stream = out + block;
    
    aes_ecb(ecb, feedback, stream);
    memcpy(&feedback, stream, AES_BLOCK_LENGTH);
    block += AES_BLOCK_LENGTH;
  }

  // Feed in bits
  while(size-- > 0) {
    *(out++) ^= *(in++);
  }
}

// Step to convert a pin + random to a hash
static void dh_encode_random(big_num_t& result, int pin, const uint8_t* random) {
	ecb_data_t ecb;

	{
		// Hash our pin (keeping only lower portion
		SHA1_CTX ctx;
		sha1_init(&ctx);
		sha1_update(&ctx, (uint8_t*)&pin, sizeof(pin));

    uint8_t sig[SHA1_BLOCK_SIZE];
    sha1_final(&ctx, sig);

		aes_setup(ecb, sig);
	}
	
	result.negative = false;
	result.used = AES_BLOCK_LENGTH / sizeof(big_num_cell_t);
	aes_ecb(ecb, random, result.digits);
}

static void dh_start(DiffieHellman* dh) {
	// Generate our secret
	//Crypto::random(dh->secret, SECRET_LENGTH);
	
	// Encode our secret as an exponent
	big_num_t secret;	

	dh_encode_random(secret, dh->pin, dh->secret);
	mont_power(*dh->mont, dh->state, *dh->gen, secret);
}

static void dh_finish(DiffieHellman* dh, uint8_t* key, int length) {
	// Encode their secret for exponent
	big_num_t temp;	
	
	dh_encode_random(temp, dh->pin, dh->secret);
	mont_power(*dh->mont, dh->state, dh->state, temp);
	mont_from(*dh->mont, temp, dh->state);
	
	memcpy(key, temp.digits, length);
}

void Crypto::init() {
  fifoHead = 0;
  fifoTail = 0;
  fifoCount = 0;

  // Setup key
  aes_key_init();
  
  // Startup AES engine
  NRF_ECB->POWER = 1;
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
	RTOS::EnterCritical();
	int count = fifoCount;
	RTOS::LeaveCritical();

  if (fifoCount >= MAX_CRYPTO_TASKS) {
    return ;
  }

  RTOS::EnterCritical();
  memcpy(&fifoQueue[fifoTail], task, sizeof(CryptoTask));
  fifoTail = (fifoTail+1) % MAX_CRYPTO_TASKS;
  fifoCount++;
  RTOS::LeaveCritical();
}

void Crypto::manage(void) {
	RTOS::EnterCritical();

	CryptoTask* task = &fifoQueue[fifoHead];
	int count = fifoCount;
	RTOS::LeaveCritical();

	// We have no pending messages
	if (count <= 0) {
		return ;
	}

	switch (task->op) {
		case CRYPTO_GENERATE_RANDOM:
			random(task->output, task->length);		
			break ;
		case CRYPTO_AES_ENCRYPT:
			aes_encrypt((uint8_t*)task->input, (uint8_t*)task->output, task->length);
			break ;
		case CRYPTO_AES_DECRYPT:
			aes_decrypt((uint8_t*)task->input, (uint8_t*)task->output, task->length);
			break ;
		case CRYPTO_START_DIFFIE_HELLMAN:
			dh_start((DiffieHellman*) task->input);
			break ;
		case CRYPTO_FINISH_DIFFIE_HELLMAN:
			dh_finish((DiffieHellman*) task->input, (uint8_t*)task->output, task->length);
			break ;
	}

	if (task->callback) {
		task->callback(task);
	}

	// Dequeue message
	RTOS::EnterCritical();
	fifoHead = (fifoHead+1) % MAX_CRYPTO_TASKS;
	fifoCount--;
	RTOS::LeaveCritical();
}

