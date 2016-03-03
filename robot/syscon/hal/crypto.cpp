#include <stdint.h>

extern "C" {
  #include "nrf.h" 
  #include "nrf_ecb.h"

  #include "nrf_sdm.h"
  #include "nrf_soc.h"
}

#include "crypto.h"
#include "timer.h"
#include "rtos.h"
#include "bignum.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct ecb_data_t {
  uint8_t key[16];
  uint8_t cleartext[16];
  uint8_t ciphertext[16];
};

static ecb_data_t ecb_data;

static uint32_t* AES_KEY = (uint32_t*) 0x1F0C0; // Reserved section in the bootloader for AES key

static inline void aes_key_init() {
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

void Crypto::init() {
  // Setup key
  aes_key_init();
  memcpy(ecb_data.key, AES_KEY, sizeof(ecb_data.key));

  // Setup AES
  NRF_ECB->POWER = 1;
  NRF_ECB->ECBDATAPTR = (uint32_t)&ecb_data;

  // THIS IS TEMPORARY BULLSHIT HERE
  /*
  NRF_ECB->TASKS_STARTECB = 1;
  while(NRF_ECB->EVENTS_ENDECB == 0) ;
  NRF_ECB->EVENTS_ENDECB = 0;
  */
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

/*
   uint32_t counter = 0x1000000;
   if(src_buf != ecb_cleartext)
   {
     memcpy(ecb_cleartext,src_buf,16);
   }
   NRF_ECB->EVENTS_ENDECB = 0;
   NRF_ECB->TASKS_STARTECB = 1;
   while(NRF_ECB->EVENTS_ENDECB == 0)
   {
    counter--;
    if(counter == 0)
    {
      return false;
    }
   }
   NRF_ECB->EVENTS_ENDECB = 0;
   if(dest_buf != ecb_ciphertext)
   {
     memcpy(dest_buf,ecb_ciphertext,16);
   }
   return true;
*/
