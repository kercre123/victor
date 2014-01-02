// Wrapper for types to make them a common ground for any compiler

#ifndef __TYPES_H__
#define __TYPES_H__

#if defined(__LEON__) || defined(__PC__)

#include <../../../../common/shared/include/mv_types.h>

#endif // __LEON__ || __PC__

#ifdef __MOVICOMPILE__

#include <stdint.h>
#include <stddef.h>

// mv typedefs
typedef unsigned char            u8;
typedef   signed char            s8;
typedef unsigned short          u16;
typedef   signed short          s16;
typedef unsigned long           u32;
typedef   signed long           s32;
typedef unsigned long long      u64;
typedef   signed long long      s64;

//typedef half					fp16;
typedef float					fp32;

#elif defined(__PC__)

#define half					float

#endif // __MOVICOMPILE__

#ifndef null
#define null NULL
#endif

#ifndef FALSE
#define FALSE        (0)
#endif

#ifndef TRUE
#define TRUE         (1)
#endif

#endif
