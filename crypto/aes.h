#ifndef __AES_H
#define __AES_H

#include <stdint.h>

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

void aes_key_init();
void aes_ecb(ecb_data_t* ecb);
void aes_fix_block(uint8_t* data, int& length);
void aes_cfb_encode(const void* key, void* iv, const uint8_t* data, uint8_t* output, int length);
void aes_cfb_decode(const void* key, const void* iv, const uint8_t* data, uint8_t* output, int length, void* iv_out = NULL);

#endif
