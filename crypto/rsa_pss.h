#ifndef __RSAPSS_H
#define __RSAPSS_H

#include "stdint.h"
#include "bignum.h"

bool verify_cert(const big_mont_t& mont, const big_rsa_t& public_key, const uint8_t* checksum, const uint8_t* cert, int size);

#endif
