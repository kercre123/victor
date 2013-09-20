#include "anki/embeddedVision/visionKernels_connectedComponents.h"

namespace Anki
{
  namespace Embedded
  {
    // Extract 2d connected components from binaryImage
    // All extracted components are stored in a single list "extractedComponents", which is sorted by id, y, then xStart
    IN_DDR Result Extract2dComponents(const Array<u8> &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList<ConnectedComponentSegment> &extractedComponents, MemoryStack scratch)
    {
      const s32 MAX_1D_COMPONENTS = binaryImage.get_size(1) / (minComponentWidth+1);
      const u16 MAX_2D_COMPONENTS = static_cast<u16>(extractedComponents.get_maximumSize());
      const s32 MAX_EQUIVALENT_ITERATIONS = 3;

      const s32 height = binaryImage.get_size(0);
      const s32 width = binaryImage.get_size(1);

      u16 numStored2dComponents = 0;

      extractedComponents.Clear();

      FixedLengthList<ConnectedComponentSegment> currentComponents1d(MAX_1D_COMPONENTS, scratch);
      FixedLengthList<ConnectedComponentSegment> previousComponents1d(MAX_1D_COMPONENTS, scratch);
      FixedLengthList<ConnectedComponentSegment> newPreviousComponents1d(MAX_1D_COMPONENTS, scratch);

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

        //currentComponents1d.Print();
        //printf("Some: %d %d %d\n", currentComponents1d.Pointer(0)->xStart, currentComponents1d.Pointer(0)->xEnd, currentComponents1d.Pointer(0)->y);

        // newPreviousComponents1d = zeros(num1dComponents_current, 3);
        newPreviousComponents1d.set_size(currentComponents1d.get_size());

        // for iCurrent = 1:num1dComponents_current
        for(s32 iCurrent=0; iCurrent<currentComponents1d.get_size(); iCurrent++) {
          bool foundMatch = false;
          u16 firstMatchedPreviousId = u16_MAX;

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
                  const Result result = extractedComponents.PushBack(newComponent);

                  // TODO: sort the components before returning
                  AnkiConditionalWarnAndReturnValue(result == RESULT_OK, RESULT_FAIL, "extract2dComponents", "Extracted maximum number of 2d components");

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

            const Result result = extractedComponents.PushBack(newComponent);

            // TODO: sort the components before returning
            AnkiConditionalWarnAndReturnValue(result == RESULT_OK, RESULT_FAIL, "extract2dComponents", "Extracted maximum number of 2d components");
          } // if(!foundMatch)
        } // for(s32 iCurrent=0; iCurrent<currentComponents1d.get_size(); iCurrent++)

        // Update previousComponents1d to be newPreviousComponents1d
        Swap(previousComponents1d, newPreviousComponents1d);
        //SWAP(FixedLengthList<ConnectedComponentSegment>, previousComponents1d, newPreviousComponents1d);
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
        }

        AnkiConditionalErrorAndReturnValue(numChanges == MAX_RECURSION_LEVEL, RESULT_FAIL, "extract2dComponents", "Issue with equivalentComponents minimum search");
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
      return result;

