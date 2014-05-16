// Opencv functions and wrappers

#ifndef _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_
#define _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/flags_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    void CannyEdgeDetection(
      InputArray _src, OutputArray _dst,
      double low_thresh, double high_thresh,
      int aperture_size, bool L2gradient );
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_
