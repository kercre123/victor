#ifndef __RSAPSS_H
#define __RSAPSS_H

#include "stdint.h"
#include "bignum.h"

struct cert_state_t {
  big_num_t rsa_decoded;
  big_num_t temp;

  const big_mont_t* mont;
  const big_rsa_t* rsa;

  uint8_t checksum[SHA512_DIGEST_SIZE];
};

void verify_init(cert_state_t& state, 
  const big_mont_t& mont, const big_rsa_t& rsa, const uint8_t* checksum, const uint8_t* cert, int size);
void verify_stage1(cert_state_t& state);
void verify_stage2(cert_state_t& state);
void verify_stage3(cert_state_t& state);
bool verify_stage4(cert_state_t& state);

bool verify_cert(const big_mont_t& mont, const big_rsa_t& public_key, const uint8_t* checksum, const uint8_t* cert, int size);

#endif
