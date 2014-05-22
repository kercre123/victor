/**
 * File: linearAlgebra_impl.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 5/2/2014
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description:
 *    Implements various linear algebra methods defined in linearAlgebra.h
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef ANKI_COMMON_BASESTATION_MATH_LINEAR_ALGEBRA_IMPL_H
#define ANKI_COMMON_BASESTATION_MATH_LINEAR_ALGEBRA_IMPL_H

#include "anki/common/basestation/math/linearAlgebra.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/matrix_impl.h"

namespace Anki {
  
  template<size_t N, typename T>
  SmallSquareMatrix<N,T> GetProjectionOperator(const Point<N,T>& normal)
  {
    SmallSquareMatrix<N,T> P;
  
    // Normal needs to be a unit vector!
    CORETECH_ASSERT(normal.Length() == 1.f);
    
    for(MatDimType i=0; i<N; ++i) {
      // Fill in diagonal elements
      P(i,i) = static_cast<T>(1) - normal[i]*normal[i];
      
      // Fill in off-diagonal elements
      for(MatDimType j=i+1; j<N; ++j) {
        P(i,j) = -normal[i]*normal[j];
        P(j,i) = P(i,j);
      }
    }
    
    return P;
  }
  
} // namespace Anki

#endif // ANKI_COMMON_BASESTATION_MATH_LINEAR_ALGEBRA_IMPL_H