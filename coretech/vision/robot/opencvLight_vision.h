// Opencv functions and wrappers

#ifndef _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_VISION_H_
#define _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_VISION_H_

#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/fixedLengthList.h"
#include "coretech/common/robot/flags_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    Result CannyEdgeDetection(
      const Array<u8> &src, Array<u8> &dst,
      const s32 low_thresh, const s32 high_thresh,
      const s32 aperture_size,
      MemoryStack scratch);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_VISION_H_
