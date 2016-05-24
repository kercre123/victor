#include <string.h>

#include "aes.h"
#include "bignum.h"
#include "diffie.h"
#include "random.h"
#include "sha1.h"

// Step to convert a pin + random to a hash
static void dh_encode_random(big_num_t& result, int pin, const uint8_t* random) {
  ecb_data_t ecb;

  {
    // Hash our pin (keeping only lower portion
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (uint8_t*)&pin, sizeof(pin));

    uint8_t sig[SHA1_BLOCK_SIZE];
    sha1_final(&ctx, sig);
  
    // Setup ECB
    memcpy(ecb.cleartext, random, AES_BLOCK_LENGTH);
    memcpy(ecb.key, sig, sizeof(ecb.key));
  }

  result.negative = false;
  result.used = AES_BLOCK_LENGTH / sizeof(big_num_cell_t);

  aes_ecb(&ecb);

  memcpy(result.digits, ecb.ciphertext, AES_BLOCK_LENGTH);
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

  // Encode our secret as an exponent
  big_num_t secret;

  dh_encode_random(secret, dh->pin, dh->local_secret);
  mont_power(*dh->mont, dh->state, *dh->gen, secret);
}

void dh_finish(DiffieHellman* dh) {
  // Encode their secret for exponent
  big_num_t temp;

  dh_encode_random(temp, dh->pin, dh->remote_secret);

  mont_power(*dh->mont, dh->state, dh->state, temp);
  mont_from(*dh->mont, temp, dh->state);

  // Override the secret with 
  ecb_data_t ecb;
  
  memcpy(ecb.key, temp.digits, AES_KEY_LENGTH);
  memcpy(ecb.cleartext, aes_key(), AES_BLOCK_LENGTH);
  
  aes_ecb(&ecb);

  memcpy(dh->encoded_key, ecb.ciphertext, AES_BLOCK_LENGTH);
}

