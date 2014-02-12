/**
File: connectedComponents.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/connectedComponents.h"

namespace Anki
{
  namespace Embedded
  {
    static Result lastResult;

    ConnectedComponentSegment::ConnectedComponentSegment()
      : xStart(-1), xEnd(-1), y(-1), id(0)
    {
    } // ConnectedComponentSegment::ConnectedComponentSegment()

    ConnectedComponentSegment::ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y, const u16 id)
      : xStart(xStart), xEnd(xEnd), y(y), id(id)
    {
    } // ConnectedComponentSegment::ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y, const u16 id)

    void ConnectedComponentSegment::Print() const
    {
      //printf("[%d: (%d->%d, %d)] ", static_cast<s32>(this->id), static_cast<s32>(this->xStart), static_cast<s32>(this->xEnd), static_cast<s32>(this->y));
      printf("[%d: (%d->%d, %d)] ", this->id, this->xStart, this->xEnd, this->y);
    } // void ConnectedComponentSegment::Print() const

    bool ConnectedComponentSegment::operator== (const ConnectedComponentSegment &component2) const
    {
      if((this->xStart == component2.xStart) &&
        (this->xEnd == component2.xEnd) &&
        (this->y == component2.y) &&
        (this->id == component2.id)) {
          return true;
      }

      return false;
    }

    Result ConnectedComponents::Extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList<ConnectedComponentSegment> &components)
    {
      bool onComponent;
      s16 componentStart;
      s16 numSkipped = 0;

      components.Clear();

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
                components.PushBack(ConnectedComponentSegment(componentStart, x-numSkipped));
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
          components.PushBack(ConnectedComponentSegment(componentStart, binaryImageWidth-numSkipped-1));
        }
      }

      return RESULT_OK;
    } // Result Extract1dComponents(const u8 * restrict binaryImageRow, const s32 binaryImageWidth, const s32 minComponentWidth, FixedLengthList<Point<s16> > &components)

    ConnectedComponents::ConnectedComponents()
      : isSortedInId(true), isSortedInY(true), isSortedInX(true), maximumId(0), maxImageWidth(-1)
    {
    }

    // Constructor for a ConnectedComponents, pointing to user-allocated MemoryStack
    ConnectedComponents::ConnectedComponents(const s32 maxComponentSegments, const s32 maxImageWidth, MemoryStack &memory)
      : isSortedInId(true), isSortedInY(true), isSortedInX(true), maximumId(0), maxImageWidth(maxImageWidth)
    {
      AnkiConditionalError(maxComponentSegments > 0 && maxComponentSegments <= u16_MAX,
        "ConnectedComponents::ConnectedComponents", "maxComponentSegments must be greater than zero and less than 0xFFFF");

      this->components = FixedLengthList<ConnectedComponentSegment>(maxComponentSegments, memory);

      // Note: these are really only needed for the Extract2dComponents methods, so if the scratch
      //       memory is needed, these could be allocated later.
      this->previousComponents1d = FixedLengthList<ConnectedComponentSegment>(maxImageWidth, memory);
      this->currentComponents1d = FixedLengthList<ConnectedComponentSegment>(maxImageWidth, memory);
      this->newPreviousComponents1d = FixedLengthList<ConnectedComponentSegment>(maxImageWidth, memory);
      this->equivalentComponents = FixedLengthList<u16>(maxComponentSegments, memory);
    } // ConnectedComponents(const s32 maxComponentSegments, MemoryStack &memory)

    Result ConnectedComponents::Extract2dComponents_PerRow_Initialize()
    {
      const u16 MAX_2D_COMPONENTS = static_cast<u16>(components.get_maximumSize());

      components.Clear();

      this->maximumId = 0;

      u16 * restrict pEquivalentComponents = equivalentComponents.Pointer(0);
      for(s32 i=0; i<MAX_2D_COMPONENTS; i++) {
        pEquivalentComponents[i] = static_cast<u16>(i);
      }

      return RESULT_OK;
    } // Result ConnectedComponents::Extract2dComponents_PerRow_Initialize()

    Result ConnectedComponents::Extract2dComponents_PerRow_NextRow(const u8 * restrict binaryImageRow, const s32 imageWidth, const s16 whichRow, const s16 minComponentWidth, const s16 maxSkipDistance)
    {
      AnkiAssert(imageWidth <= maxImageWidth);

      //for(s32 y=0; y<imageHeight; y++) {
      const ConnectedComponentSegment * restrict pCurrentComponents1d = currentComponents1d.Pointer(0);
      const ConnectedComponentSegment * restrict pPreviousComponents1d = previousComponents1d.Pointer(0);
      ConnectedComponentSegment * restrict pNewPreviousComponents1d = newPreviousComponents1d.Pointer(0);

      u16 * restrict pEquivalentComponents = equivalentComponents.Pointer(0);

      ConnectedComponents::Extract1dComponents(binaryImageRow, static_cast<s16>(imageWidth), minComponentWidth, maxSkipDistance, currentComponents1d);

      const s32 numCurrentComponents1d = currentComponents1d.get_size();
      const s32 numPreviousComponents1d = previousComponents1d.get_size();

      newPreviousComponents1d.set_size(numCurrentComponents1d);

      for(s32 iCurrent=0; iCurrent<numCurrentComponents1d; iCurrent++) {
        bool foundMatch = false;
        u16 firstMatchedPreviousId = u16_MAX;

        for(s32 iPrevious=0; iPrevious<numPreviousComponents1d; iPrevious++) {
          // The current component matches the previous one if
          // 1. the previous start is less-than-or-equal the current end, and
          // 2. the previous end is greater-than-or-equal the current start

          if(pPreviousComponents1d[iPrevious].xStart <= pCurrentComponents1d[iCurrent].xEnd &&
            pPreviousComponents1d[iPrevious].xEnd >= pCurrentComponents1d[iCurrent].xStart) {
              if(!foundMatch) {
                // If this is the first match we've found, add this 1d
                // component to components, using that previous component's id.
                foundMatch = true;
                firstMatchedPreviousId = pPreviousComponents1d[iPrevious].id;

                const ConnectedComponentSegment newComponent(pCurrentComponents1d[iCurrent].xStart, pCurrentComponents1d[iCurrent].xEnd, whichRow, firstMatchedPreviousId);

                const Result result = components.PushBack(newComponent);

                AnkiConditionalErrorAndReturnValue(result == RESULT_OK, result, "extract2dComponents", "Extracted maximum number of 2d components");

                pNewPreviousComponents1d[iCurrent] = newComponent;
              } else { // if(!foundMatch)
                // Update the lookup table for all of:
                // 1. The first matching component
                // 2. This iPrevious component.
                // 3. The newPreviousComponents1d(iCurrent) component
                // TODO: Think if this can be changed to make the final equivalence matching faster

                const u16 previousId = pPreviousComponents1d[iPrevious].id;
                const u16 minId = MIN(MIN(MIN(firstMatchedPreviousId, previousId), pEquivalentComponents[previousId]), pEquivalentComponents[firstMatchedPreviousId]);

                pEquivalentComponents[pEquivalentComponents[firstMatchedPreviousId]] = minId;
                pEquivalentComponents[firstMatchedPreviousId] = minId;
                pEquivalentComponents[previousId] = minId;
                pNewPreviousComponents1d[iCurrent].id = minId;
              } // if(!foundMatch) ... else
          } // if(pPreviousComponents1d[iPrevious].xStart
        } // for(s32 iPrevious=0; iPrevious<numPreviousComponents1d; iPrevious++)

        // If none of the previous components matched, start a new id, equal to num2dComponents
        if(!foundMatch) {
          this->maximumId++;
          firstMatchedPreviousId = this->maximumId;

          const ConnectedComponentSegment newComponent(pCurrentComponents1d[iCurrent].xStart, pCurrentComponents1d[iCurrent].xEnd, whichRow, firstMatchedPreviousId);

          pNewPreviousComponents1d[iCurrent] = newComponent;

          const Result result = components.PushBack(newComponent);
#pragma unused(result)

          AnkiConditionalWarnAndReturnValue(result == RESULT_OK, result, "extract2dComponents", "Extracted maximum number of 2d components");
        } // if(!foundMatch)
      } // for(s32 iCurrent=0; iCurrent<numCurrentComponents1d; iCurrent++)

      // Update previousComponents1d to be newPreviousComponents1d
      Swap(previousComponents1d, newPreviousComponents1d);
      //} // for(s32 y=0; y<imageHeight; y++)

      return RESULT_OK;
    } // Result ConnectedComponents::Extract2dComponents_PerRow_NextRow(const Array<u8> &binaryImageRow, const s16 minComponentWidth, const s16 maxSkipDistance)

    Result ConnectedComponents::Extract2dComponents_PerRow_Finalize()
    {
      const s32 MAX_EQUIVALENT_ITERATIONS = 3;
      u16 * restrict pEquivalentComponents = equivalentComponents.Pointer(0);

      const s32 numComponents = components.get_size();

      // After all the initial 2d labels have been created, go through
      // equivalentComponents, and update equivalentComponents internally
      for(s32 iEquivalent=0; iEquivalent<MAX_EQUIVALENT_ITERATIONS; iEquivalent++) {
        const s32 MAX_RECURSION_LEVEL = 1000; // If it gets anywhere near this high, there's a bug
        s32 numChanges = 0;

        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          // Trace back along the equivalentComponents list, to find the joined component with the minimum id
          u16 minNeighbor = pEquivalentComponents[iComponent];
          if(pEquivalentComponents[minNeighbor] != minNeighbor) {
            for(s32 recursionLevel=0; recursionLevel<MAX_RECURSION_LEVEL; recursionLevel++) {
              if(pEquivalentComponents[minNeighbor] == minNeighbor)
                break;

              minNeighbor = pEquivalentComponents[minNeighbor]; // "Recurse" to the next lower component in the list
            }
            numChanges++;

            pEquivalentComponents[iComponent] = minNeighbor;
          } // if(pEquivalentComponents[minNeighbor] != minNeighbor)
        } //  for(s32 iComponent=0; iComponent<numComponents; iComponent++)

        if(numChanges == 0) {
          break;
        }

        AnkiConditionalWarnAndReturnValue(numChanges < MAX_RECURSION_LEVEL, RESULT_FAIL, "extract2dComponents", "Issue with equivalentComponents minimum search");
      } // for(s32 iEquivalent=0; iEquivalent<MAX_EQUIVALENT_ITERATIONS; iEquivalent++)

      // Replace the id of all 1d components with their minimum equivalent id
      {
        ConnectedComponentSegment * restrict pComponents = components.Pointer(0);
        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          pComponents[iComponent].id = pEquivalentComponents[pComponents[iComponent].id];
        }
      }

      FindMaximumId();

      // TODO: put this back if it is needed by some other algorithm
      // Sort all 1D components by id, y, then x
      // const Result result = SortConnectedComponentSegmentsById();
      // const Result result = SortConnectedComponentSegments();
      // return result;

      return RESULT_OK;
    } // Result ConnectedComponents::Extract2dComponents_PerRow_Finalize()

    Result ConnectedComponents::Extract2dComponents_FullImage(const Array<u8> &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance)
    {
      const s32 imageHeight = binaryImage.get_size(0);
      const s32 imageWidth = binaryImage.get_size(1);

      if((lastResult = Extract2dComponents_PerRow_Initialize()) != RESULT_OK)
        return lastResult;

      for(s32 y=0; y<imageHeight; y++) {
        const u8 * restrict pBinaryImage = binaryImage.Pointer(y,0);

        if((lastResult = Extract2dComponents_PerRow_NextRow(pBinaryImage, imageWidth, y, minComponentWidth, maxSkipDistance)) != RESULT_OK)
          return lastResult;
      } // for(s32 y=0; y<imageHeight; y++)

      if((lastResult = Extract2dComponents_PerRow_Finalize()) != RESULT_OK)
        return lastResult;

      return RESULT_OK;
    }

    // Sort the components by id (the ids are sorted in increasing value, but with zero at the end {1...MAX_VALUE,0}), then y, then xStart
    // TODO: determine how fast this method is, then suggest usage
    Result ConnectedComponents::SortConnectedComponentSegments()
    {
      // Performs insersion sort
      // TODO: do bucket sort by id first, then run this

      ConnectedComponentSegment * restrict pComponents = components.Pointer(0);

      const s32 numComponents = components.get_size();

      for(s32 iComponent=1; iComponent<numComponents; iComponent++) {
        const ConnectedComponentSegment segmentToInsert = pComponents[iComponent];

        s32 holePosition = iComponent;

        while(holePosition > 0 && CompareConnectedComponentSegments(segmentToInsert, pComponents[holePosition-1]) < 0) {
          pComponents[holePosition] = pComponents[holePosition-1];
          holePosition--;
        }

        pComponents[holePosition] = segmentToInsert;
      } // for(s32 i=0; i<components.size(); i++)

      this->isSortedInId = true;
      this->isSortedInY = true;
      this->isSortedInX = true;

      return RESULT_OK;
    } // Result SortConnectedComponentSegments(FixedLengthList<ConnectedComponentSegment> &components)

    // Sort the components by id. This will retain the original ordering as well, so if the
    // components are already sorted in y, the output of this method will be sorted in id and y.
    // Requires numValidComponentSegments*sizeof(ConnectedComponentSegment) bytes of scratch
    Result ConnectedComponents::SortConnectedComponentSegmentsById(MemoryStack scratch)
    {
      // Performs bucket sort
      FixedLengthList<u16> idCounts(this->maximumId + 1, scratch);

      idCounts.SetZero();

      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);
      u16 * restrict pIdCounts = idCounts.Pointer(0);

      // Count the number of ComponenentSegments that have each id
      s32 numValidComponentSegments = 0;
      s32 numComponents = this->components.get_size();
      for(s32 i=0; i<numComponents; i++) {
        const u16 id = pConstComponents[i].id;

        if(id != 0) {
          pIdCounts[id]++;
          numValidComponentSegments++;
        }
      } // for(s32 i=0; i<numComponents; i++)

      if(numValidComponentSegments == 0) {
        this->components.set_size(0);
        this->isSortedInId = true;
        return RESULT_OK;
      }

      // Note: this could use a lot of space numValidComponentSegments*sizeof(ConnectedComponentSegment) = numValidComponentSegments*8
      FixedLengthList<ConnectedComponentSegment> componentsTmp(numValidComponentSegments, scratch);

      // Could fail if we don't have enough scratch space
      if(!componentsTmp.IsValid())
        return RESULT_FAIL_INVALID_OBJECT;

      // Convert the absolute count to a cumulative count (ignoring id == zero)
      u16 totalCount = 0;
      for(s32 id=1; id<=this->maximumId; id++) {
        const u16 lastCount = pIdCounts[id];

        pIdCounts[id] = totalCount;

        totalCount += lastCount;
      }

      // Go through the list, and copy each ComponentSegment into the correct "bucket"
      ConnectedComponentSegment * restrict pComponentsTmp = componentsTmp.Pointer(0);
      for(s32 oldIndex=0; oldIndex<numComponents; oldIndex++) {
        const u16 id = pConstComponents[oldIndex].id;

        if(id != 0) {
          const u16 newIndex = pIdCounts[id];
          pComponentsTmp[newIndex] = pConstComponents[oldIndex];

          pIdCounts[id]++;
        }
      } // for(s32 i=0; i<numComponents; i++)

      // Copy the sorted segments back to the original
      memcpy(this->components.Pointer(0), componentsTmp.Pointer(0), totalCount*sizeof(ConnectedComponentSegment));

      this->components.set_size(totalCount);

      this->isSortedInId = true;

      return RESULT_OK;
    }

    // Iterate through components, and return the maximum id
    Result ConnectedComponents::FindMaximumId()
    {
      const s32 numComponents = components.get_size();

      this->maximumId = 0;
      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);
      for(s32 i=0; i<numComponents; i++) {
        this->maximumId = MAX(this->maximumId, pConstComponents[i].id);
      }

      return RESULT_OK;
    }

    // Iterate through components, and compute the number of pixels for each
    // componentSizes must be at least sizeof(s32)*(maximumdId+1) bytes
    // Note: this is probably inefficient, compared with interlacing the loops in a kernel
    Result ConnectedComponents::ComputeComponentSizes(FixedLengthList<s32> &componentSizes)
    {
      AnkiConditionalErrorAndReturnValue(componentSizes.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "componentSizes is not valid");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "components is not valid");

      componentSizes.SetZero();
      componentSizes.set_size(maximumId+1);

      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);
      s32 * restrict pComponentSizes = componentSizes.Pointer(0);

      const s32 numComponents = components.get_size();

      for(s32 i=0; i<numComponents; i++) {
        const u16 id = pConstComponents[i].id;
        const s16 length = pConstComponents[i].xEnd - pConstComponents[i].xStart + 1;

        pComponentSizes[id] += length;
      }

      return RESULT_OK;
    }

    // Iterate through components, and compute the centroid of each component componentCentroids
    // must be at least sizeof(Point<s16>)*(maximumdId+1) bytes
    // Note: this is probably inefficient, compared with interlacing the loops in a kernel
    // Iterate through components, and compute the centroid of each component componentCentroids
    // must be at least sizeof(Point<s16>)*(maximumdId+1) bytes
    // Note: this is probably inefficient, compared with interlacing the loops in a kernel
    //
    // For a ConnectedComponent that has a maximum id of N, this function requires
    // 12n + 12 bytes of scratch.
    Result ConnectedComponents::ComputeComponentCentroids(FixedLengthList<Point<s16> > &componentCentroids, MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(componentCentroids.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "componentCentroids is not valid");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "components is not valid");

      componentCentroids.SetZero();
      componentCentroids.set_size(maximumId+1);

      FixedLengthList<Point<s32> > componentCentroidAccumulators(maximumId+1, scratch);
      componentCentroidAccumulators.set_size(maximumId+1);
      componentCentroidAccumulators.SetZero();

      FixedLengthList<s32> componentSizes(maximumId+1, scratch);

      if((lastResult = ComputeComponentSizes(componentSizes)) != RESULT_OK)
        return lastResult;

      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);

      {
        Point<s32> * restrict pComponentCentroidAccumulators = componentCentroidAccumulators.Pointer(0);

        const s32 numComponents = components.get_size();

        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          const u16 id = pConstComponents[iComponent].id;

          const s16 y = pConstComponents[iComponent].y;

          for(s32 x=pConstComponents[iComponent].xStart; x<=pConstComponents[iComponent].xEnd; x++) {
            pComponentCentroidAccumulators[id].x += x;
            pComponentCentroidAccumulators[id].y += y;
          }
        }
      }

      {
        const Point<s32> * restrict pConstComponentCentroidAccumulators = componentCentroidAccumulators.Pointer(0);
        const s32 * restrict pConstComponentSizes = componentSizes.Pointer(0);
        Point<s16> * restrict pComponentCentroids = componentCentroids.Pointer(0);

        for(s32 i=0; i<=maximumId; i++) {
          if(pConstComponentSizes[i] > 0) {
            pComponentCentroids[i].x = static_cast<s16>(pConstComponentCentroidAccumulators[i].x / pConstComponentSizes[i]);
            pComponentCentroids[i].y = static_cast<s16>(pConstComponentCentroidAccumulators[i].y / pConstComponentSizes[i]);
          }
        }
      }

      return RESULT_OK;
    }

    // Iterate through components, and compute bounding box for each component
    // componentBoundingBoxes must be at least sizeof(Rectangle<s16>)*(maximumdId+1) bytes
    // Note: this is probably inefficient, compared with interlacing the loops in a kernel
    Result ConnectedComponents::ComputeComponentBoundingBoxes(FixedLengthList<Rectangle<s16> > &componentBoundingBoxes)
    {
      AnkiConditionalErrorAndReturnValue(componentBoundingBoxes.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "componentBoundingBoxes is not valid");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "components is not valid");

      componentBoundingBoxes.set_size(maximumId+1);

      const s32 numComponents = components.get_size();
      const s32 numComponentBoundingBoxes = componentBoundingBoxes.get_size();

      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);
      Rectangle<s16> * restrict pComponentBoundingBoxes = componentBoundingBoxes.Pointer(0);

      for(s32 i=0; i<numComponentBoundingBoxes; i++) {
        pComponentBoundingBoxes[i].left = s16_MAX;
        pComponentBoundingBoxes[i].right = s16_MIN;
        pComponentBoundingBoxes[i].top = s16_MAX;
        pComponentBoundingBoxes[i].bottom = s16_MIN;
      }

      for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
        const u16 id = pConstComponents[iComponent].id;
        const s16 xStart = pConstComponents[iComponent].xStart;
        const s16 xEnd = pConstComponents[iComponent].xEnd;
        const s16 y = pConstComponents[iComponent].y;

        pComponentBoundingBoxes[id].left = MIN(pComponentBoundingBoxes[id].left, xStart);
        pComponentBoundingBoxes[id].right = MAX(pComponentBoundingBoxes[id].right, xEnd);
        pComponentBoundingBoxes[id].top = MIN(pComponentBoundingBoxes[id].top, y);
        pComponentBoundingBoxes[id].bottom = MAX(pComponentBoundingBoxes[id].bottom, y);
      }

      return RESULT_OK;
    }

    // Iterate through components, and compute the number of componentSegments that have each id
    // componentSizes must be at least sizeof(s32)*(maximumdId+1) bytes
    // Note: this is probably inefficient, compared with interlacing the loops in a kernel
    Result ConnectedComponents::ComputeNumComponentSegmentsForEachId(FixedLengthList<s32> &numComponentSegments)
    {
      AnkiConditionalErrorAndReturnValue(numComponentSegments.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "numComponentSegments is not valid");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "components is not valid");

      numComponentSegments.SetZero();
      numComponentSegments.set_size(maximumId+1);

      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);
      s32 * restrict pNumComponentSegments = numComponentSegments.Pointer(0);

      const s32 numComponents = components.get_size();

      for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
        const u16 id = pConstComponents[iComponent].id;

        pNumComponentSegments[id]++;
      }

      return RESULT_OK;
    }

    // The list of components may have unused ids. This function compresses the set of ids, so that
    // max(ids) == numberOfUniqueValues(ids). The function then returns the maximum id. For example, the list of ids {0,4,5,7} would be
    // changed to {0,1,2,3}, and it would return 3.
    //
    // For a ConnectedComponent that has a maximum id of N, this function requires
    // 3n + 3 bytes of scratch.
    //
    // TODO: If scratch usage is a bigger issue than computation time, this could be done with a bitmask
    Result ConnectedComponents::CompressConnectedComponentSegmentIds(MemoryStack scratch)
    {
      s32 numUsedIds = 0;

      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);

      // Compute the number of unique components
      u8 *usedIds = reinterpret_cast<u8*>(scratch.Allocate(maximumId+1));
      u16 *idLookupTable = reinterpret_cast<u16*>(scratch.Allocate( sizeof(u16)*(maximumId+1) ));

      AnkiConditionalErrorAndReturnValue(usedIds, RESULT_FAIL_OUT_OF_MEMORY, "CompressConnectedComponentSegmentIds", "Couldn't allocate usedIds");
      AnkiConditionalErrorAndReturnValue(idLookupTable, RESULT_FAIL_OUT_OF_MEMORY, "CompressConnectedComponentSegmentIds", "Couldn't allocate idLookupTable");

      memset(usedIds, 0, sizeof(usedIds[0])*(maximumId+1));

      const s32 numComponents = components.get_size();

      for(s32 i=0; i<numComponents; i++) {
        const u16 id = pConstComponents[i].id;
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

      ConnectedComponentSegment * restrict pComponents = components.Pointer(0);
      // Convert the id numbers to the compressed id numbers
      for(s32 i=0; i<numComponents; i++) {
        pComponents[i].id = idLookupTable[pComponents[i].id];
      }

      FindMaximumId(); // maximumId should be equal to (currentCompressedId - 1)

      // TODO: think is this can modify the this->isSortedInId attribute. I think it can't

      return RESULT_OK;
    } // Result CompressConnectedComponentSegmentIds(FixedLengthList<ConnectedComponentSegment> &components, MemoryStack scratch)

    // Goes through the list components, and computes the number of pixels for each.
    // For any componentId with less than minimumNumPixels pixels, all ConnectedComponentSegment with that id will have their ids set to zero
    //
    // For a ConnectedComponent that has a maximum id of N, this function requires
    // 4n + 4 bytes of scratch.
    Result ConnectedComponents::InvalidateSmallOrLargeComponents(const s32 minimumNumPixels, const s32 maximumNumPixels, MemoryStack scratch)
    {
      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);

      s32 *componentSizes = reinterpret_cast<s32*>(scratch.Allocate( sizeof(s32)*(maximumId+1) ));

      const s32 numComponents = components.get_size();

      AnkiConditionalErrorAndReturnValue(componentSizes, RESULT_FAIL_OUT_OF_MEMORY, "InvalidateSmallOrLargeComponents", "Couldn't allocate componentSizes");

      memset(componentSizes, 0, sizeof(componentSizes[0])*(maximumId+1));

      for(s32 i=0; i<numComponents; i++) {
        const u16 id = pConstComponents[i].id;
        const s16 length = pConstComponents[i].xEnd - pConstComponents[i].xStart + 1;

        componentSizes[id] += length;
      }

      // TODO: mark the components as invalid, instead of doing all the checks every time

      ConnectedComponentSegment * restrict pComponents = components.Pointer(0);
      for(s32 i=0; i<numComponents; i++) {
        const u16 id = pComponents[i].id;

        if(componentSizes[id] < minimumNumPixels || componentSizes[id] > maximumNumPixels) {
          pComponents[i].id = 0;
        }
      }

      FindMaximumId();

      this->isSortedInId = false;

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
    // For a ConnectedComponent that has a maximum id of N, this function requires
    // 8N + 8 bytes of scratch.
    Result ConnectedComponents::InvalidateSolidOrSparseComponents(const s32 sparseMultiplyThreshold, const s32 solidMultiplyThreshold, MemoryStack scratch)
    {
      const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);

      const s32 numComponents = components.get_size();

      // Storage for the bounding boxes.
      // A bounding box is (minX, minY) -> (maxX, maxY)
      s16 *minX = reinterpret_cast<s16*>(scratch.Allocate( sizeof(s16)*(maximumId+1) ));
      s16 *maxX = reinterpret_cast<s16*>(scratch.Allocate( sizeof(s16)*(maximumId+1) ));
      s16 *minY = reinterpret_cast<s16*>(scratch.Allocate( sizeof(s16)*(maximumId+1) ));
      s16 *maxY = reinterpret_cast<s16*>(scratch.Allocate( sizeof(s16)*(maximumId+1) ));
      s32 *componentSizes = reinterpret_cast<s32*>(scratch.Allocate( sizeof(s32)*(maximumId+1) ));

      AnkiConditionalErrorAndReturnValue(minX && maxX && minY && maxY && componentSizes,
        RESULT_FAIL_OUT_OF_MEMORY, "InvalidateSolidOrSparseComponents", "Couldn't allocate minX, maxX, minY, maxY, or componentSizes");

      memset(componentSizes, 0, sizeof(componentSizes[0])*(maximumId+1));

      for(s32 i=0; i<=maximumId; i++) {
        minX[i] = s16_MAX;
        maxX[i] = s16_MIN;
        minY[i] = s16_MAX;
        maxY[i] = s16_MIN;
      }

      // Compute the bounding box of each component ID
      // A bounding box is (minX, minY) -> (maxX, maxY)
      for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
        const u16 id = pConstComponents[iComponent].id;
        const s16 y = pConstComponents[iComponent].y;
        const s16 xStart = pConstComponents[iComponent].xStart;
        const s16 xEnd = pConstComponents[iComponent].xEnd;

        const s16 length = pConstComponents[iComponent].xEnd - pConstComponents[iComponent].xStart + 1;

        componentSizes[id] += length;

        minX[id] = MIN(minX[id], xStart);
        maxX[id] = MAX(maxX[id], xEnd);

        minY[id] = MIN(minY[id], y);
        maxY[id] = MAX(maxY[id], y);
      }

      for(s32 id=0; id<=maximumId; id++) {
        // The SQ26.5 parameter sparseMultiplyThreshold is set so that a component is invalid if
        // "sparseMultiplyThreshold*numPixels < boundingWidth*boundingHeight".
        // A resonable value is between 5<<5 = 160 and 100<<5 = 3200.
        //
        // The SQ26.5 parameter solidMultiplyThreshold is set so that a component is invalid if
        // "solidMultiplyThreshold*numPixels > boundingWidth*boundingHeight".
        // A resonable value is between 1.5*pow(2,5) = 48 and 5<<5 = 160.

        const s32 boundingArea = ((maxX[id]-minX[id]+1)*(maxY[id]-minY[id]+1)) << 5; // SQ26.5

        const s32 sparseMultiply = sparseMultiplyThreshold*componentSizes[id]; // (SQ26.5 * SQ31.0) -> SQ26.5
        const s32 solidMultiply = solidMultiplyThreshold*componentSizes[id]; // (SQ26.5 * SQ31.0) -> SQ26.5

        if(sparseMultiply < boundingArea || solidMultiply > boundingArea) {
          minX[id] = s16_MIN; // Set to invalid
        }
      }

      ConnectedComponentSegment * restrict pComponents = components.Pointer(0);
      for(s32 i=0; i<numComponents; i++) {
        const u16 id = pComponents[i].id;

        if(minX[id] < 0) {
          pComponents[i].id = 0;
        }
      }

      FindMaximumId();

      this->isSortedInId = false;

      return RESULT_OK;
    }

    // If a component doesn't have a hollow center, it's not a fiducial. Based on a component's
    // centroid, and its maximum extent, this method makes sure no componentSegment is inside of
    // an inner rectangle. For example, take a component centered at (50,50), that is 20 pixels
    // wide and high. If percentHorizontal=0.5 and percentVertical=0.25, then no componentSegment
    // should intersect the rectangle between (40,45) and (60,55).
    //
    // percentHorizontal and percentVertical are SQ23.8,
    // and should range from (0.0, 1.0), non-inclusive
    //
    // For a ConnectedComponent that has a maximum id of N, this function requires 10N + 10 bytes
    // of scratch.
    Result ConnectedComponents::InvalidateFilledCenterComponents(const s32 percentHorizontal, const s32 percentVertical, MemoryStack scratch)
    {
      const s32 numFractionalBitsForPercents = 8;

      FixedLengthList<Rectangle<s16> > componentBoundingBoxes(maximumId+1, scratch);

      if((lastResult = ComputeComponentBoundingBoxes(componentBoundingBoxes)) != RESULT_OK)
        return lastResult;

      // Reduce the size of the bounding box, based on percentHorizontal and percentVertical.
      // This reduced-size box is the area that must be clear
      {
        Rectangle<s16> * restrict pComponentBoundingBoxes = componentBoundingBoxes.Pointer(0);
        for(s32 iComponent=0; iComponent<=maximumId; iComponent++) {
          // TODO: these appear to be unused.  Remove?
          /*
          const s16 left = pComponentBoundingBoxes[iComponent].left;
          const s16 right = pComponentBoundingBoxes[iComponent].right;
          const s16 top = pComponentBoundingBoxes[iComponent].top;
          const s16 bottom = pComponentBoundingBoxes[iComponent].bottom;
          */
          const s16 boxWidth = pComponentBoundingBoxes[iComponent].get_width();
          const s16 boxHeight = pComponentBoundingBoxes[iComponent].get_height();

          const s16 scaledHalfWidth = static_cast<s16>((boxWidth * percentHorizontal) >> (numFractionalBitsForPercents+1)); // (SQ16.0 * SQ23.8) -> SQ 31.0, and divided by two
          const s16 scaledHalfHeight = static_cast<s16>((boxHeight * percentVertical) >> (numFractionalBitsForPercents+1)); // (SQ16.0 * SQ23.8) -> SQ 31.0, and divided by two

          pComponentBoundingBoxes[iComponent].left += scaledHalfWidth;
          pComponentBoundingBoxes[iComponent].right -= scaledHalfWidth;
          pComponentBoundingBoxes[iComponent].top += scaledHalfHeight;
          pComponentBoundingBoxes[iComponent].bottom -= scaledHalfHeight;
        }
      }

      FixedLengthList<bool> validComponents(maximumId+1, scratch);
      validComponents.set_size(maximumId+1);
      validComponents.Set(true);

      // If any ComponentSegment is within the shrunk bounding box, that componentId is invalid
      {
        const ConnectedComponentSegment * restrict pConstComponents = components.Pointer(0);
        const Rectangle<s16> * restrict pConstComponentBoundingBoxes = componentBoundingBoxes.Pointer(0);

        bool * restrict pValidComponents = validComponents.Pointer(0);

        const s32 numComponents = components.get_size();

        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          const u16 id = pConstComponents[iComponent].id;

          if(id > 0) {
            const s16 y = pConstComponents[iComponent].y;
            const s16 xStart = pConstComponents[iComponent].xStart;
            const s16 xEnd = pConstComponents[iComponent].xEnd;

            const s16 left = pConstComponentBoundingBoxes[id].left;
            const s16 right = pConstComponentBoundingBoxes[id].right;
            const s16 top = pConstComponentBoundingBoxes[id].top;
            const s16 bottom = pConstComponentBoundingBoxes[id].bottom;

            if(y > top && y < bottom && xEnd > left && xStart < right) {
              pValidComponents[id] = false;
            }
          } // if(id > 0)
        }
      }

      // Set all invalid ComponentSegments to zero
      {
        ConnectedComponentSegment * restrict pComponents = components.Pointer(0);

        const bool * restrict pValidComponents = validComponents.Pointer(0);

        const s32 numComponents = components.get_size();

        for(s32 i=0; i<numComponents; i++) {
          const u16 id = pComponents[i].id;

          if(!pValidComponents[id]) {
            pComponents[i].id = 0;
          }
        }
      }

      FindMaximumId();

      this->isSortedInId = false;

      return RESULT_OK;
    }

    Result ConnectedComponents::PushBack(const ConnectedComponentSegment &value)
    {
      const Result result = components.PushBack(value);

      if(result != RESULT_OK)
        return result;

      this->maximumId = MAX(this->maximumId, value.id);
      this->isSortedInId = false;
      this->isSortedInY = false;
      this->isSortedInX = false;
      return RESULT_OK;
    }

    bool ConnectedComponents::IsValid() const
    {
      // TODO: should we do any other checks here?
      return components.IsValid();
    }

    Result ConnectedComponents::Print() const
    {
      return this->components.Print();
    }

    u16 ConnectedComponents::get_maximumId() const
    {
      return maximumId;
    }

    s32 ConnectedComponents::get_size() const
    {
      return components.get_size();
    }

    bool ConnectedComponents::get_isSortedInId() const
    {
      return isSortedInId;
    }

    bool ConnectedComponents::get_isSortedInY() const
    {
      return isSortedInY;
    }

    bool ConnectedComponents::get_isSortedInX() const
    {
      return isSortedInX;
    }
  } // namespace Embedded
} // namespace Anki
