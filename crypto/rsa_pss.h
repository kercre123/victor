#ifndef __RSAPSS_H
#define __RSAPSS_H

#include "stdint.h"
#include "bignum.h"

struct cert_state_t {
  big_num_t rsa_decoded;
  big_num_t temp;

  big_mont_t mont; // Must be initalized from flash constatns
  big_rsa_t  rsa;  // Must be initalized from flash constants

  uint8_t checksum[SHA512_DIGEST_SIZE];
};

bool verify_cert(cert_state_t* state, const uint8_t* cert, int size);

#endif
