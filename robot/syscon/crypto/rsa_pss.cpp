#include <string.h>

#include "bignum.h"
#include "sha256.h"
#include "rsa_pss.h"
#include "publickeys.h"

// This is encoded in big-endian format
static const uint8_t PADDING[] = {
  0x00, 0xFF, 0xFF, 
  SHA256_OID,
  0xFF, 0xFF, 0x01, 0x00
};

static void MGF1(uint8_t* db, const uint8_t* checksum, int length) {
  for (uint32_t count = 0; ; count++) {
    sha256_state digest;
    uint8_t mask[SHA256_DIGEST_SIZE];

    sha_init(digest);
    sha_process(digest, checksum, SHA256_DIGEST_SIZE);
    sha_process(digest, &count, sizeof(count));
    sha_done(digest, mask);

    for (int i = 0; i < SHA256_DIGEST_SIZE; i++) {
      if (length-- <= 0) {
        return ;
      }

      *(db++) ^= mask[i];
    }
  }
}

bool verify_cert(const uint8_t* target, const int target_size, const uint8_t* cert, int cert_size)
{
  big_num_t  rsa_decoded;
  
  // RSA-2048 the cert
  {
    big_num_t  temp;

    memset(&temp, 0, sizeof(temp));
    memcpy(temp.digits, cert, cert_size);
    temp.used = (cert_size + sizeof(big_num_cell_t) - 1) / sizeof(big_num_cell_t);
    temp.negative = false;

    // Convert to the pre-rsa (with padding shifted out)
    mont_to(RSA_CERT_MONT, rsa_decoded, temp);
    mont_power(RSA_CERT_MONT, temp, rsa_decoded, AS_BN(CERT_RSA_EXP));
    mont_from(RSA_CERT_MONT, rsa_decoded, temp);
  }

  // Calculate constants
  const int keySize  = big_msb(AS_BN(RSA_CERT_MONT.modulo));
  const int modLength  = keySize / 8;
  const int dbLength   = modLength - SHA256_DIGEST_SIZE;
  const int saltLength = dbLength - sizeof(PADDING);
  const int padLength  = keySize % 8;

  uint8_t* const decoded = (uint8_t*)rsa_decoded.digits;
  const uint8_t* const mHash = decoded;
  uint8_t* const dbMasked = &decoded[SHA256_DIGEST_SIZE]; // Also a pointer to the salt
  const uint8_t* const dbSalt = dbMasked;
  const uint8_t* const dbPadding = &dbMasked[saltLength];

  // Remove padding bits
  big_shr(rsa_decoded, rsa_decoded, padLength);

  // Remove MGF on database
  MGF1(dbMasked, mHash, dbLength);

  // Verify this is SHA512 encoded (padding)
  if (memcmp(PADDING, dbPadding, sizeof(PADDING))) {
    return false;
  }

  // Generate stage 2 hash
  {
    sha256_state digest;
    uint8_t sha_ref[SHA256_DIGEST_SIZE];

    // Find our application hash
    sha_init(digest);
    sha_process(digest, target, target_size);
    sha_done(digest, sha_ref);

    // Check it against our mHash
    sha_init(digest);
    sha_process(digest, dbSalt, saltLength);
    sha_process(digest, sha_ref, sizeof(sha_ref));
    sha_process(digest, PADDING, sizeof(PADDING));
    sha_done(digest, sha_ref);

    // If our hashes do not match, cert is bunk
    return memcmp(sha_ref, mHash, SHA256_DIGEST_SIZE) == 0;
  }
}
