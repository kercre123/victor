#ifndef __RSAPSS_H
#define __RSAPSS_H

#include "stdint.h"
#include "bignum.h"

bool verify_cert(const uint8_t* target, const int target_size, const uint8_t* cert, int cert_size);

#endif
