#include "anki/embeddedVision/miscVisionKernels.h"

namespace Anki
{
  namespace Embedded
  {
    Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, Quadrilateral<s32> &extractedQuads, MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(components.IsValid(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "components is not valid");

      AnkiConditionalErrorAndReturnValue(!components.get_isSortedInId(),
        RESULT_FAIL, "ComputeQuadrilateralsFromConnectedComponents", "components must be sorted in id");

      // For each component (the list must be sorted):
      // 1. Trace the exterior boundary
      // 2. Compute the Laplacian peaks
      // 3. Find the local maxima
      // 4. Check if the quad is large enough
      // 5. Flip the quad, if needed
      // 6. Check the the quad is roughly symmetrical
      // 7. If everything is good, add the quad to the list of extractedQuads
    } // Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, Quadrilateral<s32> &extractedQuads, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki