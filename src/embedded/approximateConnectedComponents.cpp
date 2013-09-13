#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    Result extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, FixedLengthList_Component1d &extractedComponents)
    {
      bool onComponent;
      s16 componentStart;

      // If the first pixel is nonzero, we start a component at the far left
      if(binaryImageRow[0] == 0) {
        onComponent = false;
        componentStart = -1;
      } else {
        onComponent = true;
        componentStart = 0;
      }

      for(s16 x = 1; x<binaryImageWidth; x++) {
        if(onComponent) {
          if(binaryImageRow[x] == 0) {
            const s16 componentWidth = x - componentStart;
            if(componentWidth >= minComponentWidth) {
              // components1d(end+1, :) = [componentStart, x-1];
              extractedComponents.PushBack(Component1d(componentStart, x-1));
            }
            onComponent = false;
          }
        } else {
          if(binaryImageRow[x] != 0) {
            componentStart = x;
            onComponent = true;
          }
        }
      }

      if(onComponent) {
        const s16 componentWidth = binaryImageWidth - componentStart;
        if(componentWidth >= minComponentWidth) {
          // components1d(end+1, :) = [componentStart, x];
          extractedComponents.PushBack(Component1d(componentStart, binaryImageWidth-1));
        }
      }

      return RESULT_OK;
    } // Result extract1dComponents(const u8 * restrict binaryImageRow, const s32 binaryImageWidth, const s32 minComponentWidth, FixedLengthList_Point_s16 &extractedComponents)
  } // namespace Embedded
} // namespace Anki