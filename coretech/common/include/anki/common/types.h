/**
 * File: types.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/7/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: 
 * 
 *   We specify types according to their sign and bits. We should use these in
 *   our code instead of the normal 'int','short', etc. because different
 *   compilers on different architectures treat these differently.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/
 
#ifndef ANKICORETECH_COMMON_TYPES_H_
#define ANKICORETECH_COMMON_TYPES_H_

// Typedefs in mv_types.h conflict with stdint.h types (which doesn't even exist in the movidius toolchain).
#ifdef MOVI_TOOLS
#include "mv_types.h"
#else
#include <stdint.h>

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;
typedef float    f32;
typedef double   f64;
#endif

#ifdef __cplusplus
namespace Anki
{
#endif

  typedef s32 ReturnCode;
  
#ifdef __cplusplus
} // namespace Anki
#endif

#endif /* ANKICORETECH_COMMON_TYPES_H_ */