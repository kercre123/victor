#ifndef _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
#define _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_

#include "anki/embeddedCommon/config.h"
#include "anki/embeddedCommon/DASlight.h"

#ifndef MAX
#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
#define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifndef CLIP
#define CLIP(n, min, max) ( MIN(MAX(min, n), max ) )
#endif

#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

#ifndef SIGN
#define SIGN(a) ((a) >= 0)
#endif

#ifndef NULL
#define NULL (0)
#endif

#define SWAP(type, a, b) { type t; t = a; a = b; b = t; }

// ct_assert is a compile time assertion, useful for checking sizeof() and other compile time knowledge
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

namespace Anki
{
  namespace Embedded
  {
    template<typename T> inline T RoundUp(T number, T multiple)
    {
      return multiple*( (number-1)/multiple + 1 );

      // For unsigned only?
      // return (number + (multiple-1)) & ~(multiple-1);
    }

    template<typename T> inline T RoundDown(T number, T multiple)
    {
      return multiple * (number/multiple);
    }

    f32 Round(f32 number);
    f64 Round(f64 number);

    bool IsPowerOfTwo(u32 x);

    u32 Log2(u32 x);
    u64 Log2(u64 x);

    double GetTime();

    // For a square array, either:
    // 1. When lowerToUpper==true,  copies the lower (left)  triangle to the upper (right) triangle
    // 2. When lowerToUpper==false, copies the upper (right) triangle to the lower (left)  triangle
    // Functionally the same as OpenCV completeSymm()
    template<typename T> Result MakeArraySymmetric(T &arr, bool lowerToUpper = false)
    {
      DASConditionalErrorAndReturnValue(arr.get_size(0) == arr.get_size(1),
        RESULT_FAIL, "copyHalfArray", "Input array must be square");

      for(s32 y = 0; y < arr.get_size(0); y++)
      {
        const s32 x0 = lowerToUpper ? (y+1)           : 0;
        const s32 x1 = lowerToUpper ? arr.get_size(0) : y;

        for(s32 x = x0; x < x1; x++) {
          *arr.Pointer(y,x) = *arr.Pointer(x,y);
        }
      }

      return RESULT_OK;
    }

    // Perform the matrix multiplication "matOut = mat1 * mat2"
    // Note that this is the naive O(n^3) implementation
    template<typename Array_T, typename T> Result MultiplyMatrices(const Array_T &mat1, const Array_T &mat2, Array_T &matOut)
    {
      DASConditionalErrorAndReturnValue(mat1.get_size(1) == mat2.get_size(0),
        RESULT_FAIL, "MultiplyMatrices", "Input matrices are incompatible sizes");

      DASConditionalErrorAndReturnValue(matOut.get_size(0) == mat1.get_size(0),
        RESULT_FAIL, "MultiplyMatrices", "Input and Output matrices are incompatible sizes");

      DASConditionalErrorAndReturnValue(matOut.get_size(1) == mat2.get_size(1),
        RESULT_FAIL, "MultiplyMatrices", "Input and Output matrices are incompatible sizes");

      for(s32 y1=0; y1<mat1.get_size(0); y1++) {
        const T * restrict mat1_rowPointer = mat1.Pointer(y1, 0);
        T * restrict matOut_rowPointer = matOut.Pointer(y1, 0);

        for(s32 x2=0; x2<mat2.get_size(1); x2++) {
          matOut_rowPointer[x2] = 0;

          for(s32 y2=0; y2<mat2.get_size(0); y2++) {
            matOut_rowPointer[x2] += mat1_rowPointer[y2] * (*mat2.Pointer(y2, x2));
          }
        }
      }

      return RESULT_OK;
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth); // Converts from typeid names to openCV types
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

#ifndef ALIGNVARIABLE
#if defined(_MSC_VER)
#define ALIGNVARIABLE __declspec(align(16))
#elif defined(__APPLE_CC__)
#define ALIGNVARIABLE __attribute__ ((aligned (16)))
#elif defined(__GNUC__)
#define ALIGNVARIABLE __attribute__ ((aligned (16)))
#endif
#endif // #ifndef ALIGNVARIABLE
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
