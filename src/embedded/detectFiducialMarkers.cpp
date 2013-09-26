#include "anki/embeddedVision/fixedLengthList_vision.h"
#include "anki/embeddedVision/miscVisionKernels.h"
#include "anki/embeddedVision/draw_vision.h"

namespace Anki
{
  namespace Embedded
  {
    // Replaces the matlab code for the first three steps of SimpleDetector
    IN_DDR Result SimpleDetector_Steps123(
      const Array<u8> &image,
      const s32 scaleImage_numPyramidLevels,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
      const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
      ConnectedComponents &extractedComponents,
      MemoryStack scratch1,
      MemoryStack scratch2)
    {
      // TODO: figure out a simpler way to write code that reuses big blocks of memory

      Array<u8> binaryImage(image.get_size(0), image.get_size(1), scratch2);

      // 1. Compute the Scale image (use local scratch1)
      // 2. Binarize the Scale image (store in outer scratch2)
      {
        PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

        FixedPointArray<u32> scaleImage(image.get_size(0), image.get_size(1), 16, scratch1);

        if(ComputeCharacteristicScaleImage(image, scaleImage_numPyramidLevels, scaleImage, scratch2) != RESULT_OK) {
          return RESULT_FAIL;
        }

        if(ThresholdScaleImage(image, scaleImage, binaryImage) != RESULT_OK) {
          return RESULT_FAIL;
        }

        // image.Show("image", false, false);
        // binaryImage.Show("binaryImage", true, true);
      } // PUSH_MEMORY_STACK(scratch1);

      // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
      //FixedLengthList<ConnectedComponentSegment> extractedComponents(maxConnectedComponentSegments, scratch1);
      {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

        if(extractedComponents.Extract2dComponents(binaryImage, component1d_minComponentWidth, component1d_maxSkipDistance, scratch2) != RESULT_OK) {
          return RESULT_FAIL;
        }

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(image.get_size(0), image.get_size(1), scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents0", false);
        //}

        extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(image.get_size(0), image.get_size(1), scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents1", false);
        //}

        if(extractedComponents.InvalidateSmallOrLargeComponents(component_minimumNumPixels, component_maximumNumPixels, scratch2) != RESULT_OK) {
          return RESULT_FAIL;
        }

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(image.get_size(0), image.get_size(1), scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents2", false);
        //}

        extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(image.get_size(0), image.get_size(1), scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents3", false);
        //}

        if(extractedComponents.InvalidateSolidOrSparseComponents(component_sparseMultiplyThreshold, component_solidMultiplyThreshold, scratch2) != RESULT_OK) {
          return RESULT_FAIL;
        }

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(image.get_size(0), image.get_size(1), scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents4", false);
        //}

        extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(image.get_size(0), image.get_size(1), scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents4", true);
        //}
      } // PUSH_MEMORY_STACK(scratch2);

      return RESULT_OK;
    } // SimpleDetector_Steps123()

    IN_DDR Result SimpleDetector_Steps1234(
      const Array<u8> &image,
      FixedLengthList<FiducialMarker> &markers,
      const s32 scaleImage_numPyramidLevels,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
      const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
      const s32 quads_minQuadArea, const s32 quads_quadSymmetryThreshold,
      MemoryStack scratch1,
      MemoryStack scratch2)
    {
      // TODO: figure out a simpler way to write code that reuses big blocks of memory

      const s32 maxConnectedComponentSegments = u16_MAX;
      const s32 maxCandidateMarkers = 1000;
      const s32 maxExtractedQuads = 100;

      // Stored in the outermost scratch2
      FixedLengthList<FiducialMarker> candidateMarkers(maxCandidateMarkers, scratch2);

      ConnectedComponents extractedComponents; // This isn't allocated until after the scaleImage
      {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack
        Array<u8> binaryImage(image.get_size(0), image.get_size(1), scratch2);

        // 1. Compute the Scale image (use local scratch1)
        // 2. Binarize the Scale image (store in outer scratch2)
        {
          PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

          FixedPointArray<u32> scaleImage(image.get_size(0), image.get_size(1), 16, scratch1);

          if(ComputeCharacteristicScaleImage(image, scaleImage_numPyramidLevels, scaleImage, scratch2) != RESULT_OK)
            return RESULT_FAIL;

          if(ThresholdScaleImage(image, scaleImage, binaryImage) != RESULT_OK)
            return RESULT_FAIL;
        } // PUSH_MEMORY_STACK(scratch1);

        // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
        extractedComponents = ConnectedComponents(maxConnectedComponentSegments, scratch1);
        {
          PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

          if(extractedComponents.Extract2dComponents(binaryImage, component1d_minComponentWidth, component1d_maxSkipDistance, scratch2) != RESULT_OK)
            return RESULT_FAIL;

          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

          if(extractedComponents.InvalidateSmallOrLargeComponents(component_minimumNumPixels, component_maximumNumPixels, scratch2) != RESULT_OK)
            return RESULT_FAIL;

          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

          if(extractedComponents.InvalidateSolidOrSparseComponents(component_sparseMultiplyThreshold, component_solidMultiplyThreshold, scratch2) != RESULT_OK)
            return RESULT_FAIL;

          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);
        } // PUSH_MEMORY_STACK(scratch2);
      } // PUSH_MEMORY_STACK(scratch2);

      // 4. Compute candidate quadrilaterals from the connected components
      FixedLengthList<Quadrilateral<s16> > extractedQuads(maxExtractedQuads, scratch2);
      {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

        if(ComputeQuadrilateralsFromConnectedComponents(extractedComponents, quads_minQuadArea, quads_quadSymmetryThreshold, extractedQuads, scratch2) != RESULT_OK)
          return RESULT_FAIL;
      } // PUSH_MEMORY_STACK(scratch2);

      // 4b. Compute a homography for each extracted quadrilateral
      markers.set_size(extractedQuads.get_size());
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

        FiducialMarker &currentMarker = markers[iQuad];

        if(ComputeHomographyFromQuad(extractedQuads[iQuad], currentMarker.homography, scratch2) != RESULT_OK)
          return RESULT_FAIL;
      } // for(iQuad=0; iQuad<; iQuad++)

      // 5. Decode fiducial markers from the candidate quadrilaterals
      // TODO: do decoding

      return RESULT_OK;
    } //  SimpleDetector_Steps1234()

    //IN_DDR Result DetectFiducialMarkers(const Array<u8> &image,
    //  FixedLengthList<FiducialMarker> &markers,
    //  const s32 numPyramidLevels,
    //  MemoryStack scratch1,
    //  MemoryStack scratch2)
    //{
    //  // TODO: figure out a simpler way to write code that reuses big blocks of memory

    //  const s32 maxConnectedComponentSegments = u16_MAX;
    //  const s16 minComponentWidth = 3;
    //  const s16 maxSkipDistance = 1;
    //  const s32 maxCandidateMarkers = 1000;

    //  const f32 minSideLength = Round(0.03f*MAX(480,640));
    //  const f32 maxSideLength = Round(0.9f*MIN(480,640));

    //  const s32 minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
    //  const s32 maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
    //  const s32 sparseMultiplyThreshold = 1000;
    //  const s32 solidMultiplyThreshold = 2;

    //  const s32 maxExtractedQuads = 100;
    //  const s32 minQuadArea = 100;
    //  const s32 quadSymmetryThreshold = 384;

    //  // Stored in the outermost scratch2
    //  FixedLengthList<FiducialMarker> candidateMarkers(maxCandidateMarkers, scratch2);

    //  ConnectedComponents extractedComponents; // This isn't allocated until after the scaleImage
    //  {
    //    PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack
    //    Array<u8> binaryImage(image.get_size(0), image.get_size(1), scratch2);

    //    // 1. Compute the Scale image (use local scratch1)
    //    // 2. Binarize the Scale image (store in outer scratch2)
    //    {
    //      PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

    //      FixedPointArray<u32> scaleImage(image.get_size(0), image.get_size(1), 16, scratch1);

    //      if(ComputeCharacteristicScaleImage(image, numPyramidLevels, scaleImage, scratch2) != RESULT_OK)
    //        return RESULT_FAIL;

    //      if(ThresholdScaleImage(image, scaleImage, binaryImage) != RESULT_OK)
    //        return RESULT_FAIL;
    //    } // PUSH_MEMORY_STACK(scratch1);

    //    // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
    //    extractedComponents = ConnectedComponents(maxConnectedComponentSegments, scratch1);
    //    {
    //      PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

    //      if(extractedComponents.Extract2dComponents(binaryImage, minComponentWidth, maxSkipDistance, scratch2) != RESULT_OK)
    //        return RESULT_FAIL;

    //      extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

    //      if(extractedComponents.InvalidateSmallOrLargeComponents(minimumNumPixels, maximumNumPixels, scratch2) != RESULT_OK)
    //        return RESULT_FAIL;

    //      extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

    //      if(extractedComponents.InvalidateSolidOrSparseComponents(sparseMultiplyThreshold, solidMultiplyThreshold, scratch2) != RESULT_OK)
    //        return RESULT_FAIL;

    //      extractedComponents.CompressConnectedComponentSegmentIds(scratch2);
    //    } // PUSH_MEMORY_STACK(scratch2);
    //  } // PUSH_MEMORY_STACK(scratch2);

    //  // 4. Compute candidate quadrilaterals from the connected components
    //  FixedLengthList<Quadrilateral<s16> > extractedQuads(maxExtractedQuads, scratch2);
    //  {
    //    PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

    //    // TODO: compute candiate quads
    //    if(ComputeQuadrilateralsFromConnectedComponents(extractedComponents, minQuadArea, quadSymmetryThreshold, extractedQuads, scratch2) != RESULT_OK)
    //      return RESULT_FAIL;
    //  } // PUSH_MEMORY_STACK(scratch2);

    //  FixedLengthList<Array<f64> > homographies(extractedQuads.get_size(), scratch2);
    //  {
    //    PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

    //    if(ComputeHomographyFromQuad(extractedQuads, homographies, scratch2) != RESULT_OK)
    //      return RESULT_FAIL;
    //  } // PUSH_MEMORY_STACK(scratch2);

    //  // 5. Decode fiducial markers from the candidate quadrilaterals
    //  // TODO: do decoding

    //  return RESULT_OK;
    //} //  DetectFiducialMarkers()
  } // namespace Embedded
} // namespace Anki