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
#ifdef ROBOT_HARDWARE
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
#endif  // #ifdef ROBOT_HARDWARE ... #else

typedef float    f32;
typedef double   f64;

#ifdef __cplusplus
namespace Anki
{
#endif

  typedef s32 ReturnCode;
  
  // NOTE: changing the basic type of TimeStamp_t (e.g. to u16 in order to save
  //       bytes), has implications for message alignment since it currently
  //       comes first in the message structs.  
  typedef u32 TimeStamp_t;
  
#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

#ifdef __cplusplus
} // namespace Anki
#endif

// If we're using c++, Result is in a namespace. In c, it's not.
#ifdef __cplusplus
namespace Anki
{
  namespace Embedded
  {
#endif
    // Return values:
    typedef enum {
      RESULT_OK = 0,
      RESULT_FAIL = 0xFFFFFFFF,
      RESULT_FAIL_MEMORY = 0x01000000,
      RESULT_FAIL_OUT_OF_MEMORY = 0x01000001,
      RESULT_FAIL_UNINITIALIZED_MEMORY = 0x01000002,
      RESULT_FAIL_ALIASED_MEMORY = 0x01000003,
      RESULT_FAIL_IO = 0x02000000,
      RESULT_FAIL_INVALID_PARAMETERS = 0x03000000,
      RESULT_FAIL_INVALID_OBJECT = 0x04000000,
      RESULT_FAIL_INVALID_SIZE = 0x05000000,
      RESULT_FAIL_NUMERICAL = 0x06000000
    } Result;
#ifdef __cplusplus
  }
}
#endif

#endif /* ANKICORETECH_COMMON_TYPES_H_ */
