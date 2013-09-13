#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    // Extract 2d connected components from binaryImage
    // All extracted components are stored in a single list "extractedComponentPieces", which is sorted by id, y, then xStart
    Result extract2dComponents(const Array_u8 &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList_Component2dPiece &extractedComponentPieces, MemoryStack scratch)
    {
      const s32 MAX_1D_COMPONENTS = binaryImage.get_size(1) / (minComponentWidth+1);
      const s32 MAX_2D_COMPONENTS = 50000;
      const s32 MAX_EQUIVALENT_ITERATIONS = 3;

      const s32 height = binaryImage.get_size(0);
      const s32 width = binaryImage.get_size(1);

      s32 numStored2dComponents = 0;

      extractedComponentPieces.Clear();

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
        const Component1d * restrict currentComponents1d_rowPointer = currentComponents1d.Pointer(0);
        const Component1d * restrict previousComponents1d_rowPointer = previousComponents1d.Pointer(0);
        const Component1d * restrict newPreviousComponents1d_rowPointer = newPreviousComponents1d.Pointer(0);

        //  currentComponents1d = extract1dComponents(binaryImg(y,:), minComponentWidth, maxSkipDistance);
        extract1dComponents(binaryImage_rowPointer, static_cast<s16>(width), minComponentWidth, maxSkipDistance, currentComponents1d);

        //  newPreviousComponents1d = zeros(num1dComponents_current, 3);
        newPreviousComponents1d.set_size(currentComponents1d.get_size());

        //  for iCurrent = 1:num1dComponents_current
        for(s32 iCurrent=0; iCurrent<currentComponents1d.get_size(); iCurrent++) {
          bool foundMatch = false;
          for(s32 iPrevious=0; iPrevious<previousComponents1d.get_size(); iPrevious++) {
            //% The current component matches the previous one if
            //% 1. the previous start is less-than-or-equal the current end, and
            //% 2. the previous end is greater-than-or-equal the current start

            //if previousComponents1d(iPrevious, 1) <= currentComponents1d(iCurrent, 2) &&...
            //previousComponents1d(iPrevious, 2) >= currentComponents1d(iCurrent, 1)
            if(previousComponents1d_rowPointer[iPrevious].xStart <= currentComponents1d_rowPointer[iCurrent].xEnd &&
              previousComponents1d_rowPointer[iPrevious].xEnd >= currentComponents1d_rowPointer[iCurrent].xStart) {
                if(!foundMatch) {
                  //% If this is the first match we've found, add this 1d
                  //% component to components2d, using that previous component's id.
                  foundMatch = true;
                  //componentId = previousComponents1d(iPrevious, 3);
                  //components2d(numStored1dComponents+1, :) = [componentId, y, currentComponents1d(iCurrent, 1:2)]; % [componentId, y, xStart, xEnd]
                  //numStored1dComponents = numStored1dComponents + 1;

                  //newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), componentId];
                  newPreviousComponents1d_rowPointer[iCurrent] = Component1d(currentComponents1d_rowPointer[iCurrent].xStart, currentComponents1d_rowPointer[iCurrent].xEnd, y);
                } else {
                  //% Update the lookup table for all of:
                  //% 1. The first matching component
                  //% 2. This iPrevious component.
                  //% 3. The newPreviousComponents1d(iCurrent) component
                  //previousId = previousComponents1d(iPrevious, 3);
                  //minId = min(componentId, previousId);
                  //equivalentComponents(componentId) = min(equivalentComponents(componentId), minId);
                  //equivalentComponents(previousId) = min(equivalentComponents(previousId), minId);
                  //newPreviousComponents1d(iCurrent, 3) = minId;
                }
            }
          }

          //% If none of the previous components matched, start a new id, equal
          //% to num2dComponents
          if(!foundMatch) {
            //componentId = numStored2dComponents + 1;
            //
            //newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), componentId];
            //components2d(numStored1dComponents+1, :) = [componentId, y, currentComponents1d(iCurrent, 1:2)]; % [componentId, y, xStart, xEnd]
            //
            //numStored1dComponents = numStored1dComponents + 1;
            //numStored2dComponents = numStored2dComponents + 1;
          } // if(!foundMatch)
        } // for(s32 iCurrent=0; iCurrent<currentComponents1d.get_size(); iCurrent++)

        {
          const FixedLengthList_Component1d previousComponents1d_tmp = previousComponents1d;
          previousComponents1d = newPreviousComponents1d;
          newPreviousComponents1d = previousComponents1d_tmp;
        }
      } // for(s32 y=0; y<height; y++)

      //% After all the initial 2d labels have been created, go through
      //% equivalentComponents, and update equivalentComponents internally
      for(s32 iEquivalent=0; iEquivalent<MAX_EQUIVALENT_ITERATIONS; iEquivalent++) {
        //    changes = 0;
        //    for iComponent = 1:numStored2dComponents
        //        minNeighbor = equivalentComponents(iComponent);
        //
        //        if equivalentComponents(minNeighbor) ~= minNeighbor
        //            while equivalentComponents(minNeighbor) ~= minNeighbor
        //                minNeighbor = equivalentComponents(minNeighbor);
        //            end
        //            changes = changes + 1;
        //            equivalentComponents(iComponent) = minNeighbor;
        //        end
        //    end
        //
        //    if changes == 0
        //        break;
        //    end
      } // for(s32 iEquivalent=0; iEquivalent<MAX_EQUIVALENT_ITERATIONS; iEquivalent++)

      //% disp(components2d(1:numStored1dComponents, :));
      //% Replace the id of all 1d components with their minimum equivalent id
      for(s32 iComponent=0; iComponent<extractedComponentPieces.get_size(); iComponent++) {
        //    components2d(iComponent, 1) = equivalentComponents(components2d(iComponent, 1));
      }

      //% disp(components2d);

      //% Sort all 1D components by id, y, then x
      //components2d_packed = sortrows(components2d);

      // TODO: convert the pieces into regular components?

      return RESULT_OK;
    }

    Result extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList_Component1d &extractedComponentPieces)
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
                extractedComponentPieces.PushBack(Component1d(componentStart, x-numSkipped, -1));
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
          extractedComponentPieces.PushBack(Component1d(componentStart, binaryImageWidth-numSkipped-1, -1));
        }
      }

      return RESULT_OK;
    } // Result extract1dComponents(const u8 * restrict binaryImageRow, const s32 binaryImageWidth, const s32 minComponentWidth, FixedLengthList_Point_s16 &extractedComponentPieces)
  } // namespace Embedded
} // namespace Anki