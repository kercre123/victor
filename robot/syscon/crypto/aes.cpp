#include <string.h>
#include <stdint.h>

extern "C" {
  #include "nrf.h" 
  #include "nrf_sdm.h"
  #include "nrf_soc.h"
}

#include "aes.h"
#include "../hal/crypto.h"

// Top 16 bytes of application space
static uint32_t* AES_KEY = (uint32_t*) 0x1EFF0;

void aes_key_init() {
  for (int i = 0; i < 4; i++) {
    if (AES_KEY[i] != 0xFFFFFFFF) {
      return ;
    }
  }

  uint32_t key[4];
  Crypto::random(&key, sizeof(key));  // This is kinda dumb.
  
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

void aes_ecb(nrf_ecb_hal_data_t* ecb) {
  uint8_t softdevice_is_enabled;
  sd_softdevice_is_enabled(&softdevice_is_enabled);

  if (softdevice_is_enabled) {
    sd_ecb_block_encrypt(ecb);
  } else {
    NRF_ECB->POWER = 1;
    NRF_ECB->ECBDATAPTR = (uint32_t)ecb;
    NRF_ECB->EVENTS_ENDECB = 0;
    NRF_ECB->TASKS_STARTECB = 1;
    while (!NRF_ECB->EVENTS_ENDECB) ;
  }
}

void aes_decode(uint8_t* data, int length) {
  uint8_t *iv = data + length - AES_BLOCK_LENGTH;
  nrf_ecb_hal_data_t ecb;
  
  memcpy(ecb.key, AES_KEY, AES_KEY_LENGTH);

  length -= AES_BLOCK_LENGTH;

  memcpy(ecb.cleartext, iv, AES_BLOCK_LENGTH);
    
  for(int i = 0; i < length; i += AES_BLOCK_LENGTH) {
    aes_ecb(&ecb);
    memcpy(ecb.cleartext, data, AES_BLOCK_LENGTH);
    
    for (int bi = 0; bi < AES_BLOCK_LENGTH; bi++) {
      *(data++) ^= ecb.ciphertext[bi];
    }
  }
}

void aes_encode(uint8_t* data, int length) {
  uint8_t *iv = data + ((length + AES_BLOCK_LENGTH - 1) & ~(AES_BLOCK_LENGTH - 1));

  nrf_ecb_hal_data_t ecb;

  memcpy(ecb.key, AES_KEY, AES_KEY_LENGTH);
  
  Crypto::random(iv, AES_BLOCK_LENGTH);
  memcpy(ecb.cleartext, iv, AES_BLOCK_LENGTH);
  
  for (int i = 0; i < length; i += AES_BLOCK_LENGTH) {
    aes_ecb(&ecb);

    for (int bi = 0; bi < AES_BLOCK_LENGTH; bi++) {
      data[bi] ^= ecb.ciphertext[bi];
    }
    
    memcpy(ecb.cleartext, data, AES_BLOCK_LENGTH);
    data += AES_BLOCK_LENGTH;
  }
}
