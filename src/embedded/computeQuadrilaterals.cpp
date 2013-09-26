#include "anki/embeddedVision/miscVisionKernels.h"

namespace Anki
{
  namespace Embedded
  {
    // Assumes that the quad pointer are in either clockwise or counter-clockwise order
    // quadSymmetryThreshold is SQ23.8 . A reasonable value is (1.5*pow(2,8)) = 384
    // A reasonable value for minQuadArea is 100
    static bool IsQuadrilateralValid(const Quadrilateral<s16> &quad, const s32 minQuadArea, const s32 quadSymmetryThreshold)
    {
      const s32 numFractionalBits = 8;

      // Swap the corners, as in the Matlab script
      Quadrilateral<s16> quadSwapped(quad[0], quad[3], quad[1], quad[2]);

      // Verify corners are in a clockwise direction, so we don't get an accidental projective
      // mirroring when we do the tranformation below to extract the image. Can look whether the z
      // direction of cross product of the two vectors forming the quadrilateral is positive or
      // negative.

      // cross product of vectors anchored at corner 0
      s32 detA = Determinant2x2(
        quadSwapped[1].x-quadSwapped[0].x, quadSwapped[1].y-quadSwapped[0].y,
        quadSwapped[2].x-quadSwapped[0].x, quadSwapped[2].y-quadSwapped[0].y);

      if(ABS(detA) < minQuadArea)
        return false;

      if(detA > 0) {
        // corners([2 3],:) = corners([3 2],:);
        const Quadrilateral<s16>  quadSwappedTmp = Quadrilateral<s16>(quadSwapped[0], quadSwapped[2], quadSwapped[1], quadSwapped[3]);
        quadSwapped = quadSwappedTmp;
        detA = -detA;
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
      const s32 detC = Determinant2x2(
        quadSwapped[3].x-quadSwapped[2].x, quadSwapped[3].y-quadSwapped[2].y,
        quadSwapped[0].x-quadSwapped[2].x, quadSwapped[0].y-quadSwapped[2].y);

      // cross product of vectors anchored at corner 1
      const s32 detD = Determinant2x2(
        quadSwapped[0].x-quadSwapped[1].x, quadSwapped[0].y-quadSwapped[1].y,
        quadSwapped[3].x-quadSwapped[1].x, quadSwapped[3].y-quadSwapped[1].y);

      if( !(SIGN(detA) == SIGN(detB) && SIGN(detC) == SIGN(detD)) )
        return false;

      detA = abs(detA);
      detB = abs(detB);

      const s32 maxDetAB = MAX(detA,detB);
      const s32 minDetAB = MIN(detA,detB);

      // Is the quad symmetry above the threshold?
      const s32 ratio1Value = maxDetAB << numFractionalBits;
      const s32 ratio2Value = minDetAB*quadSymmetryThreshold;
      if(ratio1Value >= ratio2Value)
        return false;

      return true;
    }

    // Based on an input ConnectedComponents class (that is sorted in id), compute the corner points
    // of any quadrilaterals Candidate quadrilaterals that are smaller than minQuadArea, or are not
    // symmetric enough relative to quadSymmetryThreshold are not returned
    //
    // quadSymmetryThreshold is SQ23.8 . A reasonable value is (1.5*pow(2,8)) = 384
    // A reasonable value for minQuadArea is 100
    //
    // Required ??? bytes of scratch
    Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, const s32 minQuadArea, const s32 quadSymmetryThreshold, FixedLengthList<Quadrilateral<s16> > &extractedQuads, MemoryStack scratch)
    {
      const s32 MAX_BOUNDARY_LENTH = 10000; // Probably significantly longer than would ever be needed

      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "components is not valid");

      AnkiConditionalErrorAndReturnValue(!components.get_isSortedInId(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "components must be sorted in id");

      FixedLengthList<Point<s16> > extractedBoundary(MAX_BOUNDARY_LENTH, scratch);
      FixedLengthList<Point<s16> > peaks(4, scratch);

      s32 startComponentIndex = 0;

      // Go throught the list of components, and for each id, extract a quadrilateral. If the
      // quadrilateral looks reasonable, add it to the list extractedQuads.
      for(s32 i=0; i<components.get_maximumId(); i++) {
        s32 endComponentIndex = -1;

        // For each component (the list must be sorted):
        // 1. Trace the exterior boundary
        if(TraceNextExteriorBoundary(components, startComponentIndex, extractedBoundary, endComponentIndex, scratch) != RESULT_OK)
          return RESULT_FAIL;

        startComponentIndex = endComponentIndex + 1;

        // 2. Compute the Laplacian peaks
        if(ExtractLaplacianPeaks(extractedBoundary, peaks, scratch) != RESULT_OK)
          return RESULT_FAIL;

        if(peaks.get_size() != 4)
          continue;

        Quadrilateral<s16> quad(peaks[0], peaks[1], peaks[2], peaks[3]);

        // 3. If the quadraleteral is reasonable, add the quad to the list of extractedQuads
        if(IsQuadrilateralValid(quad, minQuadArea, quadSymmetryThreshold)) {
          extractedQuads.PushBack(quad);
        }
      }

      return RESULT_OK;
    } // Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, Quadrilateral<s32> &extractedQuads, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki