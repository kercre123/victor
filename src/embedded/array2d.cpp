#include "anki/embeddedCommon/array2d.h"
#include "anki/embeddedCommon/errorHandling.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace Anki
{
  namespace Embedded
  {
    template<> IN_DDR Result Array<u8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Type>::Print", "Array<Type> is not valid");

      printf("%s:\n", variableName);
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

      printf("%s:\n", variableName);
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

      printf("%s:\n", variableName);
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

      printf("%s:\n", variableName);
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

#ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
    template<> IN_DDR s32 Array<f32>::Set(const char * const values)
    {
      assert(this->IsValid());

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        f32 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          const f32 value = static_cast<f32>(strtod(startPointer, &endPointer));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    } // s32 Array_f32::Set(const char * const values)
#endif // #ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT

#ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
    template<> IN_DDR s32 Array<f64>::Set(const char * const values)
    {
      assert(this->IsValid());

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        f64 * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          const f64 value = static_cast<f64>(strtod(startPointer, &endPointer));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }
#endif // #ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
  } // namespace Embedded
} // namespace Anki