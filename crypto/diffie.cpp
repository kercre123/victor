#include <string.h>

#include "aes.h"
#include "bignum.h"
#include "diffie.h"
#include "random.h"
#include "sha1.h"

// Step to convert a pin + random to a hash
static void dh_encode_random(uint8_t* output, int pin, const uint8_t* random) {
  ecb_data_t ecb;

  {
    // Hash our pin (keeping only lower portion
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (uint8_t*)&pin, sizeof(pin));

    uint8_t sig[SHA1_BLOCK_SIZE];
    sha1_final(&ctx, sig);
  
    // Setup ECB
    memcpy(ecb.cleartext, random, AES_KEY_LENGTH);
    memcpy(ecb.key, sig, sizeof(ecb.key));
  }

  aes_ecb(&ecb);

  memcpy(output, ecb.ciphertext, AES_KEY_LENGTH);
}

static void fix_pin(uint32_t& pin) {
  for (int bit = 0; bit < sizeof(pin) * 8; bit += 4) {
    int nibble = (pin >> bit) & 0xF;
    if (nibble > 0x9) {
      pin += 0x06 << bit;
    }
  }
}

void dh_start(DiffieHellman* dh) {
  // Generate our secret
  gen_random(&dh->pin, sizeof(dh->pin));
  fix_pin(dh->pin);
  gen_random(dh->local_secret, SECRET_LENGTH);
  dh_encode_random(dh->local_encoded, dh->pin, dh->local_secret);
  dh_encode_random(dh->remote_encoded, dh->pin, dh->remote_secret);
}

static inline void dh_random_to_num(big_num_t& num, const uint8_t* digits) {
  memcpy(num.digits, digits, SECRET_LENGTH);
  num.used = SECRET_LENGTH / sizeof(big_num_cell_t);
  num.negative = false;
}

void dh_finish(const void* key, DiffieHellman* dh) {
  // Encode our AES key with the DH output
  ecb_data_t ecb;
  
  memcpy(ecb.key, dh->diffie_result, AES_KEY_LENGTH);
  memcpy(ecb.cleartext, key, AES_KEY_LENGTH);
  
  aes_ecb(&ecb);

  memcpy(dh->encoded_key, ecb.ciphertext, AES_KEY_LENGTH);
}

// Supply remote_secret, local_secret, encoded_key and pin
void dh_reverse(DiffieHellman* dh, uint8_t* key) {
  // Encode our secret as an exponent
  big_num_t temp;
  big_num_t state;

  dh_encode_random(dh->local_encoded, dh->pin, dh->local_secret);
  dh_random_to_num(temp, dh->local_secret);
  mont_power(*dh->mont, state, *dh->gen, temp);

  dh_encode_random(dh->remote_encoded, dh->pin, dh->remote_secret);
  dh_random_to_num(temp, dh->remote_secret);
  mont_power(*dh->mont, state, state, temp);

  mont_from(*dh->mont, temp, state);

  AES128_ECB_decrypt(dh->encoded_key, (uint8_t*)temp.digits, key);
}
