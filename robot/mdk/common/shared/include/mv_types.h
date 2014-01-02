///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Basic type definitions
/// 

#ifndef _MV_TYPES_H_
#define _MV_TYPES_H_

// 1: Defines
// ----------------------------------------------------------------------------

#ifndef FALSE
#define FALSE        (0)
#endif

#ifndef TRUE
#define TRUE         (1)
#endif

#ifndef NULL
#define NULL         (0)
#endif

#define ALL_ZEROS    (0x00000000)
#define ALL_ONES     (0xFFFFFFFF)


/* Limits of integral types.  */


// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// mv typedefs
typedef unsigned char            u8;
typedef   signed char            s8;
typedef unsigned short          u16;
typedef   signed short          s16;
typedef unsigned long           u32;
typedef   signed long           s32;
typedef unsigned long long      u64;
typedef   signed long long      s64;

//// ANSI typedefs -- todo: more appropriate protection here
#ifndef _WIN32
#ifndef __MOVICOMPILE__
#ifndef RTEMS
typedef unsigned char       uint8_t;
typedef   signed char        int8_t;
typedef unsigned short     uint16_t;
typedef   signed short      int16_t;
typedef unsigned int       uint32_t;
typedef   signed int        int32_t;
typedef unsigned long long uint64_t;
typedef   signed long long  int64_t;
#endif
#endif
#else
#include <stdint.h>
#endif

typedef signed short           fp16;
typedef float                  fp32;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif /* _MV_TYPES_H_ */
