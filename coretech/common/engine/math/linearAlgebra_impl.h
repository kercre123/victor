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

#include "coretech/common/engine/math/linearAlgebra.h"
#include "coretech/common/shared/math/matrix_impl.h"

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

  // LUT for converting our least squares methods to OpenCV ones
  static inline int GetOpenCvSolveMethod(LeastSquaresMethod method)
  {
    switch(method)
    {
      case LeastSquaresMethod::LU:         return cv::DECOMP_LU;
      case LeastSquaresMethod::Cholesky:   return cv::DECOMP_CHOLESKY;
      case LeastSquaresMethod::Eigenvalue: return cv::DECOMP_EIG;
      case LeastSquaresMethod::SVD:        return cv::DECOMP_SVD;
      case LeastSquaresMethod::QR:         return cv::DECOMP_QR;
    }
  }
  
  template<MatDimType M, MatDimType N, typename T>
  Result LeastSquares(const SmallMatrix<M, N, T>&   A,
                      const SmallMatrix<M, 1, T>&   b,
                      SmallMatrix<N,1,T>&           x,
                      LeastSquaresMethod            method)
  {
    const bool success = cv::solve(A.get_CvMatx_(), b.get_CvMatx_(), x.get_CvMatx_(),
                                   GetOpenCvSolveMethod(method) | cv::DECOMP_NORMAL);
    if(success) {
      return RESULT_OK;
    } else {
      return RESULT_FAIL;
    }
  }
  
  template<MatDimType M, MatDimType N, typename T>
  Result LeastSquares(const SmallMatrix<M, N, T>&   A,
                      const Point<M,T>&             bIn,
                      Point<N,T>&                   xOut,
                      LeastSquaresMethod            method)
  {
    SmallMatrix<M, 1, T> b{bIn};
    SmallMatrix<N, 1, T> x;
    
    Result result = LeastSquares(A, b, x, method);
    xOut = Point<N,T>(x);
    
    return result;
  }
  
} // namespace Anki

#endif // ANKI_COMMON_BASESTATION_MATH_LINEAR_ALGEBRA_IMPL_H
