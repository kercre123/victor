#ifdef __cplusplus
extern "C" {
#endif

#ifndef SE_TYPES_H
#define SE_TYPES_H
/*************************************
  Copyright (c) 2009 Signal Essence LLC
  All rights reserved
**************************************/
//
// what compiler are we using?
#include "se_platform.h"
#include <limits.h>

// This math.h is included only for the
// NAN define to define invalid float numbers
// during our conversion to floats.
#include <math.h>

// If any modules are declared to be threaded, 
// make sure SE_THREADED is turned on so the 
// scratch memory allocation works right.
#if defined(SE_THREADED_AEC)
#define SE_THREADED
#endif

// 
// basic numeric types
// and constants
///////////////////////////////////////////////

//
// max and min values for N-bit types
#define SE_MAX_INT16 (32767)
#define SE_MIN_INT16 (-32768)

// for SE_MIN_INT32, simply defining the macro as -2147483648 
// can cause compiler warnings/errors; see 
// http://stackoverflow.com/questions/9941261/warning-this-decimal-constant-is-unsigned-only-in-iso-c90
// 
// the solution is to specify (-2147483647L - 1) = -2147483648
#define SE_MAX_INT32 (+2147483647L)
#define SE_MIN_INT32 (-2147483647L - 1)      


#if (defined(SE_PLTFRM_ANSI) || defined(SE_PLTFRM_X86) || defined (SE_PLTFRM_X86_64) || defined (SE_PLTFRM_ARM))
#define SE_TYPES_DEFINED
typedef char                int8;
typedef unsigned char       uint8;
typedef short               int16;
typedef unsigned short      uint16;
typedef int                 int32;
typedef unsigned int        uint32;
typedef long long           int64;
typedef unsigned long long  uint64;
typedef float               float32;
typedef int32               printf_d;  /* This is a %d type.  On C55, %d must be int16, on most everything else, it's an int32 */
typedef long int            printf_ld;
typedef int32               enum_t;    /** This is a weird type -- it's an int the same size as an enum so that we can pass enums into the SEDiag as ints and use a casting that matches properly. */

#define SE_INT_MAX64 9223372036854775807LL
#define SE_INT_MIN64 LLONG_MIN
#endif

#if defined(SE_PLTFRM_TMS320C55)
#define SE_TYPES_DEFINED
// for TMS320C55 data types, see Optimizing C Compiler User's Guide section 5.3 
// Note: C55 doesn't have 8-bit words
typedef short               int16;
typedef unsigned short      uint16;
typedef long                int32;
typedef unsigned long       uint32;
typedef long long           int40;
typedef unsigned long long  uint40;
typedef float               float32;
typedef int16               printf_d; /* This is a %d type.  On C55, %d must be int16, on most everything else, it's an int32 */
typedef int32               printf_ld;
typedef int16               enum_t;    /** This is a weird type -- it's an int the same size as an enum so that we can pass enums into the SEDiag as ints and use a casting that matches properly. */

#endif


#if defined(SE_PLTFRM_TMS320C674X)
#include <stdint.h>
#define SE_TYPES_DEFINED
typedef short               int16;
typedef unsigned short      uint16;
typedef int                 int32;
typedef unsigned int        uint32;
typedef float               float32;
typedef int32               printf_d;
typedef int32               printf_ld;
typedef int32               enum_t;   
typedef long long           int64;
typedef unsigned long long  uint64;

#define SE_INT_MAX64 9223372036854775807LL
#define SE_INT_MIN64 (-SE_INT_MAX64-1LL)
#endif

#ifndef SE_TYPES_DEFINED
#error SE_TYPES were not defined.  This is because somehow a SE_PLTFRM_<xyc> was not set, or se_types.h needs to be updated for your new platform.
#endif

#define ZERO_STRUCT(struct_ptr) memset(struct_ptr, 0, sizeof *struct_ptr)
#define ARRAY_SIZE(static_array) (sizeof(static_array)/sizeof(static_array[0]))

// to un-warningify an unused parameter
// say UNUSED(x)
#define UNUSED(x) (void)(x)
// WARN_UNUSED
// put WARN_UNUSED before a function name to issue warnings under GCC compile time when a functions return value is ignored.
// int WARN_UNUSED foo(void) {}
#if defined(__GNUC__)
  // note that clang/LLVM also defines __GNUC__
  // to see all predefined macros:  clang -dM -E -x c /dev/null | less
  // warning  see http://clang.llvm.org/compatibility.html#inline regarding inline function declaration
  #define WARN_UNUSED       __attribute__((warn_unused_result))
  #define NO_WARN_UNUSED    __attribute__((unused))
  #define INLINE      inline
  #define GCC_NOINLINE __attribute__ ((noinline))
  #define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
  #define WARN_UNUSED
  #define NO_WARN_UNUSED    
  #define INLINE      _inline
  #define GCC_NOINLINE
  #define DEPRECATED(func) __declspec(deprecated) func
#else
  #define WARN_UNUSED 
  #define NO_WARN_UNUSED    
  #define INLINE      
  #define GCC_NOINLINE
  #define DEPRECATED(func)  func
#endif
//
// This enum identifies the various ports of interest for the AEC
// including Sin, Rin, Refin, Rout, Sout as well as the working
// internal parameters of the send and receive path.
typedef enum  {
    PORT_MMIF_FIRST_DONT_USE,
    PORT_SEND_WORKING,
    PORT_SEND_SIN,
    PORT_SEND_SOUT,
    PORT_SEND_REFIN,
    PORT_RCV_WORKING,
    PORT_RCV_RIN,
    PORT_RCV_ROUT,
    PORT_MMIF_LAST
} MMIfPortID_t ;
    





#endif  // SE_TYPES_H

#ifdef __cplusplus
}
#endif

