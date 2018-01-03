/**
File: traceBoundary.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/fiducialDetection.h"

namespace Anki
{
  namespace Embedded
  {
    // Starting a components.Pointer(startComponentIndex), trace the exterior boundary for the
    // component starting at startComponentIndex. extractedBoundary must be at at least
    // "3*componentWidth + 3*componentHeight" (If you don't know the size of the component, you can
    // just make it "3*imageWidth + 3*imageHeight" ). It's possible that a component could be
    // arbitrarily large, so if you have the space, use as much as you have.
    //
    // endComponentIndex is the last index of the component starting at startComponentIndex. The
    // next component is therefore startComponentIndex+1 .
    //
    // Requires sizeof(s16)*(2*componentWidth + 2*componentHeight) bytes of scratch
    Result TraceNextExteriorBoundary(const ConnectedComponents &components, const s32 startComponentIndex, FixedLengthList<Point<s16> > &extractedBoundary, s32 &endComponentIndex, MemoryStack scratch)
    {
      const s32 numComponents = components.get_size();

      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "components is not valid");

      AnkiConditionalErrorAndReturnValue(extractedBoundary.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "extractedBoundary is not valid");

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(components.get_isSortedInId(),
        RESULT_FAIL_INVALID_PARAMETER, "ComputeQuadrilateralsFromConnectedComponents", "components must be sorted in id");

      AnkiConditionalErrorAndReturnValue(startComponentIndex >= 0 && startComponentIndex < numComponents,
        RESULT_FAIL_INVALID_PARAMETER, "ComputeQuadrilateralsFromConnectedComponents", "startComponentIndex is not in range");

      const bool useU16 = components.get_useU16();
      const ConnectedComponentsTemplate<u16>* componentsU16 = components.get_componentsU16();
      const ConnectedComponentsTemplate<s32>* componentsS32 = components.get_componentsS32();

      u16 componentIdU16 = 0;
      s32 componentIdS32 = 0;

      if(useU16) {
        componentIdU16 = (*componentsU16)[startComponentIndex].id;

        AnkiConditionalErrorAndReturnValue(componentIdU16 > 0,
          RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "componentId is not valid.");
      } else {
        componentIdS32 = (*componentsS32)[startComponentIndex].id;

        AnkiConditionalErrorAndReturnValue(componentIdS32 > 0,
          RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "componentId is not valid.");
      }

      extractedBoundary.Clear();

      // Compute the bounding box of the current component

      //coordinate_top = min(component(:,1), [], 1);
      //coordinate_bottom = max(component(:,1), [], 1);
      //coordinate_left = min(component(:,2), [], 1);
      //coordinate_right = max(component(:,3), [], 1);
      Rectangle<s16> boundingBox(std::numeric_limits<s16>::max(), std::numeric_limits<s16>::min(), std::numeric_limits<s16>::max(), std::numeric_limits<s16>::min());
      endComponentIndex = numComponents - 1;
      
      if(useU16) {
        for(s32 i=startComponentIndex; i<numComponents; i++) {
          if((*componentsU16)[i].id != componentIdU16) {
            endComponentIndex = i-1;
            break;
          }

          const s16 xStart = (*componentsU16)[i].xStart;
          const s16 xEnd = (*componentsU16)[i].xEnd;
          const s16 y = (*componentsU16)[i].y;

          boundingBox.left = MIN(boundingBox.left, xStart);
          boundingBox.right = MAX(boundingBox.right, xEnd+1); // +1, because the coorindate we want is the crack after the right pixel
          boundingBox.top = MIN(boundingBox.top, y);
          boundingBox.bottom = MAX(boundingBox.bottom, y+1); // +1, because the coorindate we want is the crack after the bottom pixel
        }        
      } else { // if(useU16)
        for(s32 i=startComponentIndex; i<numComponents; i++) {
          if((*componentsS32)[i].id != componentIdS32) {
            endComponentIndex = i-1;
            break;
          }

          const s16 xStart = (*componentsS32)[i].xStart;
          const s16 xEnd = (*componentsS32)[i].xEnd;
          const s16 y = (*componentsS32)[i].y;

          boundingBox.left = MIN(boundingBox.left, xStart);
          boundingBox.right = MAX(boundingBox.right, xEnd+1); // +1, because the coorindate we want is the crack after the right pixel
          boundingBox.top = MIN(boundingBox.top, y);
          boundingBox.bottom = MAX(boundingBox.bottom, y+1); // +1, because the coorindate we want is the crack after the bottom pixel
        }
      } // if(useU16) ... else

      if(boundingBox.left == std::numeric_limits<s16>::max() || boundingBox.right == std::numeric_limits<s16>::min() || boundingBox.top == std::numeric_limits<s16>::max() || boundingBox.bottom == std::numeric_limits<s16>::min()) {
        AnkiWarn("ComputeQuadrilateralsFromConnectedComponents", "Something was corrupted with the input component");
        return RESULT_FAIL;
      }

      const s16 boxWidth = boundingBox.get_width();
      const s16 boxHeight = boundingBox.get_height();

      s16 * edge_left = reinterpret_cast<s16*>(scratch.Allocate(sizeof(s16)*boxHeight));
      s16 * edge_right = reinterpret_cast<s16*>(scratch.Allocate(sizeof(s16)*boxHeight));
      s16 * edge_top = reinterpret_cast<s16*>(scratch.Allocate(sizeof(s16)*boxWidth));
      s16 * edge_bottom = reinterpret_cast<s16*>(scratch.Allocate(sizeof(s16)*boxWidth));

      // Set the right away, in case a buggy component is missing a row or column
      for(s32 i=0; i<boxHeight;i++){
        edge_left[i] = std::numeric_limits<s16>::max();
        edge_right[i] = std::numeric_limits<s16>::min();
      }

      for(s32 i=0; i<boxWidth;i++){
        edge_top[i] = std::numeric_limits<s16>::max();
        edge_bottom[i] = std::numeric_limits<s16>::min();
      }

      //% 1. Compute the extreme pixels of the components, on each edge
      if(useU16) {
        for(s32 iSegment=startComponentIndex; iSegment<=endComponentIndex; iSegment++) {
          //component(:, 1) = component(:, 1) - coordinate_top + 1;
          //component(:, 2:3) = component(:, 2:3) - coordinate_left + 1;
          //xStart = component(iSubComponent, 2);
          //xEnd = component(iSubComponent, 3);
          //y = component(iSubComponent, 1);
          ConnectedComponentSegment<u16> currentSegment = (*componentsU16)[iSegment];
          currentSegment.xEnd -= boundingBox.left;
          currentSegment.xStart -= boundingBox.left;
          currentSegment.y -= boundingBox.top;

          AnkiAssert(currentSegment.xStart >= 0);
          for(s32 x=currentSegment.xStart; x<=currentSegment.xEnd; x++) {
            edge_top[x] = MIN(edge_top[x], currentSegment.y);
            edge_bottom[x] = MAX(edge_bottom[x], currentSegment.y);

            edge_left[currentSegment.y] = MIN(edge_left[currentSegment.y], currentSegment.xStart);
            edge_right[currentSegment.y] = MAX(edge_right[currentSegment.y], currentSegment.xEnd);
          } // for(s32 x=currentSegment.xStart; x<=currentSegment.xEnd; x++)
        } // for(s32 iSegment=startComponentIndex; iSegment<=endComponentIndex; iSegment++)
      } else { // if(useU16)
        for(s32 iSegment=startComponentIndex; iSegment<=endComponentIndex; iSegment++) {
          //component(:, 1) = component(:, 1) - coordinate_top + 1;
          //component(:, 2:3) = component(:, 2:3) - coordinate_left + 1;
          //xStart = component(iSubComponent, 2);
          //xEnd = component(iSubComponent, 3);
          //y = component(iSubComponent, 1);
          ConnectedComponentSegment<s32> currentSegment = (*componentsS32)[iSegment];
          currentSegment.xEnd -= boundingBox.left;
          currentSegment.xStart -= boundingBox.left;
          currentSegment.y -= boundingBox.top;

          AnkiAssert(currentSegment.xStart >= 0);
          for(s32 x=currentSegment.xStart; x<=currentSegment.xEnd; x++) {
            edge_top[x] = MIN(edge_top[x], currentSegment.y);
            edge_bottom[x] = MAX(edge_bottom[x], currentSegment.y);

            edge_left[currentSegment.y] = MIN(edge_left[currentSegment.y], currentSegment.xStart);
            edge_right[currentSegment.y] = MAX(edge_right[currentSegment.y], currentSegment.xEnd);
          } // for(s32 x=currentSegment.xStart; x<=currentSegment.xEnd; x++)
        } // for(s32 iSegment=startComponentIndex; iSegment<=endComponentIndex; iSegment++)
      } // if(useU16) ... else

      //#define PRINT_OUT_EDGE_LIMITS
#ifdef PRINT_OUT_EDGE_LIMITS
      CoreTechPrint("  ");
      for(s32 i=0; i<boxWidth;i++){
        CoreTechPrint("%d ", edge_top[i]);
      }
      CoreTechPrint("\n");

      for(s32 i=0; i<boxHeight;i++){
        CoreTechPrint("%d                    %d\n", edge_left[i], edge_right[i]);
      }
      CoreTechPrint("\n");

      for(s32 i=0; i<boxWidth;i++){
        CoreTechPrint("%d ", edge_bottom[i]);
      }

      CoreTechPrint("\n");
#endif // #ifdef PRINT_OUT_EDGE_LIMITS

      // The components are computed with an approximate method. This means that for complex shapes,
      // it is possible to have non-contiguous components. These checks are for such non-contiguous components.
      //
      // It is possible to use a heuristic to compute the boundary of non-continguous components,
      // but I think they will generally not occur with good, non-occluded fiducial markers.
      bool isNonContiguous = false;

      for(s32 y=0; y<boxHeight; y++) {
        if(edge_left[y] == std::numeric_limits<s16>::max() || edge_right[y] == std::numeric_limits<s16>::min()) {
          //CoreTechPrint("edge_left[%d]=%d edge_right[%d]=%d\n", y, static_cast<s32>(edge_left[y]), y, static_cast<s32>(edge_right[y]));
          //AnkiWarn("TraceNextExteriorBoundary", "Bad edge");
          isNonContiguous = true;
        }
      }

      for(s32 x=0; x<boxWidth; x++) {
        if(edge_top[x] == std::numeric_limits<s16>::max() || edge_bottom[x] == std::numeric_limits<s16>::min()) {
          //CoreTechPrint("edge_top[%d]=%d edge_bottom[%d]=%d\n", x, static_cast<s32>(edge_left[x]), x, static_cast<s32>(edge_right[x]));
          //AnkiWarn("TraceNextExteriorBoundary", "Bad edge");
          isNonContiguous = true;
        }
      }

      if(isNonContiguous) {
        //printf("non contiguous\n");
        return RESULT_OK;
      }

      //boundary = zeros(0, 2);

      //% 2. Go through the right edge, from top to bottom. Add each to the
      //% boundary. If two right edge pixels are not adjacent, draw a line between
      //% them. The boundary will be Manhattan (4-connected) style

      //boundary(end+1, :) = [edge_right(1), 1];
      extractedBoundary.PushBack(Point<s16>(edge_right[0], 0));

      //for y = 2:height
      for(s16 y=1; y<boxHeight; y++) {
        //% Draw the horizontal line between the previous and current right edge

        //if edge_right(y) > edge_right(y-1)
        if(edge_right[y] > edge_right[y-1]) {
          //  lineWidth = edge_right(y) - edge_right(y-1);
          //  newBoundary = zeros(lineWidth, 2);
          //  newBoundary(:,1) = edge_right(y-1):(edge_right(y)-1);
          //  newBoundary(:,2) = y;
          //  boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
          for(s16 x=edge_right[y-1]; x<=(edge_right[y]-1); x++) {
            extractedBoundary.PushBack(Point<s16>(x,y));
          }
        } else if(edge_right[y-1] > edge_right[y]) {
          //  lineWidth = edge_right(y-1) - edge_right(y);
          //  newBoundary = zeros(lineWidth, 2);
          //  newBoundary(:,1) = (edge_right(y-1)-1):-1:edge_right(y);
          //  newBoundary(:,2) = y-1;
          //  boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
          for(s16 x=edge_right[y-1]-1; x>=edge_right[y]; x--) {
            extractedBoundary.PushBack(Point<s16>(x,y-1));
          }
        } // if(edge_right[y] > edge_right[y-1]) ... elseif

        //boundary(end+1, :) = [edge_right(y), y];
        extractedBoundary.PushBack(Point<s16>(edge_right[y],y));
      } // for(s16 y=1; y<boxHeight; y++)

      //% 3. Make a bridge from the bottomost right to the bottomost left. Make the
      //% bridge using the bottom edge pixels. Note that this this loop should by
      //% definition start at a pixel that is both a bottom and a right (I don't see a way this can't be true).
      //for x = edge_right(end):-1:(edge_left(end)+1)
      for(s16 x=edge_right[boxHeight-1]; x>=(edge_left[boxHeight-1]+1); x--) {
        //if edge_bottom(x) > edge_bottom(x-1)
        if(edge_bottom[x] > edge_bottom[x-1]) {
          //    lineHeight = edge_bottom(x) - edge_bottom(x-1);
          //    newBoundary = zeros(lineHeight, 2);
          //    newBoundary(:,1) = x;
          //    newBoundary(:,2) = (edge_bottom(x)-1):-1:edge_bottom(x-1);
          //    boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
          for(s16 y=edge_bottom[x]-1; y>=edge_bottom[x-1]; y--) {
            extractedBoundary.PushBack(Point<s16>(x,y));
          }

          //elseif edge_bottom(x-1) > edge_bottom(x)
        } else if(edge_bottom[x-1] > edge_bottom[x]) {
          //    lineHeight = edge_bottom(x-1) - edge_bottom(x);
          //    newBoundary = zeros(lineHeight, 2);
          //    newBoundary(:,1) = x-1;
          //    newBoundary(:,2) = edge_bottom(x):(edge_bottom(x-1)-1);
          //    boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
          for(s16 y=edge_bottom[x]; y<=(edge_bottom[x-1]-1); y++) {
            extractedBoundary.PushBack(Point<s16>(x-1,y));
          }
        }

        //boundary(end+1, :) = [x-1, edge_bottom(x-1)];
        extractedBoundary.PushBack(Point<s16>(x-1, edge_bottom[x-1]));
      } // for(s16 x=edge_right[boxHeight-1]; x>=(edge_left[boxHeight-1]+1); x--)

      //% 4. Go through the left edge, from bottom to top.
      //for y = (height-1):-1:1
      for(s16 y=boxHeight-2; y>=0; y--) {
        //% Draw the horizontal line between the previous and current left edge
        //if edge_left(y) > edge_left(y+1)
        if(edge_left[y] > edge_left[y+1]) {
          //    lineWidth = edge_left(y) - edge_left(y+1);
          //    newBoundary = zeros(lineWidth, 2);
          //    newBoundary(:,1) = (edge_left(y+1)+1):(edge_left(y));
          //    newBoundary(:,2) = y+1;
          //    boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
          for(s16 x=edge_left[y+1]+1; x<=edge_left[y]; x++) {
            extractedBoundary.PushBack(Point<s16>(x,y+1));
          }
          //elseif edge_left(y+1) > edge_left(y)
        } else if(edge_left[y+1] > edge_left[y]) {
          //    lineWidth = edge_left(y+1) - edge_left(y);
          //    newBoundary = zeros(lineWidth, 2);
          //    newBoundary(:,1) = edge_left(y+1):-1:(edge_left(y)+1);
          //    newBoundary(:,2) = y;
          //    boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
          for(s16 x=edge_left[y+1]; x>=(edge_left[y]+1); x--) {
            extractedBoundary.PushBack(Point<s16>(x,y));
          }
        }

        //boundary(end+1, :) = [edge_left(y), y];
        extractedBoundary.PushBack(Point<s16>(edge_left[y],y));
      } // for(s16 y=boxHeight-2; y>=0; y--)

      //% 5. Make a bridge from the topmost left pixel to the topmost right.
      //for x = (edge_left(1)+1):edge_right(1)
      for(s16 x=(edge_left[0]+1); x<=edge_right[0]; x++) {
        //if edge_top(x) > edge_top(x-1)
        if(edge_top[x] > edge_top[x-1]) {
          //    lineHeight = edge_top(x) - edge_top(x-1);
          //    newBoundary = zeros(lineHeight, 2);
          //    newBoundary(:,1) = x-1;
          //    newBoundary(:,2) = (edge_top(x-1)+1):edge_top(x);
          //    boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
          for(s16 y=edge_top[x-1]+1; y<=edge_top[x]; y++) {
            extractedBoundary.PushBack(Point<s16>(x-1,y));
          }

          //elseif edge_top(x-1) > edge_top(x)
        } else if(edge_top[x-1] > edge_top[x]) {
          //    lineHeight = edge_top(x-1) - edge_top(x);
          //    newBoundary = zeros(lineHeight, 2);
          //    newBoundary(:,1) = x;
          //    newBoundary(:,2) = edge_top(x-1):-1:(edge_top(x)+1);
          //    boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
          for(s16 y=edge_top[x-1]; y>=(edge_top[x]+1); y--) {
            extractedBoundary.PushBack(Point<s16>(x,y));
          }
        }

        //boundary(end+1, :) = [x, edge_top(x)];
        extractedBoundary.PushBack(Point<s16>(x,edge_top[x]));
      }

      // Shift back the coordinate frame
      //boundary(:,1) = boundary(:,1) + coordinate_left - 1;
      //boundary(:,2) = boundary(:,2) + coordinate_top - 1;
      {
        Point<s16> * restrict pExtractedBoundary = extractedBoundary.Pointer(0);
        const s32 lengthExtractedBoundary = extractedBoundary.get_size();
        for(s32 i=0; i<lengthExtractedBoundary; i++) {
          pExtractedBoundary[i].x += boundingBox.left;
          pExtractedBoundary[i].y += boundingBox.top;
        }
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki
