#include "anki/embeddedCommon/array2d.h"
#include "anki/embeddedCommon/errorHandling.h"
#include "anki/embeddedCommon/utilities.h"
#include "anki/embeddedCommon/utilities_c.h"

//using namespace std;

namespace Anki
{
  namespace Embedded
  {
    template<> IN_DDR Result Array<u8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Type>::Print", "Array<Type> is not valid");

      printf(variableName);
      printf(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const u8 * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          printf("%d ", static_cast<s32>(rowPointer[x]));
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    }

    template<> IN_DDR Result Array<f32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Type>::Print", "Array<Type> is not valid");

      printf(variableName);
      printf(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const f32 * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          printf("%f ", rowPointer[x]);
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    }

    template<> IN_DDR Result Array<f64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Type>::Print", "Array<Type> is not valid");

      printf(variableName);
      printf(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const f64 * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          printf("%f ", rowPointer[x]);
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    }

    template<> IN_DDR Result Array<Point<s16> >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Point<s16> >::Print", "Array<Point<s16> > is not valid");

      printf(variableName);
      printf(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const Point<s16> * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          rowPointer[x].Print();
          printf(" ");
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    } // void Array<Point<s16> >::Print() const

    template<> Result Array<Rectangle<s16> >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Rectangle<s16> >::Print", "Array<Rectangle<s16> > is not valid");

      printf(variableName);
      printf(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const Rectangle<s16> * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          rowPointer[x].Print();
          printf(" ");
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    } // void Array<Point<s16> >::Print() const

    template<> Result Array<Quadrilateral<s16> >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Quadrilateral<s16> >::Print", "Array<Quadrilateral<s16> > is not valid");

      printf(variableName);
      printf(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const Quadrilateral<s16> * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          rowPointer[x].Print();
          printf(" ");
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    } // void Array<Point<s16> >::Print() const

    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<u8>::Set(const u8 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<u8>::Set", "Array<u8> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        u8 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const u8 value = swcLeonReadNoCacheU8(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<s8>::Set(const s8 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<s8>::Set", "Array<s8> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        s8 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const s8 value = swcLeonReadNoCacheI8(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<u16>::Set(const u16 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<u16>::Set", "Array<u16> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        u16 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const u16 value = swcLeonReadNoCacheU16(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<s16>::Set(const s16 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<s16>::Set", "Array<s16> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        s16 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const s16 value = swcLeonReadNoCacheI16(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<u32>::Set(const u32 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<u32>::Set", "Array<u32> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        u32 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const u32 value = swcLeonReadNoCacheU32(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<s32>::Set(const s32 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<s32>::Set", "Array<s32> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        s32 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const s32 value = swcLeonReadNoCacheI32(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<u64>::Set(const u64 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<u64>::Set", "Array<u64> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        u64 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const u64 value = swcLeonReadNoCacheU64(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<s64>::Set(const s64 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<s64>::Set", "Array<s64> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        s64 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const s64 value = swcLeonReadNoCacheI64(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<f32>::Set(const f32 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<f32>::Set", "Array<f32> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        f32 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const f32 value = swcLeonReadNoCacheF32(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<f64>::Set(const f64 * const values, const s32 numValues) {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        0, "Array<f64>::Set", "Array<f64> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        f64 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          if(numValuesSet < numValues)
    //          {
    //            const f64 value = swcLeonReadNoCacheF64(values + numValuesSet);
    //            numValuesSet++;
    //
    //            rowPointer[x] = value;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
    //
    //#ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
    //    template<> IN_DDR s32 Array<f32>::Set(const char * const values)
    //    {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        RESULT_FAIL, "Array<f32>::Set", "Array<f32> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      const char * startPointer = values;
    //      char * endPointer = NULL;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        f32 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          const f32 value = static_cast<f32>(strtod(startPointer, &endPointer));
    //          if(startPointer != endPointer) {
    //            rowPointer[x] = value;
    //            numValuesSet++;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //          startPointer = endPointer;
    //        }
    //      }
    //
    //      return numValuesSet;
    //    } // s32 Array_f32::Set(const char * const values)
    //#endif // #ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
    //
    //#ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
    //    template<> IN_DDR s32 Array<f64>::Set(const char * const values)
    //    {
    //      AnkiConditionalErrorAndReturnValue(this->IsValid(),
    //        RESULT_FAIL, "Array<f64>::Set", "Array<f64> is not valid");
    //
    //      s32 numValuesSet = 0;
    //
    //      const char * startPointer = values;
    //      char * endPointer = NULL;
    //
    //      for(s32 y=0; y<size[0]; y++) {
    //        f64 * restrict rowPointer = Pointer(y, 0);
    //        for(s32 x=0; x<size[1]; x++) {
    //          const f64 value = static_cast<f64>(strtod(startPointer, &endPointer));
    //          if(startPointer != endPointer) {
    //            rowPointer[x] = value;
    //            numValuesSet++;
    //          } else {
    //            rowPointer[x] = 0;
    //          }
    //          startPointer = endPointer;
    //        }
    //      }
    //
    //      return numValuesSet;
    //    }
    //#endif // #ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
  } // namespace Embedded
} // namespace Anki