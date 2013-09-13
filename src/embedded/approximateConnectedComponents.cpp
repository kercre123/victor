#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    Result extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList_Component1d &extractedComponents)
    {
      bool onComponent;
      s16 componentStart;
      s16 numSkipped = 0;

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
            numSkipped++;
            if(numSkipped > maxSkipDistance) {
              const s16 componentWidth = x - componentStart;
              if(componentWidth >= minComponentWidth) {
                extractedComponents.PushBack(Component1d(componentStart, x-numSkipped));
              }
              onComponent = false;
            }
          } else {
            numSkipped = 0;
          }
        } else {
          if(binaryImageRow[x] != 0) {
            componentStart = x;
            onComponent = true;
            numSkipped = 0;
          }
        }
      }

      if(onComponent) {
        const s16 componentWidth = binaryImageWidth - componentStart;
        if(componentWidth >= minComponentWidth) {
          extractedComponents.PushBack(Component1d(componentStart, binaryImageWidth-numSkipped-1));
        }
      }

      return RESULT_OK;
    } // Result extract1dComponents(const u8 * restrict binaryImageRow, const s32 binaryImageWidth, const s32 minComponentWidth, FixedLengthList_Point_s16 &extractedComponents)
  } // namespace Embedded
} // namespace Anki