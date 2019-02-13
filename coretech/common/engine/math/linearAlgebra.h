/**
 * File: linearAlgebra.h
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
 *    Defines various linear algebra methods. Templated implementations are
 *    in a separate linearAlgebra_impl.h file.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef ANKI_COMMON_BASESTATION_MATH_LINEAR_ALGEBRA_H
#define ANKI_COMMON_BASESTATION_MATH_LINEAR_ALGEBRA_H

#include "coretech/common/shared/math/matrix.h"
#include "coretech/common/shared/math/point_fwd.h"

namespace Anki {
  
  // Returns a projection operator for the given plane normal, n,
  //
  //   P = I - n*n^T
  //
  // such that
  //
  //   v' = P*v
  //
  // removes all variation of v along the normal. Note that the normal
  // must be a unit vector!
  //
  // TODO: make normal a UnitVector type
  template<size_t N, typename T>
  SmallSquareMatrix<N,T> GetProjectionOperator(const Point<N,T>& normal);
 
  enum class LeastSquaresMethod : u8 {
    LU,
    Cholesky,
    Eigenvalue,
    SVD,
    QR
  };
  
  // Returns x for the linear system Ax = b
  // 1. b and x are columns stored as a SmallMatrix
  template<MatDimType M, MatDimType N, typename T>
  Result LeastSquares(const SmallMatrix<M, N, T>&   A,
                      const SmallMatrix<M, 1, T>&   b,
                      SmallMatrix<N,1,T>&           x,
                      LeastSquaresMethod            method = LeastSquaresMethod::LU);
  
  // 2. b and x are stored as vectors (points)
  template<MatDimType M, MatDimType N, typename T>
  Result LeastSquares(const SmallMatrix<M, N, T>&   A,
                      const Point<M,T>&             b,
                      Point<N,T>&                   x,
                      LeastSquaresMethod            method = LeastSquaresMethod::LU);
  
  
} // namespace Anki

#endif // ANKI_COMMON_BASESTATION_MATH_LINEAR_ALGEBRA_H
