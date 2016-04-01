#ifndef __AES_H
#define __AES_H

#include <stdint.h>
extern "C" {
  #include "nrf_soc.h"
}

// These should always be the same length
static const int AES_KEY_LENGTH = 16;
static const int AES_BLOCK_LENGTH = 16;

void aes_key_init();
void aes_ecb(nrf_ecb_hal_data_t* ecb);
int aes_decode(uint8_t* data, int length);
int aes_encode(uint8_t* data, int length);

#endif
