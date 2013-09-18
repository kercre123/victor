#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    IN_DDR Result DetectFiducialMarkers(const Array<u8> &image,
      FixedLengthList<FiducialMarker> &markers,
      s32 numPyramidLevels,
      MemoryStack scratch1,
      MemoryStack scratch2)
    {
      // TODO: This whole function seems pretty confusing, but I can't think of a less confusing way to efficiently reuse the big blocks of memory

      const s32 maxConnectedComponentSegments = u16_MAX;
      const s16 minComponentWidth = 3;
      const s16 maxSkipDistance = 1;
      const s32 maxCandidateMarkes = 1000;

      // Stored in the outermost scratch2
      FixedLengthList<FiducialMarker> candidateMarkers(maxCandidateMarkes, scratch2);

      // 1. Compute the Scale image
      // 2. Binarize the Scale image
      // 3. Compute connected components from the binary image
      // 4. Compute candidate quadrilaterals from the connected components
      {
        PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

        {
          PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack
          Array<u8> binaryImage(image.get_size(0), image.get_size(1), scratch2);

          // 1. Compute the Scale image (use local scratch1)
          // 2. Binarize the Scale image (store in outer scratch2)
          {
            PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

            Array<u32> scaleImage(image.get_size(0), image.get_size(1), scratch1);

            if(ComputeCharacteristicScaleImage(image, numPyramidLevels, scaleImage, scratch2) != RESULT_OK) {
              return RESULT_FAIL;
            }

            if(ThresholdScaleImage(image, scaleImage, binaryImage) != RESULT_OK) {
              return RESULT_FAIL;
            }
          } // PUSH_MEMORY_STACK(scratch1);

          // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
          FixedLengthList<ConnectedComponentSegment> extractedComponents(maxConnectedComponentSegments, scratch1);
          {
            PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

            if(Extract2dComponents(binaryImage, minComponentWidth, maxSkipDistance, extractedComponents, scratch2) != RESULT_OK) {
              return RESULT_FAIL;
            }
          } // PUSH_MEMORY_STACK(scratch2);

          // 4. Compute candidate quadrilaterals from the connected components
          {
            PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

            // TODO: compute candiate quads
          }
        } // PUSH_MEMORY_STACK(scratch2);
      } // PUSH_MEMORY_STACK(scratch1);

      // 5. Decode fiducial markers from the candidate quadrilaterals
      // TODO: do decoding

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki