/**
* File: types.h
*
* Author: Andrew Stein (andrew)
* Created: 10/7/2013
*
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*         !!!!!!!!!!!!!!    ROBOT TYPES     !!!!!!!!!!!!!
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
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


#ifndef COZMO_ROBOT
#error robot/include/anki/types.h should only be used in robot files
#endif

#ifndef __COZMO_ROBOT_COMMON_TYPES_H__
#define __COZMO_ROBOT_COMMON_TYPES_H__

#ifdef TARGET_ESPRESSIF
#include "c_types.h"
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


//////////////////////////////////////////////////////////////////////////////
// MAX / MIN VALUES
//////////////////////////////////////////////////////////////////////////////
// Can't use numeric_limits because it needs to be pure C and has templates

#ifndef u8_MAX
#define u8_MAX ( (u8)(0xFF))
#endif

#ifndef u16_MAX
#define u16_MAX ( (u16)(0xFFFF) )
#endif

#ifndef u32_MAX
#define u32_MAX ( (u32)(0xFFFFFFFF) )
#endif

#ifndef u64_MAX
#define u64_MAX ( (u64)(0xFFFFFFFFFFFFFFFFLL) )
#endif

#ifndef s8_MIN
#define s8_MIN ( (s8)(-1 - 0x7F) )
#endif

#ifndef s8_MAX
#define s8_MAX ( (s8)(0x7F) )
#endif

#ifndef s16_MIN
#define s16_MIN ( (s16)(-1 - 0x7FFF) )
#endif

#ifndef s16_MAX
#define s16_MAX ( (s16)(0x7FFF) )
#endif

#ifndef s32_MIN
#define s32_MIN ( (s32)(-1 - 0x7FFFFFFF) )
#endif

#ifndef s32_MAX
#define s32_MAX ( (s32)(0x7FFFFFFF) )
#endif

#ifndef s64_MIN
#define s64_MIN ( (s64)(-1 - 0X7FFFFFFFFFFFFFFFLL) )
#endif

#ifndef s64_MAX
#define s64_MAX ( (s64)(0x7FFFFFFFFFFFFFFFLL) )
#endif

#ifndef NULL
#define NULL 0
#endif


// If we're using c++, Result is in a namespace. In c, it's not.
#ifdef __cplusplus
namespace Anki
{
#endif

  // NOTE: changing the basic type of TimeStamp_t (e.g. to u16 in order to save
  //       bytes), has implications for message alignment since it currently
  //       comes first in the message structs.
  typedef u32 TimeStamp_t;

  // PoseFrameID_t is used to denote a set of poses that were recorded since
  // the last absolute localization update. This is required in order to
  // know which pose updates coming from the robot are of the robot before
  // or after the last absolute pose update was sent to the robot.
  typedef u32 PoseFrameID_t;

  typedef u32 PoseOriginID_t;
  

  // Return values:
  typedef enum {
    RESULT_OK                        = 0,
    RESULT_FAIL                      = 0x00000001,
    RESULT_FAIL_MEMORY               = 0x01000000,
    RESULT_FAIL_OUT_OF_MEMORY        = 0x01000001,
    RESULT_FAIL_UNINITIALIZED_MEMORY = 0x01000002,
    RESULT_FAIL_ALIASED_MEMORY       = 0x01000003,
    RESULT_FAIL_IO                   = 0x02000000,
    RESULT_FAIL_IO_TIMEOUT           = 0x02000001,
    RESULT_FAIL_IO_CONNECTION_CLOSED = 0x02000002,
    RESULT_FAIL_INVALID_PARAMETER    = 0x03000000,
    RESULT_FAIL_INVALID_OBJECT       = 0x04000000,
    RESULT_FAIL_INVALID_SIZE         = 0x05000000
  } Result;
#ifdef __cplusplus
} // namespace Anki
#endif

#endif /* __COZMO_ROBOT_COMMON_TYPES_H__ */
