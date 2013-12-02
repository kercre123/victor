/**
File: matrix.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/matrix.h"

#include "anki/common/robot/opencvLight.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Matrix
    {
      Result SolveLeastSquares_f32(const Array<f32> &At, const Array<f32> &bt, Array<f32> &xt, MemoryStack scratch)
      {
        const s32 AWidth = At.get_size(0);

        Array<f32> AtA(AWidth, AWidth, scratch);
        Array<f32> Atb(AWidth, 1, scratch);
        Array<f32> w(1, AWidth, scratch);
        Array<f32> U(AWidth, AWidth, scratch);
        Array<f32> V(AWidth, AWidth, scratch);

        // A' * A = A'A
        if(MultiplyTranspose<f32,f32>(At, At, AtA) != RESULT_OK)
          return RESULT_FAIL;

        // A' * b = A'b
        if(MultiplyTranspose<f32,f32>(At, bt, Atb) != RESULT_OK)
          return RESULT_FAIL;

        const s32 AMinStride = RoundUp<s32>(AWidth, MEMORY_ALIGNMENT);

        {
          PUSH_MEMORY_STACK(scratch);
          void * svdScratch = scratch.Allocate(sizeof(f32)*(AMinStride*3) + 64);
          if(svd_f32(AtA, w, U, V, svdScratch)  != RESULT_OK)
            return RESULT_FAIL;
        } // PUSH_MEMORY_STACK(scratch);

        {
          PUSH_MEMORY_STACK(scratch);
          void * svdScratch = scratch.Allocate(sizeof(f32)*(AMinStride*2) + 64);

          Array<f32> AtbTransposed(1, AWidth, scratch);
          Reshape(true, Atb, AtbTransposed);

          if(svdBackSubstitute_f32(w, V, V, AtbTransposed, xt, svdScratch)  != RESULT_OK)
            return RESULT_FAIL;
        } // PUSH_MEMORY_STACK(scratch);

        return RESULT_OK;
      }

      Result SolveLeastSquares_f64(const Array<f64> &At, const Array<f64> &bt, Array<f64> &xt, MemoryStack scratch)
      {
        const s32 AWidth = At.get_size(0);

        Array<f64> AtA(AWidth, AWidth, scratch);
        Array<f64> Atb(AWidth, 1, scratch);
        Array<f64> w(1, AWidth, scratch);
        Array<f64> U(AWidth, AWidth, scratch);
        Array<f64> V(AWidth, AWidth, scratch);

        // A' * A = A'A
        if(MultiplyTranspose<f64,f64>(At, At, AtA) != RESULT_OK)
          return RESULT_FAIL;

        // A' * b = A'b
        if(MultiplyTranspose<f64,f64>(At, bt, Atb) != RESULT_OK)
          return RESULT_FAIL;

        const s32 AMinStride = RoundUp<s32>(AWidth, MEMORY_ALIGNMENT);

        {
          PUSH_MEMORY_STACK(scratch);
          void * svdScratch = scratch.Allocate(sizeof(f64)*(AMinStride*3) + 64);
          if(svd_f64(AtA, w, U, V, svdScratch)  != RESULT_OK)
            return RESULT_FAIL;
        } // PUSH_MEMORY_STACK(scratch);

        {
          PUSH_MEMORY_STACK(scratch);
          void * svdScratch = scratch.Allocate(sizeof(f64)*(AMinStride*2) + 64);

          Array<f64> AtbTransposed(1, AWidth, scratch);
          Reshape(true, Atb, AtbTransposed);

          if(svdBackSubstitute_f64(w, V, V, AtbTransposed, xt, svdScratch)  != RESULT_OK)
            return RESULT_FAIL;
        } // PUSH_MEMORY_STACK(scratch);

        return RESULT_OK;
      }
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki