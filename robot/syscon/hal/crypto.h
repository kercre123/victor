#ifndef __RNG_H
#define __RNG_H

#include "bignum.h"

static const int MAX_CRYPTO_TASKS = 4;
static const int AES_KEY_LENGTH = 16;
static const int AES_BLOCK_LENGTH = 16;
static const int SECRET_LENGTH = 16;

enum CryptoOperation {
  CRYPTO_GENERATE_RANDOM,
  CRYPTO_AES_ENCRYPT,
  CRYPTO_AES_DECRYPT,
  CRYPTO_START_DIFFIE_HELLMAN,
  CRYPTO_FINISH_DIFFIE_HELLMAN
};

struct ecb_data_t {
  uint8_t key[AES_KEY_LENGTH];
  uint8_t cleartext[AES_BLOCK_LENGTH];
  uint8_t ciphertext[AES_BLOCK_LENGTH];
};

struct DiffieHellman {
  // These are the numbers for our diffie group
  const big_mont_t* mont;
  const big_num_t*  gen;
  
  int               pin;
  uint8_t           secret[SECRET_LENGTH];

  big_num_t         state;
};

typedef void (*crypto_callback)(struct CryptoTask*);

struct CryptoTask {
	CryptoOperation op;
	crypto_callback callback;
	const void *input;
	void *output;
	int	 length;
};

namespace Crypto {
  void init();
  void manage();
  void random(void* data, int length);
	void execute(const CryptoTask* task);
}

#endif
