#ifndef _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_
#define _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_

#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
  {
    /*! Performs an Singular Value Decomposition on the mXn, float32 input array. [u^t,w,v^t] = SVD(a); */
    Result svd_f32(
      Array_f32 &a, //!< Input array mXn
      Array_f32 &w, //!< W array mXn
      Array_f32 &uT, //!< U-transpose array mXm
      Array_f32 &vT, //!< V-transpose array nXn
      void * scratch //!< A scratch buffer, with at least "sizeof(float)*(n*2 + m)" bytes
      );

    /*! Performs an Singular Value Decomposition on the mXn, float32 input array. [w,u^t,v^t] = SVD(a); */
    void icvLightSVD_32f(
      float* a, //!< Pointer to the upper-left of the input array. Warning: this array will be modified.
      int lda,  //!< A_stride_in_bytes / sizeof(float)
      int m,    //!< Number of rows of A
      int n,    //!< Number of columns of A
      float* w, //!< Pointer to the upper-left of the W array
      float* uT,//!< Pointer to the upper-left of the U-transpose array
      int lduT, //!< U_stride_in_bytes / sizeof(float)
      int nu,   //!< Number of columns of U
      float* vT,//!< Pointer to the upper-left of the V-transpose array
      int ldvT, //!< V_stride_in_bytes / sizeof(float)
      float* buffer //!< A scratch buffer, with at least "sizeof(float)*(n*2 + m)" bytes
      );
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_
