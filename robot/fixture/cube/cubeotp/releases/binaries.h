#ifndef __BINARIES_H
#define __BINARIES_H

#include <stdbool.h>
#include <stdint.h>

extern const uint8_t g_CubeBoot[];
extern const uint8_t g_CubeBootEnd[];
#define g_CubeBootSize (g_CubeBootEnd - g_CubeBoot)

#endif
