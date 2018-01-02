/**
File: computeQuadrilaterals.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/fiducialDetection.h"

//#define SEND_BOUNDARIES_TO_MATLAB
#ifdef SEND_BOUNDARIES_TO_MATLAB
#include "coretech/common/robot/matlabInterface.h"
#endif

namespace Anki
{
  namespace Embedded
  {
    static bool IsQuadrilateralValidAndUpdateOrdering(const Quadrilateral<s16> &quad, const s32 minQuadArea, const s32 quadSymmetryThreshold, const s32 minDistanceFromImageEdge, const s32 imageHeight, const s32 imageWidth, Quadrilateral<s16> &quadSwapped)
    {
      // Swap the corners, as in the Matlab script
      quadSwapped = Quadrilateral<s16>(quad[0], quad[3], quad[1], quad[2]);

      bool areCornersDisordered = false;
      const bool isReasonable = IsQuadrilateralReasonable(quadSwapped, minQuadArea, quadSymmetryThreshold, minDistanceFromImageEdge, imageHeight, imageWidth, areCornersDisordered);

      if(areCornersDisordered) {
        quadSwapped = Quadrilateral<s16>(quadSwapped[0], quadSwapped[2], quadSwapped[1], quadSwapped[3]);
      }

      return isReasonable;
    }

    bool IsQuadrilateralReasonable(const Quadrilateral<s16> &quad, const s32 minQuadArea, const s32 quadSymmetryThreshold, const s32 minDistanceFromImageEdge, const s32 imageHeight, const s32 imageWidth, bool &areCornersDisordered)
    {
      const s32 numFractionalBits = 8;

      // Verify corners are in a clockwise direction, so we don't get an accidental projective
      // mirroring when we do the tranformation below to extract the image. Can look whether the z
      // direction of cross product of the two vectors forming the quadrilateral is positive or
      // negative.

      // cross product of vectors anchored at corner 0
      s32 detA = Determinant2x2(
        quad[1].x-quad[0].x, quad[1].y-quad[0].y,
        quad[2].x-quad[0].x, quad[2].y-quad[0].y);

      if(ABS(detA) < minQuadArea)
        return false;

      // Swap the corners, as in the Matlab script
      Quadrilateral<s16> quadSwapped;

      if(detA > 0) {
        // corners([2 3],:) = corners([3 2],:);
        quadSwapped = Quadrilateral<s16>(quad[0], quad[2], quad[1], quad[3]);
        detA = -detA;
        areCornersDisordered = true;
      } else {
        quadSwapped = quad;
        areCornersDisordered = false;
      }

      // One last check: make sure we've got roughly a symmetric quadrilateral (a parallelogram?) by
      // seeing if the area computed using the cross product (via determinates) referenced to
      // opposite corners are similar (and the signs are in agreement, so that two of the sides
      // don't cross each other in the middle or form some sort of weird concave
      // shape) .

      // cross product of vectors anchored at corner 3
      s32 detB = Determinant2x2(
        quadSwapped[2].x-quadSwapped[3].x, quadSwapped[2].y-quadSwapped[3].y,
        quadSwapped[1].x-quadSwapped[3].x, quadSwapped[1].y-quadSwapped[3].y);

      // cross product of vectors anchored at corner 2
      s32 detC = Determinant2x2(
        quadSwapped[3].x-quadSwapped[2].x, quadSwapped[3].y-quadSwapped[2].y,
        quadSwapped[0].x-quadSwapped[2].x, quadSwapped[0].y-quadSwapped[2].y);

      // cross product of vectors anchored at corner 1
      s32 detD = Determinant2x2(
        quadSwapped[0].x-quadSwapped[1].x, quadSwapped[0].y-quadSwapped[1].y,
        quadSwapped[3].x-quadSwapped[1].x, quadSwapped[3].y-quadSwapped[1].y);

      if( !(signbit(detA) == signbit(detB) && signbit(detC) == signbit(detD)) )
        return false;

      detA = abs(detA);
      detB = abs(detB);
      detC = abs(detC);
      detD = abs(detD);

      const s32 maxDetAB = MAX(detA,detB);
      const s32 minDetAB = MIN(detA,detB);
      const s32 maxDetCD = MAX(detC,detD);
      const s32 minDetCD = MIN(detC,detD);

      // Is either quad symmetry check above the threshold?
      const s32 ratio1Value_AB = maxDetAB << numFractionalBits;
      const s32 ratio2Value_AB = minDetAB*quadSymmetryThreshold;
      const s32 ratio1Value_CD = maxDetCD << numFractionalBits;
      const s32 ratio2Value_CD = minDetCD*quadSymmetryThreshold;
      if(ratio1Value_AB >= ratio2Value_AB && ratio1Value_CD >= ratio2Value_CD)
        return false;

      // Check if any of the corners are close to the edge of the image
      for(s32 i=0; i<4; i++) {
        if(quadSwapped[i].x < minDistanceFromImageEdge || quadSwapped[i].y < minDistanceFromImageEdge ||
          quadSwapped[i].x >= (imageWidth - minDistanceFromImageEdge - 1) || quadSwapped[i].y >= (imageHeight - minDistanceFromImageEdge - 1) ) {
            return false;
        }
      }

      return true;
    }

    // Based on an input ConnectedComponents class (that is sorted in id), compute the corner points
    // of any quadrilaterals Candidate quadrilaterals that are smaller than minQuadArea, or are not
    // symmetric enough relative to quadSymmetryThreshold are not returned
    //
    // quadSymmetryThreshold is SQ23.8 . A reasonable value is (2.0*pow(2,8)) = 512
    // A reasonable value for minQuadArea is 100
    //
    // Required ??? bytes of scratch
    Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, const s32 minQuadArea, const s32 quadSymmetryThreshold, const s32 minDistanceFromImageEdge, const s32 minLaplacianPeakRatio, const s32 imageHeight, const s32 imageWidth, const CornerMethod cornerMethod, FixedLengthList<Quadrilateral<s16> > &extractedQuads, MemoryStack scratch)
    {
      static const s32 MAX_BOUNDARY_LENTH = 10000; // Probably significantly longer than would ever be needed

      Result lastResult;

      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "components is not valid");

      AnkiConditionalErrorAndReturnValue(components.get_isSortedInId(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "components must be sorted in id");

      FixedLengthList<Point<s16> > extractedBoundary(MAX_BOUNDARY_LENTH, scratch, Flags::Buffer(false,false,false));
      FixedLengthList<Point<s16> > peaks(4, scratch, Flags::Buffer(false,false,false));

      s32 startComponentIndex = 0;

      //printf("components.get_maximumId() = %d\n", components.get_maximumId());

#ifdef SEND_BOUNDARIES_TO_MATLAB
      Anki::Embedded::Matlab matlab(false);     
      matlab.EvalStringEcho("boundaries = {};");
#endif // #ifdef SEND_BOUNDARIES_TO_MATLAB

      // Go through the list of components, and for each id, extract a quadrilateral. If the
      // quadrilateral looks reasonable, add it to the list extractedQuads.
      for(s32 iComponent=0; iComponent<components.get_maximumId(); iComponent++) {
        s32 endComponentIndex = -1;

        // For each component (the list must be sorted):
        // 1. Trace the exterior boundary
        //BeginBenchmark("TraceNextExteriorBoundary");
        if((lastResult = TraceNextExteriorBoundary(components, startComponentIndex, extractedBoundary, endComponentIndex, scratch)) != RESULT_OK)
          return lastResult;
        //EndBenchmark("TraceNextExteriorBoundary");

        //printf("extractedBoundary.size = %d\n", extractedBoundary.get_size());
#ifdef SEND_BOUNDARIES_TO_MATLAB
        {
          std::shared_ptr<s16> xs(new s16[extractedBoundary.get_size()]);
          std::shared_ptr<s16> ys(new s16[extractedBoundary.get_size()]);

          for(s32 i=0; i<extractedBoundary.get_size(); i++) {
            xs.get()[i] = extractedBoundary[i].x;
            ys.get()[i] = extractedBoundary[i].y;
          }

          matlab.Put(xs.get(), extractedBoundary.get_size(), "xs");
          matlab.Put(ys.get(), extractedBoundary.get_size(), "ys");
          matlab.EvalStringEcho("boundaries{end+1} = [xs, ys]; clear('xs'); clear('ys');");
        }
#endif // #ifdef SEND_BOUNDARIES_TO_MATLAB

        startComponentIndex = endComponentIndex + 1;

        if(extractedBoundary.get_size() == 0)
          continue;

        // 2. Compute the Laplacian peaks
        //BeginBenchmark("ExtractPeaks");
        if(cornerMethod == CORNER_METHOD_LAPLACIAN_PEAKS) {
          if((lastResult = ExtractLaplacianPeaks(extractedBoundary, minLaplacianPeakRatio, peaks, scratch)) != RESULT_OK)
            return lastResult;
        } else if(cornerMethod == CORNER_METHOD_LINE_FITS) {
          if((lastResult = ExtractLineFitsPeaks(extractedBoundary, peaks, imageHeight, imageWidth, scratch)) != RESULT_OK)
            return lastResult;
        } else {
          AnkiAssert(false);
        }
        
        //EndBenchmark("ExtractPeaks");

        if(peaks.get_size() != 4)
          continue;

        Quadrilateral<s16> quad(peaks[0], peaks[1], peaks[2], peaks[3]);
        Quadrilateral<s16> quadSwapped;

        /*printf("curQuad = (%d,%d), (%d,%d), (%d,%d), (%d,%d)\n", 
          quad[0].x, quad[0].y,
          quad[1].x, quad[1].y,
          quad[2].x, quad[2].y,
          quad[3].x, quad[3].y);*/

        // 3. If the quadraleteral is reasonable, add the quad to the list of extractedQuads
        // IsQuadrilateralValidAndUpdateOrdering also changes the order of the points, into the non-rotated and corner-opposite format
        if(IsQuadrilateralValidAndUpdateOrdering(quad, minQuadArea, quadSymmetryThreshold, minDistanceFromImageEdge, imageHeight, imageWidth, quadSwapped)) {
          extractedQuads.PushBack(quadSwapped);
        }
      }

      return RESULT_OK;
    } // Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, Quadrilateral<s32> &extractedQuads, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki
