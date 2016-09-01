#include <string.h>

#include "bignum.h"
#include "sha512.h"
#include "rsa_pss.h"

// This is encoded in big-endian format
static const uint8_t PADDING[] = {
  0x00, 0xFF, 0xFF,
  SHA512_OID,
  0xFF, 0xFF, 0x01, 0x00
};

static void MGF1(uint8_t* db, const uint8_t* checksum, int length) {
  for (uint32_t count = 0; ; count++) {
    sha512_state digest;
    uint8_t mask[SHA512_DIGEST_SIZE];

    sha512_init(digest);
    sha512_process(digest, checksum, SHA512_DIGEST_SIZE);
    sha512_process(digest, &count, sizeof(count));
    sha512_done(digest, mask);

    for (int i = 0; i < SHA512_DIGEST_SIZE; i++) {
      if (length-- <= 0) {
        return ;
      }

      *(db++) ^= mask[i];
    }
  }
}

bool verify_cert(cert_state_t* state, const uint8_t* cert, int size)
{
  big_num_t&  rsa_decoded = state->rsa_decoded;
  big_num_t&  temp        = state->temp;
  big_mont_t& mont        = state->mont;
  big_rsa_t&  rsa         = state->rsa;
  const uint8_t* checksum = state->checksum;

  temp.used = (size + sizeof(big_num_cell_t) - 1) / sizeof(big_num_cell_t);
  temp.negative = false;
  temp.digits[temp.used - 1] = 0;
  memcpy(temp.digits, cert, size);

  // Convert to the pre-rsa (with padding shifted out)
  mont_to(mont, rsa_decoded, temp);
  mont_power(mont, temp, rsa_decoded, rsa.exp);
  mont_from(mont, rsa_decoded, temp);

  // Calculate constants
  const int key_length  = big_msb(rsa.modulo);
  const int mod_length  = key_length / 8;
  const int db_length   = mod_length - SHA512_DIGEST_SIZE;
  const int salt_length = db_length - sizeof(PADDING);
  const int pad_length  = key_length % 8;

  big_shr(rsa_decoded, rsa_decoded, pad_length);

  // Pointers for access
  uint8_t* const decoded = (uint8_t*)rsa_decoded.digits;
  uint8_t* const dbMasked = &decoded[SHA512_DIGEST_SIZE];
  uint8_t* const padding = &decoded[SHA512_DIGEST_SIZE + salt_length];

  // Remove MGF on database
  MGF1(dbMasked, decoded, db_length);

  // Verify this is SHA512 encoded (padding)
  if (memcmp(PADDING, padding, sizeof(PADDING))) {
    return false;
  }

  // Generate stage 2 hash
  {
    sha512_state digest;
    uint8_t sha_ref[SHA512_DIGEST_SIZE];

    sha512_init(digest);
    sha512_process(digest, dbMasked, salt_length);
    sha512_process(digest, checksum, SHA512_DIGEST_SIZE);
    sha512_process(digest, padding, sizeof(PADDING));
    sha512_done(digest, sha_ref);

    // If our hashes do not match, cert is bunk
    return memcmp(sha_ref, decoded, SHA512_DIGEST_SIZE) == 0;
  }
}
