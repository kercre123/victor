/* Sensory Confidential
 *
 * Copyright (C)2007-2014 Sensory, Inc
 *---------------------------------------------------------------------------
 */
#ifndef SENSORYTYPES_H
#define SENSORYTYPES_H

#undef  __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifndef bool
typedef uint8_t  bool;
#endif
typedef uint8_t  b08;
typedef uint16_t b16;
typedef uint32_t b24;
typedef uint32_t b32;
typedef uint64_t b48;
typedef uint64_t b64;

typedef uint8_t  u08;
typedef uint16_t u16;
typedef uint32_t u24;
typedef uint32_t u32;
typedef uint64_t u48;
typedef uint64_t u64;

typedef int8_t   s08;
typedef int16_t  s16;
typedef int32_t  s24;
typedef int32_t  s32;
typedef int64_t  s48;
typedef int64_t  s64;

/* For pointer-to-int conversions.
 * uintptr_t is a typedef in Windows.
 */
#if !defined(uintptr_t) && !defined(_WIN32)
# ifdef __LP64__
#  define uintptr_t uint64_t
# else
#  define uintptr_t uint32_t
# endif
#endif

#ifdef _WIN64
/* LLP64, long is 32 bits, pointers are 64 bits */
#  define strtoptr _strtoui64
# else
#  define strtoptr strtoul
#endif


/* We're assuming a C-99 compiler here, use %z for size_t format strings
 */
#ifndef PRIdSZ
# define PRIdSZ "zd"
#endif
#ifndef PRIiSZ
# define PRIiSZ "zi"
#endif
#ifndef PRIuSZ
# define PRIuSZ "zu"
#endif
#ifndef PRIxSZ
# define PRIxSZ "zx"
#endif
#ifndef SCNdSZ
# define SCNdSZ "zd"
#endif
#ifndef SCNiSZ
# define SCNiSZ "zi"
#endif
#ifndef SCNuSZ
# define SCNuSZ "zu"
#endif
#ifndef SCNxSZ
# define SCNxSZ "zx"
#endif

#endif /* SENSORYTYPES_H */
