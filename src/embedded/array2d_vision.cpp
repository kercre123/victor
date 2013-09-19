#include "anki/embeddedVision/array2d_vision.h"

namespace Anki
{
  namespace Embedded
  {
    template<> Result Array<ConnectedComponentSegment >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Point<s16> >::Print", "Array<Point<s16> > is not valid");

      printf("%s:\n", variableName);
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const ConnectedComponentSegment * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          rowPointer[x].Print();
          printf(" ");
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    } // template<> Result Array<ConnectedComponentSegment >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const

    template<> Result Array<FiducialMarker >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Point<s16> >::Print", "Array<Point<s16> > is not valid");

      printf("%s:\n", variableName);
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const FiducialMarker * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          rowPointer[x].Print();
          printf(" ");
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    } // template<> Result Array<FiducialMarker >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
  } // namespace Embedded
} // namespace Anki