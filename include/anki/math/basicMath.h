#ifndef _ANKICORETECH_BASICMATH_H_
#define _ANKICORETECH_BASICMATH_H_

#include "anki/common/datastructures.h"

namespace Anki {
  
  // Forward declarations:
  template<typename T> class Matrix;
  template<typename T> class Point3;
  typedef Point3<float> Vec3f;
    
  // Rodrigues' formula for converting between angle+axis representation and 3x3
  // matrix representation.
  Result Rodrigues(const Vec3f  &Rvec_in, Matrix<float> &Rmat_out);
  Result Rodrigues(const Matrix<float> &Rmat_in, Vec3f &Rvec_out);
  
} // namespace Anki

#endif //_ANKICORETECH_BASICMATH_H_