/**
File: cInterfaces_c.h
Author: Peter Barnum
Created: 2013

Some c++ algorithms will be accelerated in C-only code. This header has conversion routines and declarations from c++ to c.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_C_INTERFACES_H_
#define _ANKICORETECHEMBEDDED_COMMON_C_INTERFACES_H_

#include "anki/common/robot/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_S32_POINTER(data, stride, index0, index1) ((const s32*)( (const char*)(data) + (index1)*sizeof(s32) + (index0)*(stride) ))

  typedef struct
  {
    s16 left;
    s16 right;
    s16 top;
    s16 bottom;
  } C_Rectangle_s16;

  typedef struct
  {
    s32 size0, size1;
    s32 stride;
    u32 flags;

    s32 * data;
  } C_Array_s32;

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_C_INTERFACES_H_