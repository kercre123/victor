/**
 ** This should have been implemented by the compiler.
 **/

#include <stdint.h>

int64_t __lshrdi3(int64_t a, int32_t b)
{
    const int bits_in_word = sizeof(uint32_t) * 8;

    union {
        struct {
            int32_t lo;
            uint32_t hi;
        } parts;

        int64_t word;
    } value;

    value.word = a;

    value.parts.lo = (value.parts.hi << (bits_in_word - b)) | (value.parts.lo >> b);
    value.parts.hi >>= b;

    return value.word;
}
