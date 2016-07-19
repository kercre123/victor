#include "bignum.h"

#ifndef __DIFFIE_H
#define __DIFFIE_H

static const int SECRET_LENGTH = AES_KEY_LENGTH;

struct DiffieHellman {
  // These are the numbers for our diffie group
  const big_mont_t* mont;
  const big_num_t*  gen;
  
  uint32_t          pin;
  uint8_t           local_secret[SECRET_LENGTH];
  uint8_t           remote_secret[SECRET_LENGTH];
  uint8_t           encoded_key[AES_KEY_LENGTH];
};

void dh_start(DiffieHellman* dh);
void dh_finish(const void* key, DiffieHellman* dh);
void dh_reverse(DiffieHellman* dh, uint8_t* key);

#endif
