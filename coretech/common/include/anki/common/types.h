/**
* File: types.h
*
* Author: Andrew Stein (andrew)
* Created: 10/7/2013
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*       !!!!!!!!!!!!!!   CORETECH COMMON TYPES     !!!!!!!!!!!!!
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

#include <stdint.h>
#include <limits.h>

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

typedef u32 UserDeviceID_t;
typedef u32 RobotID_t;

typedef u32 CameraID_t;

const CameraID_t ANY_CAMERA = UINT_MAX;

typedef unsigned long long int BaseStationTime_t;

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
  // TODO: add toString!!!
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
    RESULT_FAIL_INVALID_SIZE         = 0x05000000,
    RESULT_FAIL_ORIGIN_MISMATCH      = 0x06000000
  } Result;
#ifdef __cplusplus
} // namespace Anki
#endif

#endif /* ANKICORETECH_COMMON_TYPES_H_ */
