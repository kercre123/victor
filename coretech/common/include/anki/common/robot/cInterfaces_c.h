#ifndef _ANKICORETECHEMBEDDED_COMMON_C_INTERFACES_H_
#define _ANKICORETECHEMBEDDED_COMMON_C_INTERFACES_H_

#include "anki/embeddedCommon/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_S32_POINTER(data, stride, index0, index1) ((const s32*)( (const char*)(data) + (index1)*sizeof(s32) + (index0)*(stride) ))

  typedef enum
  {
    C_ACCELERATION_NATURAL_CPP = 0, // Normal C++, possibly including natural c or shave emulation c subfunctions
    C_ACCELERATION_NATURAL_C = 1, // Normal C, written in a standard C style. May not be bit-exact with C_ACCELERATION_SHAVE_C or C_ACCELERATION_SHAVE_EMULATION_C
    C_ACCELERATION_SHAVE_EMULATION_C = 2, // Normal C, but written to emulate the SHAVEs behavior. It should be bit-exact with C_ACCELERATION_SHAVE
    C_ACCELERATION_SHAVE = 3 // SHAVE-native code. It could include C, intrisics, and/or assembly. It should be bit-exact with C_ACCELERATION_SHAVE_EMULATION_C
  } C_AccelerationType;

  typedef struct
  {
    C_AccelerationType type;
    s32 version; // There may be multiple versions of an algorithms. Each should be bit-exact (or almost bit-exact). Otherwise, they should have a different name.
  } C_Acceleration;

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
    s32 useBoundaryFillPatterns;

    s32 * data;
  } C_Array_s32;

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_C_INTERFACES_H_