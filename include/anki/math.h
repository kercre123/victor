#ifndef _ANKICORETECH_MATH_H_
#define _ANKICORETECH_MATH_H_

#include "anki/math/config.h"

namespace Anki {
  
  // Forward declarations:
  class Vec3f;
  class Mat3x3;
  class Result;
  
  // Rodrigues' formula for converting between angle+axis representation and 3x3
  // matrix representation.
  Result Rodrigues(const Vec3f  &Rvec_in, Mat3x3 &Rmat_out);
  Result Rodrigues(const Mat3x3 &Rmat_in, Vec3f  &Rvec_out);
  
} // namespace Anki
#endif //_ANKICORETECH_VISION_VISION_H_
