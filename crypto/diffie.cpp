#include <string.h>

#include "aes.h"
#include "bignum.h"
#include "diffie.h"
#include "random.h"
#include "sha1.h"

#include "publickeys.h"

// Step to convert a pin + random to a hash
void dh_encode_random(uint8_t* output, int pin, const uint8_t* random) {
  // Hash our pin (keeping only lower portion
  SHA1_CTX ctx;
  sha1_init(&ctx);
  sha1_update(&ctx, (uint8_t*)&pin, sizeof(pin));

  uint8_t sig[SHA1_BLOCK_SIZE];
  sha1_final(&ctx, sig);
  
  AES128_ECB_encrypt(random, sig, output);
}

static inline void dh_random_to_num(big_num_t& num, const uint8_t* digits) {
  memcpy(num.digits, digits, SECRET_LENGTH);
  num.used = SECRET_LENGTH / sizeof(big_num_cell_t);
  num.negative = false;
}

// Supply remote_secret, local_secret, encoded_key and pin
void dh_reverse(uint8_t* local_secret, uint8_t* remote_secret, int pin, uint8_t* key) {
  // Encode our secret as an exponent
  big_num_t temp;
  big_num_t state;

  uint8_t local_encoded[SECRET_LENGTH];
  uint8_t remote_encoded[SECRET_LENGTH];
  uint8_t encoded_key[AES_KEY_LENGTH];

  dh_encode_random(local_encoded, pin, local_secret);
  dh_random_to_num(temp, local_secret);
  mont_power(RSA_DIFFIE_MONT, state, RSA_DIFFIE_EXP_MONT, temp);

  dh_encode_random(remote_encoded, pin, remote_secret);
  dh_random_to_num(temp, remote_secret);
  mont_power(RSA_DIFFIE_MONT, state, state, temp);

  mont_from(RSA_DIFFIE_MONT, temp, state);

  AES128_ECB_decrypt(encoded_key, (uint8_t*)temp.digits, key);
}
