/**
File: connectedComponents.h
Author: Peter Barnum
Created: 2013

Definitions of connectedComponents_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_

#include "coretech/vision/robot/connectedComponents_declarations.h"

#include "coretech/common/shared/types.h"
#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/fixedLengthList.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark --- Definitions ---

    template<typename Type> ConnectedComponentSegment<Type>::ConnectedComponentSegment()
      : xStart(-1), xEnd(-1), y(-1), id(0)
    {
    } // ConnectedComponentSegment<Type>::ConnectedComponentSegment<Type>()

    template<typename Type> ConnectedComponentSegment<Type>::ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y, const Type id)
      : xStart(xStart), xEnd(xEnd), y(y), id(id)
    {
    } // ConnectedComponentSegment<Type>::ConnectedComponentSegment<Type>(const s16 xStart, const s16 xEnd, const s16 y, const Type id)

    template<typename Type> void ConnectedComponentSegment<Type>::Print() const
    {
      CoreTechPrint("[%d: (%d->%d, %d)] ", static_cast<s32>(this->id), this->xStart, this->xEnd, this->y);
    } // void ConnectedComponentSegment<Type>::Print() const

    template<typename Type> bool ConnectedComponentSegment<Type>::operator== (const ConnectedComponentSegment<Type> &component2) const
    {
      if((this->xStart == component2.xStart) &&
        (this->xEnd == component2.xEnd) &&
        (this->y == component2.y) &&
        (this->id == component2.id)) {
          return true;
      }

      return false;
    }

    template<typename Type> inline s64 ConnectedComponentSegment<Type>::Compare(const ConnectedComponentSegment<Type> &a, const ConnectedComponentSegment<Type> &b)
    {
      // Wraps zero around to the max value
      Type idA = a.id;
      Type idB = b.id;

      if(idA <= 0) {
        idA = saturate_cast<Type>(1e100);
      } else {
        idA--;
      }

      if(idB <= 0) {
        idB = saturate_cast<Type>(1e100);
      } else {
        idB--;
      }

      const s64 idDifference = (static_cast<s64>(std::numeric_limits<s16>::max())+1) * (static_cast<s64>(std::numeric_limits<s16>::max())+1) * (static_cast<s64>(idA) - static_cast<s64>(idB));
      const s64 yDifference = (static_cast<s64>(std::numeric_limits<s16>::max())+1) * (static_cast<s64>(a.y) - static_cast<s64>(b.y));
      const s64 xStartDifference = (static_cast<s64>(a.xStart) - static_cast<s64>(b.xStart));

      return idDifference + yDifference + xStartDifference;
    }

    template<typename Type> inline const ConnectedComponentSegment<Type>* ConnectedComponentsTemplate<Type>::Pointer(const s32 index) const
    {
      return components.Pointer(index);
    }

    template<typename Type> inline const ConnectedComponentSegment<Type>& ConnectedComponentsTemplate<Type>::operator[](const s32 index) const
    {
      return *components.Pointer(index);
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::Extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList<ConnectedComponentSegment<Type> > &components)
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
                //                CoreTechPrint("one: %d %d\n", componentStart, x-numSkipped);
                components.PushBack(ConnectedComponentSegment<Type>(componentStart, x-numSkipped));
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
          //          CoreTechPrint("two: %d %d\n", componentStart, binaryImageWidth-numSkipped-1);
          components.PushBack(ConnectedComponentSegment<Type>(componentStart, binaryImageWidth-numSkipped-1));
        }
      }

      return RESULT_OK;
    } // Result Extract1dComponents(const u8 * restrict binaryImageRow, const s32 binaryImageWidth, const s32 minComponentWidth, FixedLengthList<Point<s16> > &components)

    template<typename Type> ConnectedComponentsTemplate<Type>::ConnectedComponentsTemplate()
      : curState(STATE_INVALID), isSortedInId(true), isSortedInY(true), isSortedInX(true), maximumId(0), maxImageWidth(-1)
    {
    }

    template<typename Type> ConnectedComponentsTemplate<Type>::ConnectedComponentsTemplate(const Type maxComponentSegments, const s16 maxImageWidth, MemoryStack &memory)
      : curState(STATE_INVALID), isSortedInId(true), isSortedInY(true), isSortedInX(true), maximumId(0), maxImageWidth(maxImageWidth), maxComponentSegments(maxComponentSegments)
    {
      //AnkiConditionalErrorAndReturn(maxComponentSegments > 0 && maxComponentSegments <= Type_MAX,
      //  "ConnectedComponentsTemplate<Type>::ConnectedComponents", "maxComponentSegments must be greater than zero and less than 0xFFFF");

      AnkiConditionalErrorAndReturn(maxImageWidth > 0 && maxImageWidth <= std::numeric_limits<s16>::max(),
        "ConnectedComponentsTemplate<Type>::ConnectedComponents", "maxImageWidth must be less than 0x7FFF");

      this->components = FixedLengthList<ConnectedComponentSegment<Type> >(maxComponentSegments, memory);

      this->previousComponents1d = FixedLengthList<ConnectedComponentSegment<Type> >();
      this->currentComponents1d = FixedLengthList<ConnectedComponentSegment<Type> >();
      this->newPreviousComponents1d = FixedLengthList<ConnectedComponentSegment<Type> >();
      this->equivalentComponents = FixedLengthList<Type>();

      this->curState = STATE_CONSTRUCTED;
    } // ConnectedComponents(const s32 maxComponentSegments, MemoryStack &memory)

    template<typename Type> Result ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_Initialize(MemoryStack &fastMemory, MemoryStack &slowerMemory, MemoryStack &slowestMemory)
    {
      const Type MAX_2D_COMPONENTS = static_cast<Type>(components.get_maximumSize());

      AnkiConditionalErrorAndReturnValue(this->curState == STATE_CONSTRUCTED,
        RESULT_FAIL, "ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_NextRow", "Object is not constructed (or was initialized earlier)");

      components.Clear();

      this->maximumId = 0;

      this->previousComponents1d = FixedLengthList<ConnectedComponentSegment<Type> >(maxImageWidth, fastMemory);
      this->currentComponents1d = FixedLengthList<ConnectedComponentSegment<Type> >(maxImageWidth, fastMemory);
      this->newPreviousComponents1d = FixedLengthList<ConnectedComponentSegment<Type> >(maxImageWidth, fastMemory);

      const s32 bytesRequired = 256 + MEMORY_ALIGNMENT + maxComponentSegments * sizeof(Type);
      if(slowerMemory.ComputeLargestPossibleAllocation() >= bytesRequired) {
        this->equivalentComponents = FixedLengthList<Type>(maxComponentSegments, slowerMemory);
      } else {
        this->equivalentComponents = FixedLengthList<Type>(maxComponentSegments, slowestMemory);
      }

      Type * restrict pEquivalentComponents = equivalentComponents.Pointer(0);
      for(s32 i=0; i<MAX_2D_COMPONENTS; i++) {
        pEquivalentComponents[i] = static_cast<Type>(i);
      }

      this->curState = STATE_INITIALIZED;

      return RESULT_OK;
    } // template<typename Type> Result ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_Initialize()

    template<typename Type> Result ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_NextRow(const u8 * restrict binaryImageRow, const s32 imageWidth, const s16 whichRow, const s16 minComponentWidth, const s16 maxSkipDistance)
    {
      const Type TYPE_MAX = saturate_cast<Type>(1e100);

      AnkiAssert(imageWidth <= maxImageWidth);

      AnkiConditionalErrorAndReturnValue(this->curState == STATE_INITIALIZED,
        RESULT_FAIL, "ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_NextRow", "Object is not initialized");

      const ConnectedComponentSegment<Type> * restrict pCurrentComponents1d = currentComponents1d.Pointer(0);
      const ConnectedComponentSegment<Type> * restrict pPreviousComponents1d = previousComponents1d.Pointer(0);
      ConnectedComponentSegment<Type> * restrict pNewPreviousComponents1d = newPreviousComponents1d.Pointer(0);

      Type * restrict pEquivalentComponents = equivalentComponents.Pointer(0);

      //BeginBenchmark("e2dc_pr_nextRow_extract");
      ConnectedComponentsTemplate<Type>::Extract1dComponents(binaryImageRow, static_cast<s16>(imageWidth), minComponentWidth, maxSkipDistance, currentComponents1d);
      //EndBenchmark("e2dc_pr_nextRow_extract");

      const s32 numCurrentComponents1d = currentComponents1d.get_size();
      const s32 numPreviousComponents1d = previousComponents1d.get_size();

      newPreviousComponents1d.set_size(numCurrentComponents1d);

      //BeginBenchmark("e2dc_pr_nextRow_mainLoop");

      for(s32 iCurrent=0; iCurrent<numCurrentComponents1d; iCurrent++) {
        bool foundMatch = false;
        Type firstMatchedPreviousId = TYPE_MAX;

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

                const ConnectedComponentSegment<Type> newComponent(pCurrentComponents1d[iCurrent].xStart, pCurrentComponents1d[iCurrent].xEnd, whichRow, firstMatchedPreviousId);

                const Result result = components.PushBack(newComponent);

                AnkiConditionalErrorAndReturnValue(result == RESULT_OK, result, "extract2dComponents", "Extracted maximum number of 2d components");

                pNewPreviousComponents1d[iCurrent] = newComponent;
              } else { // if(!foundMatch)
                // Update the lookup table for all of:
                // 1. The first matching component
                // 2. This iPrevious component.
                // 3. The newPreviousComponents1d(iCurrent) component
                // TODO: Think if this can be changed to make the final equivalence matching faster

                const Type previousId = pPreviousComponents1d[iPrevious].id;
                const Type minId = MIN(MIN(MIN(firstMatchedPreviousId, previousId), pEquivalentComponents[previousId]), pEquivalentComponents[firstMatchedPreviousId]);

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

          const ConnectedComponentSegment<Type> newComponent(pCurrentComponents1d[iCurrent].xStart, pCurrentComponents1d[iCurrent].xEnd, whichRow, firstMatchedPreviousId);

          pNewPreviousComponents1d[iCurrent] = newComponent;

          const Result result = components.PushBack(newComponent);
          #pragma unused(result)

          AnkiConditionalWarnAndReturnValue(result == RESULT_OK, result, "extract2dComponents", "Extracted maximum number of 2d components");
        } // if(!foundMatch)
      } // for(s32 iCurrent=0; iCurrent<numCurrentComponents1d; iCurrent++)

      //EndBenchmark("e2dc_pr_nextRow_mainLoop");

      //BeginBenchmark("e2dc_pr_nextRow_finalize");

      // Update previousComponents1d to be newPreviousComponents1d
      Swap(previousComponents1d, newPreviousComponents1d);

      //EndBenchmark("e2dc_pr_nextRow_finalize");

      return RESULT_OK;
    } // template<typename Type> Result ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_NextRow(const Array<u8> &binaryImageRow, const s16 minComponentWidth, const s16 maxSkipDistance)

    template<typename Type> Result ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_Finalize()
    {
      const s32 MAX_EQUIVALENT_ITERATIONS = 3;

      AnkiConditionalErrorAndReturnValue(this->curState == STATE_INITIALIZED,
        RESULT_FAIL, "ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_NextRow", "Object is not initialized");

      Type * restrict pEquivalentComponents = equivalentComponents.Pointer(0);

      const s32 numComponents = components.get_size();

      // After all the initial 2d labels have been created, go through
      // equivalentComponents, and update equivalentComponents internally
      for(s32 iEquivalent=0; iEquivalent<MAX_EQUIVALENT_ITERATIONS; iEquivalent++) {
        const s32 MAX_RECURSION_LEVEL = 1000; // If it gets anywhere near this high, there's probably a bug
        s32 numChanges = 0;

        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          // Trace back along the equivalentComponents list, to find the joined component with the minimum id
          Type minNeighbor = pEquivalentComponents[iComponent];
          if(pEquivalentComponents[minNeighbor] != minNeighbor) {
            s32 recursionLevel;
            for(recursionLevel=0; recursionLevel<MAX_RECURSION_LEVEL; recursionLevel++) {
              if(pEquivalentComponents[minNeighbor] == minNeighbor)
                break;

              minNeighbor = pEquivalentComponents[minNeighbor]; // "Recurse" to the next lower component in the list
            }

            AnkiConditionalWarnAndReturnValue(recursionLevel < MAX_RECURSION_LEVEL, RESULT_FAIL, "extract2dComponents", "Issue with equivalentComponents minimum search (%d)", recursionLevel);

            numChanges++;

            pEquivalentComponents[iComponent] = minNeighbor;
          } // if(pEquivalentComponents[minNeighbor] != minNeighbor)
        } //  for(s32 iComponent=0; iComponent<numComponents; iComponent++)

        if(numChanges == 0) {
          break;
        }
      } // for(s32 iEquivalent=0; iEquivalent<MAX_EQUIVALENT_ITERATIONS; iEquivalent++)

      // Replace the id of all 1d components with their minimum equivalent id
      {
        ConnectedComponentSegment<Type> * restrict pComponents = components.Pointer(0);
        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          pComponents[iComponent].id = pEquivalentComponents[pComponents[iComponent].id];
        }
      }

      FindMaximumId();

      this->curState = STATE_FINALIZED;

      // TODO: put this back if it is needed by some other algorithm
      // Sort all 1D components by id, y, then x
      // const Result result = SortConnectedComponentSegmentsById();
      // const Result result = SortConnectedComponentSegments();
      // return result;

      return RESULT_OK;
    } // template<typename Type> Result ConnectedComponentsTemplate<Type>::Extract2dComponents_PerRow_Finalize()

    template<typename Type> Result ConnectedComponentsTemplate<Type>::Extract2dComponents_FullImage(const Array<u8> &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, MemoryStack scratch)
    {
      Result lastResult;

      const s32 imageHeight = binaryImage.get_size(0);
      const s32 imageWidth = binaryImage.get_size(1);

      if((lastResult = Extract2dComponents_PerRow_Initialize(scratch, scratch, scratch)) != RESULT_OK)
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

    template<typename Type> Result ConnectedComponentsTemplate<Type>::SortConnectedComponentSegments()
    {
      // Performs insersion sort
      // TODO: do bucket sort by id first, then run this

      ConnectedComponentSegment<Type> * restrict pComponents = components.Pointer(0);

      const s32 numComponents = components.get_size();

      for(s32 iComponent=1; iComponent<numComponents; iComponent++) {
        const ConnectedComponentSegment<Type> segmentToInsert = pComponents[iComponent];

        s32 holePosition = iComponent;

        while(holePosition > 0 && ConnectedComponentSegment<Type>::Compare(segmentToInsert, pComponents[holePosition-1]) < 0) {
          pComponents[holePosition] = pComponents[holePosition-1];
          holePosition--;
        }

        pComponents[holePosition] = segmentToInsert;
      } // for(s32 i=0; i<components.size(); i++)

      this->isSortedInId = true;
      this->isSortedInY = true;
      this->isSortedInX = true;

      return RESULT_OK;
    } // Result SortConnectedComponentSegments(FixedLengthList<ConnectedComponentSegment<Type>> &components)

    template<typename Type> Result ConnectedComponentsTemplate<Type>::SortConnectedComponentSegmentsById(MemoryStack scratch)
    {
      // Performs bucket sort
      FixedLengthList<Type> idCounts(this->maximumId + 1, scratch);

      idCounts.SetZero();

      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);
      Type * restrict pIdCounts = idCounts.Pointer(0);

      // Count the number of ComponenentSegments that have each id
      s32 numValidComponentSegments = 0;
      s32 numComponents = this->components.get_size();
      for(s32 i=0; i<numComponents; i++) {
        const Type id = pConstComponents[i].id;

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

      // NOTE: this could use a lot of space numValidComponentSegments*sizeof(ConnectedComponentSegment<Type>) = numValidComponentSegments*8
      FixedLengthList<ConnectedComponentSegment<Type> > componentsTmp(numValidComponentSegments, scratch);

      // Could fail if we don't have enough scratch space
      if(!componentsTmp.IsValid())
        return RESULT_FAIL_OUT_OF_MEMORY;

      // Convert the absolute count to a cumulative count (ignoring id == zero)
      Type totalCount = 0;
      for(s32 id=1; id<=this->maximumId; id++) {
        const Type lastCount = pIdCounts[id];

        pIdCounts[id] = totalCount;

        totalCount += lastCount;
      }

      // Go through the list, and copy each ComponentSegment into the correct "bucket"
      ConnectedComponentSegment<Type> * restrict pComponentsTmp = componentsTmp.Pointer(0);
      for(s32 oldIndex=0; oldIndex<numComponents; oldIndex++) {
        const Type id = pConstComponents[oldIndex].id;

        if(id != 0) {
          const Type newIndex = pIdCounts[id];
          pComponentsTmp[newIndex] = pConstComponents[oldIndex];

          pIdCounts[id]++;
        }
      } // for(s32 i=0; i<numComponents; i++)

      // Copy the sorted segments back to the original
      memcpy(this->components.Pointer(0), componentsTmp.Pointer(0), totalCount*sizeof(ConnectedComponentSegment<Type>));

      this->components.set_size(totalCount);

      this->isSortedInId = true;

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::FindMaximumId()
    {
      const s32 numComponents = components.get_size();

      this->maximumId = 0;
      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);
      for(s32 i=0; i<numComponents; i++) {
        this->maximumId = MAX(this->maximumId, pConstComponents[i].id);
      }

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::ComputeComponentSizes(FixedLengthList<s32> &componentSizes)
    {
      AnkiConditionalErrorAndReturnValue(componentSizes.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "componentSizes is not valid");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "components is not valid");

      componentSizes.SetZero();
      componentSizes.set_size(maximumId+1);

      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);
      s32 * restrict pComponentSizes = componentSizes.Pointer(0);

      const s32 numComponents = components.get_size();

      for(s32 i=0; i<numComponents; i++) {
        const Type id = pConstComponents[i].id;
        const s16 length = pConstComponents[i].xEnd - pConstComponents[i].xStart + 1;

        pComponentSizes[id] += length;
      }

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::ComputeComponentCentroids(FixedLengthList<Point<s16> > &componentCentroids, MemoryStack scratch)
    {
      Result lastResult;

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

      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);

      {
        Point<s32> * restrict pComponentCentroidAccumulators = componentCentroidAccumulators.Pointer(0);

        const s32 numComponents = components.get_size();

        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          const Type id = pConstComponents[iComponent].id;

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

    template<typename Type> Result ConnectedComponentsTemplate<Type>::ComputeComponentBoundingBoxes(FixedLengthList<Rectangle<s16> > &componentBoundingBoxes)
    {
      AnkiConditionalErrorAndReturnValue(componentBoundingBoxes.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "componentBoundingBoxes is not valid");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "components is not valid");

      componentBoundingBoxes.set_size(maximumId+1);

      const s32 numComponents = components.get_size();
      const s32 numComponentBoundingBoxes = componentBoundingBoxes.get_size();

      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);
      Rectangle<s16> * restrict pComponentBoundingBoxes = componentBoundingBoxes.Pointer(0);

      for(s32 i=0; i<numComponentBoundingBoxes; i++) {
        pComponentBoundingBoxes[i].left = std::numeric_limits<s16>::max();
        pComponentBoundingBoxes[i].right = std::numeric_limits<s16>::min();
        pComponentBoundingBoxes[i].top = std::numeric_limits<s16>::max();
        pComponentBoundingBoxes[i].bottom = std::numeric_limits<s16>::min();
      }

      for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
        const Type id = pConstComponents[iComponent].id;
        const s16 xStart = pConstComponents[iComponent].xStart;
        const s16 xEnd = pConstComponents[iComponent].xEnd;
        const s16 y = pConstComponents[iComponent].y;

        pComponentBoundingBoxes[id].left = MIN(pComponentBoundingBoxes[id].left, xStart);
        pComponentBoundingBoxes[id].right = MAX(pComponentBoundingBoxes[id].right, xEnd+1);
        pComponentBoundingBoxes[id].top = MIN(pComponentBoundingBoxes[id].top, y);
        pComponentBoundingBoxes[id].bottom = MAX(pComponentBoundingBoxes[id].bottom, y+1);
      }

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::ComputeNumComponentSegmentsForEachId(FixedLengthList<s32> &numComponentSegments)
    {
      AnkiConditionalErrorAndReturnValue(numComponentSegments.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "numComponentSegments is not valid");
      AnkiConditionalErrorAndReturnValue(components.IsValid(), RESULT_FAIL_INVALID_OBJECT, "ComputeComponentSizes", "components is not valid");

      numComponentSegments.SetZero();
      numComponentSegments.set_size(maximumId+1);

      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);
      s32 * restrict pNumComponentSegments = numComponentSegments.Pointer(0);

      const s32 numComponents = components.get_size();

      for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
        const Type id = pConstComponents[iComponent].id;

        pNumComponentSegments[id]++;
      }

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::CompressConnectedComponentSegmentIds(MemoryStack scratch)
    {
      s32 numUsedIds = 0;

      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);

      // Compute the number of unique components
      u8 *usedIds = reinterpret_cast<u8*>(scratch.Allocate(maximumId+1));
      Type *idLookupTable = reinterpret_cast<Type*>(scratch.Allocate( sizeof(Type)*(maximumId+1) ));

      AnkiConditionalErrorAndReturnValue(usedIds, RESULT_FAIL_OUT_OF_MEMORY, "CompressConnectedComponentSegmentIds", "Couldn't allocate usedIds");
      AnkiConditionalErrorAndReturnValue(idLookupTable, RESULT_FAIL_OUT_OF_MEMORY, "CompressConnectedComponentSegmentIds", "Couldn't allocate idLookupTable");

      memset(usedIds, 0, sizeof(usedIds[0])*(maximumId+1));

      const s32 numComponents = components.get_size();

      for(s32 i=0; i<numComponents; i++) {
        const Type id = pConstComponents[i].id;
        usedIds[id] = 1;
      }

      for(s32 i=0; i<=maximumId; i++) {
        numUsedIds += usedIds[i];
      }

      // Create a mapping table from original id to compressed id
      idLookupTable[0] = 0;
      Type currentCompressedId = 1;
      for(s32 i=1; i<=maximumId; i++) {
        if(usedIds[i] != 0) {
          idLookupTable[i] = currentCompressedId++;
        }
      }

      ConnectedComponentSegment<Type> * restrict pComponents = components.Pointer(0);
      // Convert the id numbers to the compressed id numbers
      for(s32 i=0; i<numComponents; i++) {
        pComponents[i].id = idLookupTable[pComponents[i].id];
      }

      FindMaximumId(); // maximumId should be equal to (currentCompressedId - 1)

      // TODO: Think if this can modify the this->isSortedInId attribute. I think it can't

      return RESULT_OK;
    } // Result CompressConnectedComponentSegmentIds(FixedLengthList<ConnectedComponentSegment<Type>> &components, MemoryStack scratch)

    template<typename Type> Result ConnectedComponentsTemplate<Type>::InvalidateSmallOrLargeComponents(const s32 minimumNumPixels, const s32 maximumNumPixels, MemoryStack scratch)
    {
      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);

      s32 *componentSizes = reinterpret_cast<s32*>(scratch.Allocate( sizeof(s32)*(maximumId+1) ));

      const s32 numComponents = components.get_size();

      AnkiConditionalErrorAndReturnValue(componentSizes, RESULT_FAIL_OUT_OF_MEMORY, "InvalidateSmallOrLargeComponents", "Couldn't allocate componentSizes");

      memset(componentSizes, 0, sizeof(componentSizes[0])*(maximumId+1));

      for(s32 i=0; i<numComponents; i++) {
        const Type id = pConstComponents[i].id;
        const s16 length = pConstComponents[i].xEnd - pConstComponents[i].xStart + 1;

        componentSizes[id] += length;
      }

      // TODO: mark the components as invalid, instead of doing all the checks every time

      ConnectedComponentSegment<Type> * restrict pComponents = components.Pointer(0);
      for(s32 i=0; i<numComponents; i++) {
        const Type id = pComponents[i].id;

        if(componentSizes[id] < minimumNumPixels || componentSizes[id] > maximumNumPixels) {
          pComponents[i].id = 0;
        }
      }

      FindMaximumId();

      this->isSortedInId = false;

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::InvalidateSolidOrSparseComponents(const s32 sparseMultiplyThreshold, const s32 solidMultiplyThreshold, MemoryStack scratch)
    {
      const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);

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
        minX[i] = std::numeric_limits<s16>::max();
        maxX[i] = std::numeric_limits<s16>::min();
        minY[i] = std::numeric_limits<s16>::max();
        maxY[i] = std::numeric_limits<s16>::min();
      }

      // Compute the bounding box of each component ID
      // A bounding box is (minX, minY) -> (maxX, maxY)
      for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
        const Type id = pConstComponents[iComponent].id;
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
          minX[id] = std::numeric_limits<s16>::min(); // Set to invalid
        }
      }

      ConnectedComponentSegment<Type> * restrict pComponents = components.Pointer(0);
      for(s32 i=0; i<numComponents; i++) {
        const Type id = pComponents[i].id;

        if(minX[id] < 0) {
          pComponents[i].id = 0;
        }
      }

      FindMaximumId();

      this->isSortedInId = false;

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::InvalidateFilledCenterComponents_shrunkRectangle(const s32 percentHorizontal, const s32 percentVertical, MemoryStack scratch)
    {
      const s32 numFractionalBitsForPercents = 8;

      Result lastResult;

      FixedLengthList<Rectangle<s16> > componentBoundingBoxes(maximumId+1, scratch);

      if((lastResult = ComputeComponentBoundingBoxes(componentBoundingBoxes)) != RESULT_OK)
        return lastResult;

      // Reduce the size of the bounding box, based on percentHorizontal and percentVertical.
      // This reduced-size box is the area that must be clear
      {
        Rectangle<s16> * restrict pComponentBoundingBoxes = componentBoundingBoxes.Pointer(0);
        for(s32 iComponent=0; iComponent<=maximumId; iComponent++) {
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
        const ConnectedComponentSegment<Type> * restrict pConstComponents = components.Pointer(0);
        const Rectangle<s16> * restrict pConstComponentBoundingBoxes = componentBoundingBoxes.Pointer(0);

        bool * restrict pValidComponents = validComponents.Pointer(0);

        const s32 numComponents = components.get_size();

        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          const Type id = pConstComponents[iComponent].id;

          if(id > 0) {
            const s16 y = pConstComponents[iComponent].y;
            const s16 xStart = pConstComponents[iComponent].xStart;
            const s16 xEnd = pConstComponents[iComponent].xEnd;

            const s16 left = pConstComponentBoundingBoxes[id].left;
            const s16 right = pConstComponentBoundingBoxes[id].right;
            const s16 top = pConstComponentBoundingBoxes[id].top;
            const s16 bottom = pConstComponentBoundingBoxes[id].bottom;

            if(y > top && y < (bottom-1) && xEnd > left && xStart < (right-1)) {
              pValidComponents[id] = false;
            }
          } // if(id > 0)
        }
      }

      // Set all invalid ComponentSegments to zero
      {
        ConnectedComponentSegment<Type> * restrict pComponents = components.Pointer(0);

        const bool * restrict pValidComponents = validComponents.Pointer(0);

        const s32 numComponents = components.get_size();

        for(s32 i=0; i<numComponents; i++) {
          const Type id = pComponents[i].id;

          if(!pValidComponents[id]) {
            pComponents[i].id = 0;
          }
        }
      }

      FindMaximumId();

      this->isSortedInId = false;

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::InvalidateFilledCenterComponents_hollowRows(const f32 minHollowRatio, MemoryStack scratch)
    {
      const s32 numComponents = components.get_size();

      ConnectedComponentSegment<Type> * restrict pComponents = components.Pointer(0);

      FixedLengthList<s32> maxCenterDistanceSum(maximumId+1, scratch, Flags::Buffer(false,false,true));

      maxCenterDistanceSum.Set(0);

      s32 * restrict pMaxCenterDistanceSum = maxCenterDistanceSum.Pointer(0);

      // Compute the sum of the maximium x-distance for each row
      {
        PUSH_MEMORY_STACK(scratch);

        FixedLengthList<s32> lastComponentY(maximumId+1, scratch, Flags::Buffer(false,false,true));
        FixedLengthList<s32> lastComponentRightEdge(maximumId+1, scratch, Flags::Buffer(false,false,true));
        FixedLengthList<s32> maxCenterDistance(maximumId+1, scratch, Flags::Buffer(false,false,true));

        s32 * restrict pLastComponentY = lastComponentY.Pointer(0);
        s32 * restrict pLastComponentRightEdge = lastComponentRightEdge.Pointer(0);
        s32 * restrict pMaxCenterDistance = maxCenterDistance.Pointer(0);

        lastComponentY.Set(-1);
        maxCenterDistance.Set(0);

        for(s32 iComponent=0; iComponent<numComponents; iComponent++) {
          const Type id = pComponents[iComponent].id;
          const s16 xStart = pComponents[iComponent].xStart;
          const s16 xEnd = pComponents[iComponent].xEnd;
          const s16 y = pComponents[iComponent].y;

          const s32 lastY = pLastComponentY[id];

          if(y == lastY) {
            const s32 lastXEnd = pLastComponentRightEdge[id];
            const s32 xDistance = xStart - lastXEnd - 1;
            AnkiAssert(xDistance > 0);
            pMaxCenterDistance[id] = MAX(pMaxCenterDistance[id], xDistance);
            pLastComponentRightEdge[id] = xEnd;
          } else {
            pMaxCenterDistanceSum[id] += pMaxCenterDistance[id];
            pMaxCenterDistance[id] = 0;
            pLastComponentY[id] = y;
            pLastComponentRightEdge[id] = xEnd;
          }
        } // for(s32 iComponent=0; iComponent<numComponents; iComponent++)

        for(s32 id=1; id<=maximumId; id++) {
          pMaxCenterDistanceSum[id] += pMaxCenterDistance[id];
        }
      } // Compute the sum of the maximium x-distance for each row

      FixedLengthList<bool> validComponents(maximumId+1, scratch, Flags::Buffer(false,false,true));
      validComponents.Set(true);

      bool * restrict pValidComponents = validComponents.Pointer(0);

      // If the sum of maximum x-distances is too small, invalidate the component
      {
        FixedLengthList<s32> componentSizes(maximumId+1, scratch, Flags::Buffer(false,false,true));
        const s32 * restrict pComponentSizes = componentSizes.Pointer(0);

        this->ComputeComponentSizes(componentSizes);

        for(s32 id=1; id<=maximumId; id++) {
          const f32 curCenterSum = static_cast<f32>(pMaxCenterDistanceSum[id]);
          const f32 curComponentSize = static_cast<f32>(pComponentSizes[id]);

          // ratio should be greater-than-or-equal-to 0, and could be greater than 1.0
          const f32 ratio = curCenterSum / curComponentSize;

          if(ratio < minHollowRatio) {
            pValidComponents[id] = false;
          }
        }
      } // If the sum of maximum x-distances is too small, invalidate the component

      // Set all invalid ComponentSegments to zero
      {
        for(s32 i=0; i<numComponents; i++) {
          const Type id = pComponents[i].id;

          if(!pValidComponents[id]) {
            pComponents[i].id = 0;
          }
        }
      } // Set all invalid ComponentSegments to zero

      FindMaximumId();

      this->isSortedInId = false;

      return RESULT_OK;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::PushBack(const ConnectedComponentSegment<Type> &value)
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

    template<typename Type> bool ConnectedComponentsTemplate<Type>::IsValid() const
    {
      // TODO: should we do any other checks here?

      if(curState == STATE_INVALID)
        return false;

      if(!components.IsValid())
        return false;

      if(curState == STATE_INITIALIZED) {
        if(!currentComponents1d.IsValid())
          return false;

        if(!previousComponents1d.IsValid())
          return false;

        if(!newPreviousComponents1d.IsValid())
          return false;

        if(!equivalentComponents.IsValid())
          return false;
      }

      return true;
    }

    template<typename Type> Result ConnectedComponentsTemplate<Type>::Print() const
    {
      return this->components.Print();
    }

    template<typename Type> Type ConnectedComponentsTemplate<Type>::get_maximumId() const
    {
      return maximumId;
    }

    template<typename Type> s32 ConnectedComponentsTemplate<Type>::get_size() const
    {
      return components.get_size();
    }

    template<typename Type> bool ConnectedComponentsTemplate<Type>::get_isSortedInId() const
    {
      return isSortedInId;
    }

    template<typename Type> bool ConnectedComponentsTemplate<Type>::get_isSortedInY() const
    {
      return isSortedInY;
    }

    template<typename Type> bool ConnectedComponentsTemplate<Type>::get_isSortedInX() const
    {
      return isSortedInX;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
