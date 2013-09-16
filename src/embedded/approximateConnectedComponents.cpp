#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    // Extract 2d connected components from binaryImage
    // All extracted components are stored in a single list "extractedComponents", which is sorted by id, y, then xStart
    Result Extract2dComponents(const Array_u8 &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList_ConnectedComponentSegment &extractedComponents, MemoryStack scratch)
    {
      const s32 MAX_1D_COMPONENTS = binaryImage.get_size(1) / (minComponentWidth+1);
      const u16 MAX_2D_COMPONENTS = static_cast<u16>(extractedComponents.get_maximumSize());
      const s32 MAX_EQUIVALENT_ITERATIONS = 3;

      const s32 height = binaryImage.get_size(0);
      const s32 width = binaryImage.get_size(1);

      u16 numStored2dComponents = 0;

      extractedComponents.Clear();

      FixedLengthList_ConnectedComponentSegment currentComponents1d(MAX_1D_COMPONENTS, scratch);
      FixedLengthList_ConnectedComponentSegment previousComponents1d(MAX_1D_COMPONENTS, scratch);
      FixedLengthList_ConnectedComponentSegment newPreviousComponents1d(MAX_1D_COMPONENTS, scratch);

      u16 *equivalentComponents = reinterpret_cast<u16*>(scratch.Allocate(MAX_2D_COMPONENTS*sizeof(u16)));
      for(u16 i=0; i<MAX_2D_COMPONENTS; i++) {
        equivalentComponents[i] = i;
      }

      //for y = 1:size(binaryImg, 1)
      for(s16 y=0; y<height; y++) {
        const u8 * restrict binaryImage_rowPointer = binaryImage.Pointer(y,0);
        const ConnectedComponentSegment * restrict currentComponents1d_rowPointer = currentComponents1d.Pointer(0);
        const ConnectedComponentSegment * restrict previousComponents1d_rowPointer = previousComponents1d.Pointer(0);
        ConnectedComponentSegment * restrict newPreviousComponents1d_rowPointer = newPreviousComponents1d.Pointer(0);

        // currentComponents1d = Extract1dComponents(binaryImg(y,:), minComponentWidth, maxSkipDistance);
        Extract1dComponents(binaryImage_rowPointer, static_cast<s16>(width), minComponentWidth, maxSkipDistance, currentComponents1d);
        //currentComponents1d.Print();

        // newPreviousComponents1d = zeros(num1dComponents_current, 3);
        newPreviousComponents1d.set_size(currentComponents1d.get_size());

        // for iCurrent = 1:num1dComponents_current
        for(s32 iCurrent=0; iCurrent<currentComponents1d.get_size(); iCurrent++) {
          bool foundMatch = false;
          u16 firstMatchedPreviousId = MAX_uint16_T;

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
                  //% component to extractedComponents, using that previous component's id.
                  foundMatch = true;
                  firstMatchedPreviousId = previousComponents1d_rowPointer[iPrevious].id;

                  const ConnectedComponentSegment newComponent(currentComponents1d_rowPointer[iCurrent].xStart, currentComponents1d_rowPointer[iCurrent].xEnd, y, firstMatchedPreviousId);

                  //extractedComponents(numStored1dComponents+1, :) = [firstMatchedPreviousId, y, currentComponents1d(iCurrent, 1:2)]; % [firstMatchedPreviousId, y, xStart, xEnd]
                  //newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), firstMatchedPreviousId];
                  if(extractedComponents.PushBack(newComponent) != RESULT_OK) {
                    // TODO: sort the components before returning
                    DASWarnAndReturnValue(RESULT_FAIL, "extract2dComponents", "Extracted maximum number of 2d components");
                  }

                  newPreviousComponents1d_rowPointer[iCurrent] = newComponent;
                } else { // if(!foundMatch)
                  //% Update the lookup table for all of:
                  //% 1. The first matching component
                  //% 2. This iPrevious component.
                  //% 3. The newPreviousComponents1d(iCurrent) component
                  // TODO: Think if this can be changed to make the final equivalence matching faster

                  const u16 previousId = previousComponents1d_rowPointer[iPrevious].id;
                  const u16 minId = MIN(firstMatchedPreviousId, previousId);

                  equivalentComponents[firstMatchedPreviousId] = MIN(equivalentComponents[firstMatchedPreviousId], minId);
                  equivalentComponents[previousId] = MIN(equivalentComponents[previousId], minId);
                  newPreviousComponents1d_rowPointer[iCurrent].id = minId;
                } // if(!foundMatch) ... else
            } // if(previousComponents1d_rowPointer[iPrevious].xStart
          } // for(s32 iPrevious=0; iPrevious<previousComponents1d.get_size(); iPrevious++)

          // If none of the previous components matched, start a new id, equal to num2dComponents
          if(!foundMatch) {
            firstMatchedPreviousId = 1 + numStored2dComponents++;

            const ConnectedComponentSegment newComponent(currentComponents1d_rowPointer[iCurrent].xStart, currentComponents1d_rowPointer[iCurrent].xEnd, y, firstMatchedPreviousId);

            //newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), firstMatchedPreviousId];
            //extractedComponents(numStored1dComponents+1, :) = [firstMatchedPreviousId, y, currentComponents1d(iCurrent, 1:2)]; % [firstMatchedPreviousId, y, xStart, xEnd]
            newPreviousComponents1d_rowPointer[iCurrent] = newComponent;

            if(extractedComponents.PushBack(newComponent) != RESULT_OK) {
              // TODO: sort the components before returning
              DASWarnAndReturnValue(RESULT_FAIL, "extract2dComponents", "Extracted maximum number of 2d components");
            }
          } // if(!foundMatch)
        } // for(s32 iCurrent=0; iCurrent<currentComponents1d.get_size(); iCurrent++)

        // Update previousComponents1d to be newPreviousComponents1d
        std::swap(previousComponents1d, newPreviousComponents1d);
      } // for(s32 y=0; y<height; y++)

      //% After all the initial 2d labels have been created, go through
      //% equivalentComponents, and update equivalentComponents internally
      for(s32 iEquivalent=0; iEquivalent<MAX_EQUIVALENT_ITERATIONS; iEquivalent++) {
        const s32 MAX_RECURSION_LEVEL = 1000; // If it gets anywhere near this high, there's a bug
        s32 numChanges = 0;

        for(s32 iComponent=0; iComponent<extractedComponents.get_size(); iComponent++) {
          // Trace back along the equivalentComponents list, to find the joined component with the minimum id
          u16 minNeighbor = equivalentComponents[iComponent];
          if(equivalentComponents[minNeighbor] != minNeighbor) {
            for(s32 recursionLevel=0; recursionLevel<MAX_RECURSION_LEVEL; recursionLevel++) {
              if(equivalentComponents[minNeighbor] == minNeighbor)
                break;

              minNeighbor = equivalentComponents[minNeighbor]; // "Recurse" to the next lower component in the list
            }
            numChanges++;
            equivalentComponents[minNeighbor] = minNeighbor;
          } // if(equivalentComponents[minNeighbor] != minNeighbor)
        } //  for(s32 iComponent=0; iComponent<extractedComponents.get_size(); iComponent++)

        if(numChanges == 0) {
          break;
        } else if(numChanges == MAX_RECURSION_LEVEL) {
          DASErrorAndReturnValue(RESULT_FAIL, "extract2dComponents", "Issue with equivalentComponents minimum search");
        }
      } // for(s32 iEquivalent=0; iEquivalent<MAX_EQUIVALENT_ITERATIONS; iEquivalent++)

      //% disp(extractedComponents(1:numStored1dComponents, :));
      //extractedComponents.Print();

      //% Replace the id of all 1d components with their minimum equivalent id
      {
        ConnectedComponentSegment * restrict extractedComponents_rowPointer = extractedComponents.Pointer(0);
        for(s32 iComponent=0; iComponent<extractedComponents.get_size(); iComponent++) {
          // extractedComponents(iComponent, 1) = equivalentComponents(extractedComponents(iComponent, 1));
          extractedComponents_rowPointer[iComponent].id = equivalentComponents[extractedComponents_rowPointer[iComponent].id];
        }
      }

      //% disp(extractedComponents);
      //extractedComponents.Print();

      //% Sort all 1D components by id, y, then x
      const Result result = SortConnectedComponentSegments(extractedComponents);

      // TODO: convert the pieces into regular components?

      return result;
    }

    Result Extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList_ConnectedComponentSegment &extractedComponents)
    {
      bool onComponent;
      s16 componentStart;
      s16 numSkipped = 0;

      extractedComponents.Clear();

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
                extractedComponents.PushBack(ConnectedComponentSegment(componentStart, x-numSkipped));
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
          extractedComponents.PushBack(ConnectedComponentSegment(componentStart, binaryImageWidth-numSkipped-1));
        }
      }

      return RESULT_OK;
    } // Result Extract1dComponents(const u8 * restrict binaryImageRow, const s32 binaryImageWidth, const s32 minComponentWidth, FixedLengthList_Point_s16 &extractedComponents)

    // Sort the components by id, y, then xStart
    // TODO: determine how fast this method is, then suggest usage
    Result SortConnectedComponentSegments(FixedLengthList_ConnectedComponentSegment &components)
    {
      // Performs insersion sort
      // TODO: do bucket sort by id first, then run this

      ConnectedComponentSegment * restrict components_rowPointer = components.Pointer(0);

      for(s32 i=1; i<components.get_size(); i++) {
        const ConnectedComponentSegment segmentToInsert = components_rowPointer[i];

        s32 holePosition = i;

        while(holePosition > 0 && CompareConnectedComponentSegments(segmentToInsert, components_rowPointer[holePosition-1]) < 0) {
          components_rowPointer[holePosition] = components_rowPointer[holePosition-1];
          holePosition--;
        }

        components_rowPointer[holePosition] = segmentToInsert;
      } // for(s32 i=0; i<components.size(); i++)

      return RESULT_OK;
    } // Result SortConnectedComponentSegments(FixedLengthList_ConnectedComponentSegment &components)
  } // namespace Embedded
} // namespace Anki