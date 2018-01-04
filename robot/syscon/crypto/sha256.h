// SHA-256. Adapted from LibTomCrypt. This code is Public Domain
#ifndef __SHA256_H
#define __SHA256_H

#include <stdint.h>

static const int SHA256_DIGEST_SIZE = 32;

#define SHA256_OID 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01

struct sha256_state
{
    uint64_t length;
    uint32_t state[8];
    uint32_t curlen;
    unsigned char buf[64];
};

void sha_init(sha256_state& md);
void sha_process(sha256_state& md, const void* in, std::uint32_t inlen);
void sha_done(sha256_state& md, void* out);

#endif
