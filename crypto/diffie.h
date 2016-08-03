#include "bignum.h"

#ifndef __DIFFIE_H
#define __DIFFIE_H

static const int SECRET_LENGTH = AES_KEY_LENGTH;

void dh_encode_random(uint8_t* output, int pin, const uint8_t* random);
void dh_reverse(const uint8_t* local_secret, const uint8_t* remote_secret, int pin, uint8_t* key);

#endif
