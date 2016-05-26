#ifndef __AES_H
#define __AES_H

#include <stdint.h>
extern "C" {
  #include "nrf_soc.h"
}

static const int AES_KEY_LENGTH = 16;

struct ecb_data_t
{
  uint8_t key[AES_KEY_LENGTH];
  uint8_t cleartext[AES_KEY_LENGTH];
  uint8_t ciphertext[AES_KEY_LENGTH];
};

// Do not use these directly (software fallbacks)
void AES128_ECB_encrypt(uint8_t* input, const uint8_t* key, uint8_t *output);
void AES128_ECB_decrypt(uint8_t* input, const uint8_t* key, uint8_t *output);
void AES128_CBC_encrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
void AES128_CBC_decrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);

void aes_key_init();
void aes_ecb(ecb_data_t* ecb);
int aes_decode(const void* key, uint8_t* data, int length);
int aes_encode(const void* key, uint8_t* data, int length);

#endif
