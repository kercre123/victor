#ifndef __AES_H
#define __AES_H

#include <stdint.h>
extern "C" {
  #include "nrf_soc.h"
}

static const int AES_KEY_LENGTH = 16;
static const int AES_BLOCK_LENGTH = 16;

struct ecb_data_t
{
  uint8_t key[AES_KEY_LENGTH];
  uint8_t cleartext[AES_BLOCK_LENGTH];
  uint8_t ciphertext[AES_BLOCK_LENGTH];
};


void aes_key_init();
void aes_ecb(ecb_data_t* ecb);
int aes_decode(const void* key, uint8_t* data, int length);
int aes_encode(const void* key, uint8_t* data, int length);

#endif
