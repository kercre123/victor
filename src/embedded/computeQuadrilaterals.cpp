#include "anki/embeddedVision/miscVisionKernels.h"

namespace Anki
{
  namespace Embedded
  {
    Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, Quadrilateral<s32> &extractedQuads, MemoryStack scratch)
    {
      const s32 MAX_BOUNDARY_LENTH = 10000; // Probably significantly longer than would ever be needed

      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "components is not valid");

      AnkiConditionalErrorAndReturnValue(!components.get_isSortedInId(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "components must be sorted in id");

      FixedLengthList<Point<s16>> extractedBoundary(MAX_BOUNDARY_LENTH, scratch);

      s32 startComponentIndex = 0;
      for(s32 i=0; i<components.get_size(); i++) {
        s32 endComponentIndex = -1;

        // For each component (the list must be sorted):
        // 1. Trace the exterior boundary
        if(TraceNextExteriorBoundary(components, startComponentIndex, extractedBoundary, endComponentIndex, scratch) != RESULT_OK)
          return RESULT_FAIL;

        // 2. Compute the Laplacian peaks
        // 3. Find the local maxima
        // 4. Check if the quad is large enough
        // 5. Flip the quad, if needed
        // 6. Check the the quad is roughly symmetrical
        // 7. If everything is good, add the quad to the list of extractedQuads
      }
    } // Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, Quadrilateral<s32> &extractedQuads, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki