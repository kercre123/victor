#include <string.h>
#include <stdint.h>

extern "C" {
  #include "nrf.h" 
  #include "nrf_sdm.h"
  #include "nrf_soc.h"
}

#include "aes.h"
#include "random.h"
#include "../hal/storage.h"

// Top 16 bytes of application space
void aes_key_init() {
  if (aes_key() != NULL) {
    return ;
  }
  
  uint8_t key[AES_KEY_LENGTH];
  gen_random(key, AES_KEY_LENGTH);
  Storage::set(STORAGE_AES_KEY, key, AES_KEY_LENGTH);
}

const void* aes_key() {
  return Storage::get_lazy(STORAGE_AES_KEY);
}

void aes_ecb(ecb_data_t* ecb) {
  uint8_t softdevice_is_enabled;
  sd_softdevice_is_enabled(&softdevice_is_enabled);

  if (softdevice_is_enabled) {
    sd_ecb_block_encrypt((nrf_ecb_hal_data_t*)ecb);
  } else {
    NRF_ECB->POWER = 1;
    NRF_ECB->ECBDATAPTR = (uint32_t)ecb;
    NRF_ECB->EVENTS_ENDECB = 0;
    NRF_ECB->TASKS_STARTECB = 1;
    while (!NRF_ECB->EVENTS_ENDECB) ;
  }
}

int aes_encode(uint8_t* data, int length) {
  ecb_data_t ecb;

  // This forces the block length to be a multiple of 16
  // while also injecting random numbers into the buffer
  // so we don't get cross talk
  if (length % AES_BLOCK_LENGTH) {
    int pad = AES_BLOCK_LENGTH - (length % AES_BLOCK_LENGTH);
    gen_random(data + length, pad);
    length += AES_BLOCK_LENGTH;
  }

  memcpy(ecb.key, aes_key(), AES_KEY_LENGTH);
  
  gen_random(ecb.cleartext, AES_BLOCK_LENGTH);

  for (int i = 0; i < length; i += AES_BLOCK_LENGTH) {
    aes_ecb(&ecb);
    for (int k = 0; k < AES_BLOCK_LENGTH; k++) {
      ecb.ciphertext[k] ^= data[k];
    }
    memcpy(data, ecb.cleartext, AES_BLOCK_LENGTH);
    memcpy(ecb.cleartext, ecb.ciphertext, AES_BLOCK_LENGTH);
    data += AES_BLOCK_LENGTH;
  }

  memcpy(data, ecb.cleartext, AES_BLOCK_LENGTH);

  return length + AES_BLOCK_LENGTH;
}

int aes_decode(uint8_t* data, int length) {
  ecb_data_t ecb;
  
  memcpy(ecb.key, aes_key(), AES_KEY_LENGTH);
  memcpy(ecb.cleartext, data, AES_BLOCK_LENGTH);

  uint8_t *block = data + AES_BLOCK_LENGTH;
  for(int i = AES_BLOCK_LENGTH; i < length; i += AES_BLOCK_LENGTH) {
    aes_ecb(&ecb);
    memcpy(ecb.cleartext, block, AES_BLOCK_LENGTH);

    for (int bi = 0; bi < AES_BLOCK_LENGTH; bi++) {
      *(data++) = *(block++) ^ ecb.ciphertext[bi];
    }
  }
  
  return length - AES_BLOCK_LENGTH;
}
