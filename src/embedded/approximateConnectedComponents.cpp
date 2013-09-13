#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    Result extract2dComponents(const Array_u8 &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList_Component2d &extractedComponents, MemoryStack scratch)
    {
      const s32 MAX_1D_COMPONENTS = binaryImage.get_size(1) / (minComponentWidth+1);
      const s32 MAX_2D_COMPONENTS = 50000;
      const s32 MAX_EQUIVALENT_ITERATIONS = 3;

      const s32 height = binaryImage.get_size(0);
      const s32 width = binaryImage.get_size(1);

      s32 numStored2dComponents = 0;

      FixedLengthList_Component1d currentComponents1d(MAX_1D_COMPONENTS, scratch);
      FixedLengthList_Component1d previousComponents1d(MAX_1D_COMPONENTS, scratch);
      FixedLengthList_Component1d newPreviousComponents1d(MAX_1D_COMPONENTS, scratch);

      s32 *equivalentComponents = reinterpret_cast<s32*>(scratch.Allocate(MAX_2D_COMPONENTS*sizeof(s32)));
      for(s32 i=0; i<MAX_2D_COMPONENTS; i++) {
        equivalentComponents[i] = i;
      }

      //for y = 1:size(binaryImg, 1)
      for(s32 y=0; y<height; y++) {
        const u8 * restrict binaryImage_rowPointer = binaryImage.Pointer(y,0);

        //  currentComponents1d = extract1dComponents(binaryImg(y,:), minComponentWidth, maxSkipDistance);
        extract1dComponents(binaryImage_rowPointer, static_cast<s16>(width), minComponentWidth, maxSkipDistance, currentComponents1d);

        //  newPreviousComponents1d = zeros(num1dComponents_current, 3);
        newPreviousComponents1d.Clear();

        //  for iCurrent = 1:num1dComponents_current
        for(s32 iCurrent=0; iCurrent<currentComponents1d.get_size(); iCurrent++) {
          //      foundMatch = false;
          //      for iPrevious = 1:num1dComponents_previous
          //          % The current component matches the previous one if
          //          % 1. the previous start is less-than-or-equal the current end, and
          //          % 2. the previous end is greater-than-or-equal the current start
          //          if previousComponents1d(iPrevious, 1) <= currentComponents1d(iCurrent, 2) &&...
          //              previousComponents1d(iPrevious, 2) >= currentComponents1d(iCurrent, 1)
          //
          //              if ~foundMatch
          //                  % If this is the first match we've found, add this 1d
          //                  % component to components2d, using that previous component's id.
          //                  foundMatch = true;
          //                  componentId = previousComponents1d(iPrevious, 3);
          //                  components2d(numStored1dComponents+1, :) = [componentId, y, currentComponents1d(iCurrent, 1:2)]; % [componentId, y, xStart, xEnd]
          //                  numStored1dComponents = numStored1dComponents + 1;
          //
          //                  newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), componentId];
          //              else
          //                  % Update the lookup table for all of:
          //                  % 1. The first matching component
          //                  % 2. This iPrevious component.
          //                  % 3. The newPreviousComponents1d(iCurrent) component
          //                  previousId = previousComponents1d(iPrevious, 3);
          //                  minId = min(componentId, previousId);
          //                  equivalentComponents(componentId) = min(equivalentComponents(componentId), minId);
          //                  equivalentComponents(previousId) = min(equivalentComponents(previousId), minId);
          //                  newPreviousComponents1d(iCurrent, 3) = minId;
          //              end
          //          end
          //      end
          //
          //      % If none of the previous components matched, start a new id, equal
          //      % to num2dComponents
          //      if ~foundMatch
          //          componentId = numStored2dComponents + 1;
          //
          //          newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), componentId];
          //          components2d(numStored1dComponents+1, :) = [componentId, y, currentComponents1d(iCurrent, 1:2)]; % [componentId, y, xStart, xEnd]
          //
          //          numStored1dComponents = numStored1dComponents + 1;
          //          numStored2dComponents = numStored2dComponents + 1;
          //      end
        } // for(s32 iCurrent=0; iCurrent<currentComponents1d.get_size(); iCurrent++)

        {
          const FixedLengthList_Component1d previousComponents1d_tmp = previousComponents1d;
          previousComponents1d = newPreviousComponents1d;
          newPreviousComponents1d = previousComponents1d_tmp;
        }
      } // for(s32 y=0; y<height; y++)

      return RESULT_OK;
    }

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