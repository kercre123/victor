// SHA-512. Adapted from LibTomCrypt. This code is Public Domain
#ifndef __SHA512_H
#define __SHA512_H

#include <stdint.h>

#define SHA512_DIGEST_SIZE  64
#define SHA512_BLOCK_SIZE  128

#define SHA512_OID          0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03

struct sha512_state
{
    uint64_t    length;
    uint64_t    state[8];
    uint32_t    curlen;
    uint8_t     buf[128];
};

void sha512_init(sha512_state& md);
void sha512_process(sha512_state& md, const void* in, uint32_t inlen);
void sha512_done(sha512_state& md, void* out);

#endif