      //      return RESULT_OK;
    }

    IN_DDR Result Extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList<ConnectedComponentSegment> &extractedComponents)
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
                //                printf("one: %d %d\n", componentStart, x-numSkipped);
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
          //          printf("two: %d %d\n", componentStart, binaryImageWidth-numSkipped-1);
          extractedComponents.PushBack(ConnectedComponentSegment(componentStart, binaryImageWidth-numSkipped-1));
        }
      }

      return RESULT_OK;
    } // Result Extract1dComponents(const u8 * restrict binaryImageRow, const s32 binaryImageWidth, const s32 minComponentWidth, FixedLengthList<Point<s16> > &extractedComponents)

    // Sort the components by id (the ids are sorted in increasing value, but with zero at the end {1...MAX_VALUE,0}), then y, then xStart
    // TODO: determine how fast this method is, then suggest usage
    IN_DDR Result SortConnectedComponentSegments(FixedLengthList<ConnectedComponentSegment> &components)
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
    } // Result SortConnectedComponentSegments(FixedLengthList<ConnectedComponentSegment> &components)

    // Iterate through components, and return the maximum id
    u16 FindMaximumId(const FixedLengthList<ConnectedComponentSegment> &components)
    {
      u16 maximumId = 0;
      const ConnectedComponentSegment * restrict components_constRowPointer = components.Pointer(0);
      for(s32 i=0; i<components.get_size(); i++) {
        maximumId = MAX(maximumId, components_constRowPointer[i].id);
      }

      return maximumId;
    }

    // Iterate through components, and compute the number of pixels for each
    // componentSizes must be at least sizeof(s32)*(maximumdId+1) bytes
    // Note: this is probably inefficient, compared with interlacing the loops in a kernel
    Result ComputeComponentSizes(const FixedLengthList<ConnectedComponentSegment> &components, s32 * restrict componentSizes, const u16 maximumId)
    {
      AnkiConditionalErrorAndReturnValue(componentSizes, RESULT_FAIL, "ComputeComponentSizes", "componentSizes is NULL");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL, "ComputeComponentSizes", "components is not valid");

      memset(componentSizes, 0, sizeof(componentSizes[0])*(maximumId+1));

      const ConnectedComponentSegment * restrict components_constRowPointer = components.Pointer(0);

      for(s32 i=0; i<components.get_size(); i++) {
        const u16 id = components_constRowPointer[i].id;
        const s16 length = components_constRowPointer[i].xEnd - components_constRowPointer[i].xStart + 1;

        componentSizes[id] += length;
      }

      return RESULT_OK;
    }

    // Iterate through components, and compute the number of componentSegments that have each id
    // componentSizes must be at least sizeof(s32)*(maximumdId+1) bytes
    // Note: this is probably inefficient, compared with interlacing the loops in a kernel
    Result ComputeNumComponentSegments(const FixedLengthList<ConnectedComponentSegment> &components, s32 * restrict numComponentSegments, const u16 maximumId)
    {
      AnkiConditionalErrorAndReturnValue(numComponentSegments, RESULT_FAIL, "ComputeComponentSizes", "numComponentSegments is NULL");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL, "ComputeComponentSizes", "components is not valid");

      memset(numComponentSegments, 0, sizeof(numComponentSegments[0])*(maximumId+1));

      const ConnectedComponentSegment * restrict components_constRowPointer = components.Pointer(0);

      for(s32 i=0; i<components.get_size(); i++) {
        const u16 id = components_constRowPointer[i].id;

        numComponentSegments[id]++;
      }

      return RESULT_OK;
    }

    // The list of components may have unused ids. This function compresses the set of ids, so that
    // max(ids) == numberOfUniqueValues(ids). The function then returns the maximum id. For example, the list of ids {0,4,5,7} would be
    // changed to {0,1,2,3}, and it would return 3.
    //
    // For a components parameter that has a maximum id of N, this function requires
    // 3n + 3 bytes of scratch.
    //
    // TODO: If scratch usage is a bigger issue than computation time, this could be done with a bitmask
    IN_DDR u16 CompressConnectedComponentSegmentIds(FixedLengthList<ConnectedComponentSegment> &components, MemoryStack scratch)
    {
      s32 numUsedIds = 0;

      const ConnectedComponentSegment * restrict components_constRowPointer = components.Pointer(0);

      const u16 maximumId = FindMaximumId(components);

      // Compute the number of unique components
      u8 *usedIds = reinterpret_cast<u8*>(scratch.Allocate(maximumId+1));
      u16 *idLookupTable = reinterpret_cast<u16*>(scratch.Allocate( sizeof(u16)*(maximumId+1) ));

      AnkiConditionalErrorAndReturnValue(usedIds, 0, "CompressConnectedComponentSegmentIds", "Couldn't allocate usedIds");
      AnkiConditionalErrorAndReturnValue(idLookupTable, 0, "CompressConnectedComponentSegmentIds", "Couldn't allocate idLookupTable");

      memset(usedIds, 0, sizeof(usedIds[0])*(maximumId+1));

      for(s32 i=0; i<components.get_size(); i++) {
        const u16 id = components_constRowPointer[i].id;
        usedIds[id] = 1;
      }

      for(s32 i=0; i<=maximumId; i++) {
        numUsedIds += usedIds[i];
      }

      // Create a mapping table from original id to compressed id
      idLookupTable[0] = 0;
      u16 currentCompressedId = 1;
      for(s32 i=1; i<=maximumId; i++) {
        if(usedIds[i] != 0) {
          idLookupTable[i] = currentCompressedId++;
        }
      }

      ConnectedComponentSegment * restrict components_rowPointer = components.Pointer(0);
      // Convert the id numbers to the compressed id numbers
      for(s32 i=0; i<components.get_size(); i++) {
        components_rowPointer[i].id = idLookupTable[components_rowPointer[i].id];
      }

      return currentCompressedId - 1;
    } // Result CompressConnectedComponentSegmentIds(FixedLengthList<ConnectedComponentSegment> &components, MemoryStack scratch)

    // Goes through the list components, and computes the number of pixels for each.
    // For any componentId with less than minimumNumPixels pixels, all ConnectedComponentSegment with that id will have their ids set to zero
    //
    // For a components parameter that has a maximum id of N, this function requires
    // 4n + 4 bytes of scratch.
    IN_DDR Result MarkSmallOrLargeComponentsAsInvalid(FixedLengthList<ConnectedComponentSegment> &components, const s32 minimumNumPixels, const s32 maximumNumPixels, MemoryStack scratch)
    {
      const u16 maximumId = FindMaximumId(components);

      const ConnectedComponentSegment * restrict components_constRowPointer = components.Pointer(0);

      s32 *componentSizes = reinterpret_cast<s32*>(scratch.Allocate( sizeof(s32)*(maximumId+1) ));

      AnkiConditionalErrorAndReturnValue(componentSizes, RESULT_FAIL, "MarkSmallOrLargeComponentsAsInvalid", "Couldn't allocate componentSizes");

      memset(componentSizes, 0, sizeof(componentSizes[0])*(maximumId+1));

      for(s32 i=0; i<components.get_size(); i++) {
        const u16 id = components_constRowPointer[i].id;
        const s16 length = components_constRowPointer[i].xEnd - components_constRowPointer[i].xStart + 1;

        componentSizes[id] += length;
      }

      // TODO: mark the components as invalid, instead of doing all the checks every time

      ConnectedComponentSegment * restrict components_rowPointer = components.Pointer(0);
      for(s32 i=0; i<components.get_size(); i++) {
        const u16 id = components_rowPointer[i].id;

        if(componentSizes[id] < minimumNumPixels || componentSizes[id] > maximumNumPixels) {
          components_rowPointer[i].id = 0;
        }
      }

      return RESULT_OK;
    }

    // Goes through the list components, and computes the "solidness", which is the ratio of
    // "numPixels / (boundingWidth*boundingHeight)". For any componentId with that is too solid or
    // sparse (opposite of solid), all ConnectedComponentSegment with that id will have their ids
    // set to zero
    //
    // The SQ26.5 parameter sparseMultiplyThreshold is set so that a component is invalid if
    // "sparseMultiplyThreshold*numPixels < boundingWidth*boundingHeight".
    // A resonable value is between 5<<5 = 160 and 100<<5 = 3200.
    //
    // The SQ26.5 parameter solidMultiplyThreshold is set so that a component is invalid if
    // "solidMultiplyThreshold*numPixels > boundingWidth*boundingHeight".
    // A resonable value is between 1.5*pow(2,5) = 48 and 5<<5 = 160.
    //
    // Note: This can overflow if the number of pixels is greater than 2^26 (a bit more Ultra-HD resolution)
    //
    // For a components parameter that has a maximum id of N, this function requires
    // 8N + 8 bytes of scratch.
    IN_DDR Result MarkSolidOrSparseComponentsAsInvalid(FixedLengthList<ConnectedComponentSegment> &components, const s32 sparseMultiplyThreshold, const s32 solidMultiplyThreshold, MemoryStack scratch)
    {
      const u16 maximumId = FindMaximumId(components);

      const ConnectedComponentSegment * restrict components_constRowPointer = components.Pointer(0);

      // Storage for the bounding boxes.
      // A bounding box is (minX, minY) -> (maxX, maxY)
      s16 *minX = reinterpret_cast<s16*>(scratch.Allocate( sizeof(s16)*(maximumId+1) ));
      s16 *maxX = reinterpret_cast<s16*>(scratch.Allocate( sizeof(s16)*(maximumId+1) ));
      s16 *minY = reinterpret_cast<s16*>(scratch.Allocate( sizeof(s16)*(maximumId+1) ));
      s16 *maxY = reinterpret_cast<s16*>(scratch.Allocate( sizeof(s16)*(maximumId+1) ));
      s32 *componentSizes = reinterpret_cast<s32*>(scratch.Allocate( sizeof(s32)*(maximumId+1) ));

      AnkiConditionalErrorAndReturnValue(minX && maxX && minY && maxY && componentSizes,
        RESULT_FAIL, "MarkSolidOrSparseComponentsAsInvalid", "Couldn't allocate minX, maxX, minY, maxY, or componentSizes");

      memset(componentSizes, 0, sizeof(componentSizes[0])*(maximumId+1));

      for(u16 i=0; i<=maximumId; i++) {
        minX[i] = s16_MAX;
        maxX[i] = s16_MIN;
        minY[i] = s16_MAX;
        maxY[i] = s16_MIN;
      }

      // Compute the bounding box of each component ID
      // A bounding box is (minX, minY) -> (maxX, maxY)
      for(s32 i=0; i<components.get_size(); i++) {
        const u16 id = components_constRowPointer[i].id;
        const s16 y = components_constRowPointer[i].y;
        const s16 xStart = components_constRowPointer[i].xStart;
        const s16 xEnd = components_constRowPointer[i].xEnd;

        const s16 length = components_constRowPointer[i].xEnd - components_constRowPointer[i].xStart + 1;

        componentSizes[id] += length;

        minX[id] = MIN(minX[id], xStart);
        maxX[id] = MAX(maxX[id], xEnd);

        minY[id] = MIN(minY[id], y);
        maxY[id] = MAX(maxY[id], y);
      }

      for(u16 i=0; i<=maximumId; i++) {
        // The SQ26.5 parameter sparseMultiplyThreshold is set so that a component is invalid if
        // "sparseMultiplyThreshold*numPixels < boundingWidth*boundingHeight".
        // A resonable value is between 5<<5 = 160 and 100<<5 = 3200.
        //
        // The SQ26.5 parameter solidMultiplyThreshold is set so that a component is invalid if
        // "solidMultiplyThreshold*numPixels > boundingWidth*boundingHeight".
        // A resonable value is between 1.5*pow(2,5) = 48 and 5<<5 = 160.

        const s32 boundingArea = ((maxX[i]-minX[i]+1)*(maxY[i]-minY[i]+1)) << 5; // SQ26.5

        const s32 sparseMultiply = sparseMultiplyThreshold*componentSizes[i]; // (SQ26.5 * SQ31.0) -> SQ26.5
        const s32 solidMultiply = solidMultiplyThreshold*componentSizes[i]; // (SQ26.5 * SQ31.0) -> SQ26.5

        if(sparseMultiply < boundingArea || solidMultiply > boundingArea) {
          minX[i] = s16_MIN; // Set to invalid
        }
      }

      ConnectedComponentSegment * restrict components_rowPointer = components.Pointer(0);
      for(s32 i=0; i<components.get_size(); i++) {
        const u16 id = components_rowPointer[i].id;

        if(minX[id] < 0) {
          components_rowPointer[i].id = 0;
        }
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki