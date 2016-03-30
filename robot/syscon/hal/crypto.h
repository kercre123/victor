#ifndef __RNG_H
#define __RNG_H

#include "aes.h"
#include "bignum.h"

#ifndef MAX
#define MAX(x,y) ((x < y) ? (y) : (x))
#endif

static const int MAX_CRYPTO_TASKS = 4;
static const int SECRET_LENGTH = AES_BLOCK_LENGTH;

enum CryptoOperation {
  // Generate random number
  CRYPTO_GENERATE_RANDOM,
  
  // AES CFB block mode stuff
  CRYPTO_ECB,
  CRYPTO_AES_DECODE,
  CRYPTO_AES_ENCODE,
  
  // Pin code stuff
  CRYPTO_START_DIFFIE_HELLMAN,
  CRYPTO_FINISH_DIFFIE_HELLMAN
};

struct DiffieHellman {
  // These are the numbers for our diffie group
  const big_mont_t* mont;
  const big_num_t*  gen;
  
  int               pin;
  uint8_t           secret[MAX(SECRET_LENGTH, AES_KEY_LENGTH)];

  big_num_t         state;
};

typedef void (*crypto_callback)(const void *state);

struct CryptoTask {
  CryptoOperation op;
  crypto_callback callback;
  const void *state;
  int length;
};

namespace Crypto {
  void init();
  void manage();
  void random(void* data, int length);
  void execute(const CryptoTask* task);
}

#endif
