#ifndef _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
#define _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities_c.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class Array;
    template<typename Type> class FixedLengthList;

    template<typename Type> inline Type RoundUp(Type number, Type multiple);

    template<typename Type> inline Type RoundDown(Type number, Type multiple);

    template<typename Type> Type approximateExp(const Type exponent, const s32 numTerms = 10);

    // For a square array, either:
    // 1. When lowerToUpper==true,  copies the lower (left)  triangle to the upper (right) triangle
    // 2. When lowerToUpper==false, copies the upper (right) triangle to the lower (left)  triangle
    // Functionally the same as OpenCV completeSymm()
    template<typename Type> Result MakeArraySymmetric(Type &arr, bool lowerToUpper = false);

    // Perform the matrix multiplication "matOut = mat1 * mat2"
    // Note that this is the naive O(n^3) implementation
    template<typename Array_Type, typename Type> Result MultiplyMatrices(const Array_Type &mat1, const Array_Type &mat2, Array_Type &matOut);

    // Swap a with b
    template<typename Type> void Swap(Type &a, Type &b);

    template<typename Type> u32 BinaryStringToUnsignedNumber(const FixedLengthList<Type> &bits, bool firstBitIsLow = false);

    // Movidius doesn't have floating point printf (no %f option), so do it with %d
    void PrintfOneArray_f32(const Array<f32> &array, const char * variableName);
    void PrintfOneArray_f64(const Array<f64> &array, const char * variableName);

    // Return the sum of every element in the Array
    s32 Sum(const Array<u8> &image);

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    // Converts from typeid names to openCV types
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

#pragma mark --- Implementations ---

    template<typename Type> inline Type RoundUp(Type number, Type multiple)
    {
      return multiple*( (number-1)/multiple + 1 );

      // For unsigned only?
      // return (number + (multiple-1)) & ~(multiple-1);
    }

    template<typename Type> inline Type RoundDown(Type number, Type multiple)
    {
      return multiple * (number/multiple);
    }

    template<typename Type> Type approximateExp(const Type exponent, const s32 numTerms)
    {
      assert(numTerms > 2);

      const Type exponentAbs = ABS(exponent);

      Type sum = static_cast<Type>(1) + exponentAbs;

      Type numerator = static_cast<Type>(exponentAbs);
      Type denominator = static_cast<Type>(1);
      for(s32 i=2; i<=numTerms; i++) {
        numerator *= exponentAbs;
        denominator *= i;

        sum += numerator / denominator;
      }

      if(exponent < 0) {
        sum = static_cast<Type>(1) / sum;
      }

      return sum;
    }

    template<typename Type> Result MakeArraySymmetric(Type &arr, bool lowerToUpper)
    {
      AnkiConditionalErrorAndReturnValue(arr.get_size(0) == arr.get_size(1),
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

    template<typename Array_Type, typename Type> Result MultiplyMatrices(const Array_Type &mat1, const Array_Type &mat2, Array_Type &matOut)
    {
      AnkiConditionalErrorAndReturnValue(mat1.get_size(1) == mat2.get_size(0),
        RESULT_FAIL, "MultiplyMatrices", "Input matrices are incompatible sizes");

      AnkiConditionalErrorAndReturnValue(matOut.get_size(0) == mat1.get_size(0),
        RESULT_FAIL, "MultiplyMatrices", "Input and Output matrices are incompatible sizes");

      AnkiConditionalErrorAndReturnValue(matOut.get_size(1) == mat2.get_size(1),
        RESULT_FAIL, "MultiplyMatrices", "Input and Output matrices are incompatible sizes");

      for(s32 y1=0; y1<mat1.get_size(0); y1++) {
        const Type * restrict mat1_rowPointer = mat1.Pointer(y1, 0);
        Type * restrict matOut_rowPointer = matOut.Pointer(y1, 0);

        for(s32 x2=0; x2<mat2.get_size(1); x2++) {
          matOut_rowPointer[x2] = 0;

          for(s32 y2=0; y2<mat2.get_size(0); y2++) {
            matOut_rowPointer[x2] += mat1_rowPointer[y2] * (*mat2.Pointer(y2, x2));
          }
        }
      }

      return RESULT_OK;
    }

    template<typename Type> void Swap(Type &a, Type &b)
    {
      const Type tmp = a;
      a = b;
      b = tmp;
    } // template<typename Type> Swap(Type a, Type b)

    template<typename Type> u32 BinaryStringToUnsignedNumber(const FixedLengthList<Type> &bits, bool firstBitIsLow)
    {
      u32 number = 0;

      for(s32 bit=0; bit<bits.get_size(); bit++) {
        if(firstBitIsLow) {
          if(bit == 0) {
            number += bits[bit];
          } else {
            number += bits[bit] << bit;
          }
        } else {
          if(bit == (bits.get_size()-1)) {
            number += bits[bit];
          } else {
            number += bits[bit] << (bits.get_size() - bit - 1);
          }
        }
      }

      return number;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
