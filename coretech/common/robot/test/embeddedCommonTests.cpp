/**
File: embeddedCommonTests.cpp
Author: Peter Barnum
Created: 2013

Various tests of the coretech common embedded library.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

//#define USING_MOVIDIUS_COMPILER

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/benchmarking_c.h"
#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/opencvLight.h"
#include "anki/common/robot/gtestLight.h"
#include "anki/common/robot/matrix.h"
#include "anki/common/robot/comparisons.h"
#include "anki/common/robot/arraySlices.h"
#include "anki/common/robot/find.h"
#include "anki/common/robot/interpolate.h"
#include "anki/common/robot/arrayPatterns.h"

using namespace Anki::Embedded;

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#endif

#if ANKICORETECH_EMBEDDED_USE_MATLAB
#include "anki/common/robot/matlabInterface.h"
Matlab matlab(false);
#endif

#if ANKICORETECH_EMBEDDED_USE_GTEST
#include "gtest/gtest.h"
#endif

#ifdef USING_MOVIDIUS_GCC_COMPILER
#include "swcLeonUtils.h"
#endif

//#define BUFFER_IN_DDR_WITH_L2
#define BUFFER_IN_CMX

#if defined(BUFFER_IN_DDR_WITH_L2) && defined(BUFFER_IN_CMX)
You cannot use both CMX and L2 Cache;
#endif

#define MAX_BYTES 5000

#ifdef _MSC_VER
static char buffer[MAX_BYTES];
#else

#ifdef BUFFER_IN_CMX
static char buffer[MAX_BYTES];    // CMX is default
#else // #ifdef BUFFER_IN_CMX

#ifdef BUFFER_IN_DDR_WITH_L2
__attribute__((section(".ddr.bss,DDR"))) static char buffer[MAX_BYTES]; // With L2 cache
#else
__attribute__((section(".ddr_direct.bss,DDR_DIRECT"))) static char buffer[MAX_BYTES]; // No L2 cache
#endif

#endif // #ifdef BUFFER_IN_CMX ... #else

#endif // #ifdef USING_MOVIDIUS_COMPILER

GTEST_TEST(CoreTech_Common, CholeskyDecomposition)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  const f32 A_data[9] = {
    2.7345f, 1.8859f, 2.0785f,
    1.8859f, 2.2340f, 2.0461f,
    2.0785f, 2.0461f, 2.7591f};

  Array<f32> A(3,3,ms);
  Array<f32> L(3,3,ms);
  Array<f32> bt(1,3,ms);
  Array<f32> xt(1,3,ms);

  ASSERT_TRUE(A.IsValid());
  ASSERT_TRUE(L.IsValid());

  A.Set(A_data, 9);

  const Result result = Matrix::SolveLeastSquaresWithCholesky<f32,f32,f32>(A, bt, L, xt, false);
  ASSERT_TRUE(result == RESULT_OK);

  A.Print("A");
  L.Print("L");

  const f32 L_groundTruth_data[9] = {
    1.6536f, 0.0f, 0.0f,
    1.1405f, 0.9661f, 0.0f,
    1.2569f, 0.6341f, 0.8815f};

  Array<f32> L_groundTruth(3,3,ms);

  L_groundTruth.Set(L_groundTruth_data, 9);

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(L, L_groundTruth, .01, .001));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ExplicitPrintf)
{
  const u32 hex1 = 0x40000000;
  const u32 hex2 = 0x3FF80000;
  const u32 hex3 = 0x3FFD5555;
  const u32 hex4 = 0x40C4507F;

  const f32 *float1 = (f32*)&hex1;
  const f32 *float2 = (f32*)&hex2;
  const f32 *float3 = (f32*)&hex3;
  const f32 *float4 = (f32*)&hex4;

  explicitPrintf(0, EXPLICIT_PRINTF_FLIP_CHARACTERS, "The following two lines should be the same:\n");
  explicitPrintf(0, EXPLICIT_PRINTF_FLIP_CHARACTERS, "%x %x %x %x\n", hex1, hex2, hex3, hex4);
  explicitPrintf(0, EXPLICIT_PRINTF_FLIP_CHARACTERS, "%x %x %x %x\n", *((u32*) float1), *((u32*) float2), *((u32*) float3), *((u32*)float4));

  //explicitPrintf(0, EXPLICIT_PRINTF_FLIP_CHARACTERS, "%f %f %f %f\n", *float1, *float2, *float3, *float4);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MatrixSortWithIndexes)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  const s32 arr_data[15] = {81, 10, 16, 91, 28, 97, 13, 55, 96, 91, 96, 49, 63, 96, 80};

  Array<s32> arr(5,3,ms);
  Array<s32> arrIndexes(5,3,ms);

  Array<s32> sortedArr_groundTruth(5,3,ms);
  Array<s32> sortedArrIndexes_groundTruth(5,3,ms);

  ASSERT_TRUE(arr.IsValid());
  ASSERT_TRUE(sortedArr_groundTruth.IsValid());
  ASSERT_TRUE(sortedArrIndexes_groundTruth.IsValid());

  // sortWhichDimension==0 sortAscending==false
  {
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::Sort(arr, arrIndexes, 0, false) == RESULT_OK);

    //arr.Print("sortWhichDimension==0 sortAscending==false");
    //arrIndexes.Print("Indexes: sortWhichDimension==0 sortAscending==false");

    const s32 sortedArr_groundTruthData[15] = {91, 96, 97, 91, 96, 96, 81, 55, 80, 63, 28, 49, 13, 10, 16};

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);
    ASSERT_TRUE(sortedArr_groundTruth[0][0] == 91);

    const s32 sortedArrIndexes_groundTruthData[15] = {2, 4, 2, 4, 5, 3, 1, 3, 5, 5, 2, 4, 3, 1, 1};
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_groundTruthData, 15) == 15);

    Matrix::Subtract<s32,s32,s32>(sortedArrIndexes_groundTruth, 1, sortedArrIndexes_groundTruth); // Matlab -> C indexing
    //sortedArrIndexes_groundTruth.Print("sortedArrIndexes_groundTruth");

    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));
  }

  // sortWhichDimension==0 sortAscending==true
  {
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::Sort(arr, arrIndexes, 0, true) == RESULT_OK);

    //arr.Print("Values: sortWhichDimension==0 sortAscending==true");
    //arrIndexes.Print("Indexes: sortWhichDimension==0 sortAscending==true");

    const s32 sortedArr_groundTruthData[15] = {13, 10, 16, 63, 28, 49, 81, 55, 80, 91, 96, 96, 91, 96, 97};
    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);

    const s32 sortedArrIndexes_groundTruthData[15] = {3, 1, 1, 5, 2, 4, 1, 3, 5, 2, 4, 3, 4, 5, 2};
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_groundTruthData, 15) == 15);
    Matrix::Subtract<s32,s32,s32>(sortedArrIndexes_groundTruth, 1, sortedArrIndexes_groundTruth); // Matlab -> C indexing

    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==false
  {
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::Sort(arr, arrIndexes, 1, false) == RESULT_OK);

    //arr.Print("sortWhichDimension==1 sortAscending==false");
    //arrIndexes.Print("Indexes: sortWhichDimension==1 sortAscending==false");

    const s32 sortedArr_groundTruthData[15] = {81, 16, 10, 97, 91, 28, 96, 55, 13, 96, 91, 49, 96, 80, 63};
    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);

    const s32 sortedArrIndexes_groundTruthData[15] = {1, 3, 2, 3, 1, 2, 3, 2, 1, 2, 1, 3, 2, 3, 1};
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_groundTruthData, 15) == 15);
    Matrix::Subtract<s32,s32,s32>(sortedArrIndexes_groundTruth, 1, sortedArrIndexes_groundTruth); // Matlab -> C indexing

    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==true
  {
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::Sort(arr, arrIndexes, 1, true) == RESULT_OK);

    //arr.Print("sortWhichDimension==1 sortAscending==true");
    //arrIndexes.Print("Indexes: sortWhichDimension==1 sortAscending==true");

    const s32 sortedArr_groundTruthData[15] = {10, 16, 81, 28, 91, 97, 13, 55, 96, 49, 91, 96, 63, 80, 96};
    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);

    const s32 sortedArrIndexes_groundTruthData[15] = {2, 3, 1, 2, 1, 3, 1, 2, 3, 3, 1, 2, 1, 3, 2};
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_groundTruthData, 15) == 15);
    Matrix::Subtract<s32,s32,s32>(sortedArrIndexes_groundTruth, 1, sortedArrIndexes_groundTruth); // Matlab -> C indexing

    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MatrixSort)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  const s32 arr_data[15] = {81, 10, 16, 91, 28, 97, 13, 55, 96, 91, 96, 49, 63, 96, 80};
  Array<s32> arr(5,3,ms);
  Array<s32> sortedArr_groundTruth(5,3,ms);

  ASSERT_TRUE(arr.IsValid());
  ASSERT_TRUE(sortedArr_groundTruth.IsValid());

  // sortWhichDimension==0 sortAscending==false
  {
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::Sort(arr, 0, false) == RESULT_OK);

    //arr.Print("sortWhichDimension==0 sortAscending==false");

    const s32 sortedArr_groundTruthData[15] = {91, 96, 97, 91, 96, 96, 81, 55, 80, 63, 28, 49, 13, 10, 16};
    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);

    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==0 sortAscending==true
  {
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::Sort(arr, 0, true) == RESULT_OK);

    //arr.Print("sortWhichDimension==0 sortAscending==true");

    const s32 sortedArr_groundTruthData[15] = {13, 10, 16, 63, 28, 49, 81, 55, 80, 91, 96, 96, 91, 96, 97};
    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);

    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==false
  {
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::Sort(arr, 1, false) == RESULT_OK);

    //arr.Print("sortWhichDimension==1 sortAscending==false");

    const s32 sortedArr_groundTruthData[15] = {81, 16, 10, 97, 91, 28, 96, 55, 13, 96, 91, 49, 96, 80, 63};
    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);

    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==true
  {
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::Sort(arr, 1, true) == RESULT_OK);

    //arr.Print("sortWhichDimension==1 sortAscending==true");

    const s32 sortedArr_groundTruthData[15] = {10, 16, 81, 28, 91, 97, 13, 55, 96, 49, 91, 96, 63, 80, 96};
    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);

    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, LinearLeastSquares32)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<f32> At(2,3,ms);
  Array<f32> bt(1,3,ms);
  Array<f32> xt(1,2,ms);

  At[0][0] = 0.814723686393179f; At[1][0] = 0.913375856139019f;
  At[0][1] = 0.905791937075619f; At[1][1] = 0.632359246225410f;
  At[0][2] = 0.126986816293506f; At[1][2] = 0.0975404049994095f;

  bt[0][0] = 0.278498218867048f;
  bt[0][1] = 0.546881519204984f;
  bt[0][2] = 0.957506835434298f;

  ASSERT_TRUE(Matrix::SolveLeastSquaresWithSVD_f32(At, bt, xt, ms) == RESULT_OK);

  Array<f32> xt_groundTruth(1,2,ms);
  xt_groundTruth[0][0] = 1.28959732768902f;
  xt_groundTruth[0][1] = -0.820726191726098f;

  xt.Print("xt");
  xt_groundTruth.Print("xt_groundTruth");

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(xt, xt_groundTruth, .001, .0001));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, LinearLeastSquares64)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<f64> At(2,3,ms);
  Array<f64> bt(1,3,ms);
  Array<f64> xt(1,2,ms);

  At[0][0] = 0.814723686393179; At[1][0] = 0.913375856139019;
  At[0][1] = 0.905791937075619; At[1][1] = 0.632359246225410;
  At[0][2] = 0.126986816293506; At[1][2] = 0.0975404049994095;

  bt[0][0] = 0.278498218867048;
  bt[0][1] = 0.546881519204984;
  bt[0][2] = 0.957506835434298;

  ASSERT_TRUE(Matrix::SolveLeastSquaresWithSVD_f64(At, bt, xt, ms) == RESULT_OK);

  Array<f64> xt_groundTruth(1,2,ms);
  xt_groundTruth[0][0] = 1.28959732768902;
  xt_groundTruth[0][1] = -0.820726191726098;

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(xt, xt_groundTruth, .001, .0001));

  xt.Print("xt");
  xt_groundTruth.Print("xt_groundTruth");

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MatrixMultiplyTranspose)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<s32> in1(2,3,ms);
  Array<s32> in2(2,3,ms);

  in1[0][0] = 1; in1[0][1] = 2; in1[0][2] = 3;
  in1[1][0] = 4; in1[1][1] = 5; in1[1][2] = 6;

  in2[0][0] = 10; in2[0][1] = 11; in2[0][2] = 12;
  in2[1][0] = 13; in2[1][1] = 14; in2[1][2] = 15;

  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> out(2,2,ms);
    const Result result = Matrix::MultiplyTranspose<s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print("out");

    ASSERT_TRUE(out[0][0] == 68);
    ASSERT_TRUE(out[0][1] == 86);
    ASSERT_TRUE(out[1][0] == 167);
    ASSERT_TRUE(out[1][1] == 212);
  }

  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> out(2,2,ms);
    const Result result = Matrix::MultiplyTranspose<s32,s32>(in2, in1, out);
    ASSERT_TRUE(result == RESULT_OK);

    ASSERT_TRUE(out[0][0] == 68);
    ASSERT_TRUE(out[0][1] == 167);
    ASSERT_TRUE(out[1][0] == 86);
    ASSERT_TRUE(out[1][1] == 212);
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Reshape)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<s32> in(2,2,ms);
  in[0][0] = 1; in[0][1] = 2;
  in[1][0] = 3; in[1][1] = 4;

  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> out = Matrix::Reshape<s32,s32>(true, in, 4, 1, ms);

    ASSERT_TRUE(out[0][0] == 1);
    ASSERT_TRUE(out[1][0] == 3);
    ASSERT_TRUE(out[2][0] == 2);
    ASSERT_TRUE(out[3][0] == 4);
  }

  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> out = Matrix::Reshape<s32,s32>(false, in, 4, 1, ms);

    ASSERT_TRUE(out[0][0] == 1);
    ASSERT_TRUE(out[1][0] == 2);
    ASSERT_TRUE(out[2][0] == 3);
    ASSERT_TRUE(out[3][0] == 4);
  }

  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> out = Matrix::Reshape<s32,s32>(true, in, 1, 4, ms);

    ASSERT_TRUE(out[0][0] == 1);
    ASSERT_TRUE(out[0][1] == 3);
    ASSERT_TRUE(out[0][2] == 2);
    ASSERT_TRUE(out[0][3] == 4);
  }

  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> out = Matrix::Reshape<s32,s32>(false, in, 1, 4, ms);

    ASSERT_TRUE(out[0][0] == 1);
    ASSERT_TRUE(out[0][1] == 2);
    ASSERT_TRUE(out[0][2] == 3);
    ASSERT_TRUE(out[0][3] == 4);
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ArrayPatterns)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  const s32 arrayHeight = 3;
  const s32 arrayWidth = 2;

  {
    PUSH_MEMORY_STACK(ms);

    Array<s16> out = Zeros<s16>(arrayHeight, arrayWidth, ms);

    for(s32 y=0; y<arrayHeight; y++) {
      const s16 * const pOut = out.Pointer(y, 0);

      for(s32 x=0; x<arrayWidth; x++) {
        ASSERT_TRUE(pOut[x] == 0);
      }
    }
  }

  {
    PUSH_MEMORY_STACK(ms);

    Array<u8> out = Ones<u8>(arrayHeight, arrayWidth, ms);

    for(s32 y=0; y<arrayHeight; y++) {
      const u8 * const pOut = out.Pointer(y, 0);

      for(s32 x=0; x<arrayWidth; x++) {
        ASSERT_TRUE(pOut[x] == 1);
      }
    }
  }

  {
    PUSH_MEMORY_STACK(ms);
    Array<f64> out = Eye<f64>(arrayHeight, arrayWidth, ms);

    for(s32 y=0; y<arrayHeight; y++) {
      const f64 * const pOut = out.Pointer(y, 0);

      for(s32 x=0; x<arrayWidth; x++) {
        if(x==y) {
          ASSERT_TRUE(FLT_NEAR(pOut[x], 1.0));
        } else {
          ASSERT_TRUE(FLT_NEAR(pOut[x], 0.0));
        }
      }
    }
  }

  {
    PUSH_MEMORY_STACK(ms);

    // [logspace(-3,1,5), 3.14159]
    const f32 inData[6] = {0.001f, 0.01f, 0.1f, 1.0f, 10.0f, 3.14159f};

    Array<f32> expIn(arrayHeight, arrayWidth, ms);
    expIn.Set(inData, 6);

    Array<f32> out = Exp<f32>(expIn, ms);

    const f32 out_groundTruthData[6] = {1.00100050016671f, 1.01005016708417f, 1.10517091807565f, 2.71828182845905f, 22026.4657948067f, 23.1406312269550f};

    Array<f32> out_groundTruth = Array<f32>(3,2,ms);
    out_groundTruth.Set(out_groundTruthData, 6);

    //out.Print("out");
    //out_groundTruth.Print("out_groundTruth");

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(out, out_groundTruth, .05, .001));
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Interp2_twoDimensional)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> reference(3,5,ms);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid(1+(-0.9:0.9:6), 1+(-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));
  Array<f32> xGridMatrix = mesh.EvaluateX2(ms);
  Array<f32> yGridMatrix = mesh.EvaluateY2(ms);

  // result = round(interp2(reference, xGridVector, yGridVector)); result(isnan(result)) = 0
  Array<u8> result(xGridMatrix.get_size(0), xGridMatrix.get_size(1), ms);
  ASSERT_TRUE(Interp2(reference, xGridMatrix, yGridMatrix, result) == RESULT_OK);

  const u8 result_groundTruth[6][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 2, 3, 4, 5, 0, 0},
    {0, 11, 12, 13, 14, 15, 0, 0},
    {0, 21, 22, 23, 24, 25, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}};

  //result.Print("result");

  for(s32 y=0; y<6; y++) {
    for(s32 x=0; x<8; x++) {
      ASSERT_TRUE(result[y][x] == result_groundTruth[y][x]);
    }
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Interp2_oneDimensional)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> reference(3,5,ms);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid(1+(-0.9:0.9:6), 1+(-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));
  Array<f32> xGridVector = mesh.EvaluateX1(true, ms);
  Array<f32> yGridVector = mesh.EvaluateY1(true, ms);

  // result = round(interp2(reference, xGridVector(:), yGridVector(:)))
  Array<u8> result(1, xGridVector.get_size(1), ms);
  ASSERT_TRUE(Interp2(reference, xGridVector, yGridVector, result) == RESULT_OK);

  const u8 result_groundTruth[48] = {0, 0, 0, 0, 0, 0, 0, 1, 11, 21, 0, 0, 0, 2, 12, 22, 0, 0, 0, 3, 13, 23, 0, 0, 0, 4, 14, 24, 0, 0, 0, 5, 15, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  //result.Print("result");

  for(s32 i=0; i<48; i++) {
    ASSERT_TRUE(result[0][i] == result_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Meshgrid_twoDimensional)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  // [x,y] = meshgrid(1:5,1:3)
  Meshgrid<s32> mesh(LinearSequence<s32>(1,5), LinearSequence<s32>(1,3));

  Array<s32> out(20,20,ms);
  ASSERT_TRUE(out.IsValid());

  {
    ASSERT_TRUE(mesh.EvaluateX2(out(3,5,2,6)) == RESULT_OK);
    const s32 out_groundTruth[3][5] = {{1, 2, 3, 4, 5}, {1, 2, 3, 4, 5}, {1, 2, 3, 4, 5}};
    for(s32 y=0; y<3; y++) {
      for(s32 x=0; x<5; x++) {
        ASSERT_TRUE(out[y+3][x+2] == out_groundTruth[y][x]);
      }
    }
  }

  {
    ASSERT_TRUE(mesh.EvaluateY2(out(3,5,2,6)) == RESULT_OK);
    const s32 out_groundTruth[3][5] = {{1, 1, 1, 1, 1}, {2, 2, 2, 2, 2}, {3, 3, 3, 3, 3}};
    for(s32 y=0; y<3; y++) {
      for(s32 x=0; x<5; x++) {
        ASSERT_TRUE(out[y+3][x+2] == out_groundTruth[y][x]);
      }
    }
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Meshgrid_oneDimensional)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  // [x,y] = meshgrid(1:5,1:3)
  Meshgrid<s32> mesh(LinearSequence<s32>(1,5), LinearSequence<s32>(1,3));

  Array<s32> out(20,20,ms);
  ASSERT_TRUE(out.IsValid());

  {
    ASSERT_TRUE(mesh.EvaluateX1(true, out(3,3,2,16)) == RESULT_OK);
    const s32 out_groundTruth[15] = {1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5};
    for(s32 x=0; x<15; x++) {
      ASSERT_TRUE(out[3][x+2] == out_groundTruth[x]);
    }
  }

  {
    ASSERT_TRUE(mesh.EvaluateX1(false, out(3,3,2,16)) == RESULT_OK);
    const s32 out_groundTruth[15] = {1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5};
    for(s32 x=0; x<15; x++) {
      ASSERT_TRUE(out[3][x+2] == out_groundTruth[x]);
    }
  }

  {
    ASSERT_TRUE(mesh.EvaluateY1(true, out(3,3,2,16)) == RESULT_OK);
    const s32 out_groundTruth[15] = {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3};
    for(s32 x=0; x<15; x++) {
      ASSERT_TRUE(out[3][x+2] == out_groundTruth[x]);
    }
  }

  {
    ASSERT_TRUE(mesh.EvaluateY1(false, out(3,3,2,16)) == RESULT_OK);
    const s32 out_groundTruth[15] = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3};
    for(s32 x=0; x<15; x++) {
      ASSERT_TRUE(out[3][x+2] == out_groundTruth[x]);
    }
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Linspace)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  LinearSequence<f32> sequence1 = Linspace<f32>(0,9,10);
  LinearSequence<f32> sequence2 = Linspace<f32>(0,9,1<<29);
  LinearSequence<s32> sequence3 = Linspace<s32>(0,9,10);
  LinearSequence<s32> sequence4 = Linspace<s32>(0,9,9);

  ASSERT_TRUE(sequence1.get_size() == 10);
  ASSERT_TRUE(sequence2.get_size() == 536870912);
  ASSERT_TRUE(sequence3.get_size() == 10);
  ASSERT_TRUE(sequence4.get_size() == 10);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Find_SetArray1)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<s32> in1(1,6,ms); //in1: 2000 2000 2000 3 4 5

  ASSERT_TRUE(in1.IsValid());

  for(s32 x=0; x<6; x++) {
    in1[0][x] = x;
  }

  in1(0,0,0,2).Set(2000);

  Find<s32,Comparison::LessThanOrEqual<s32,s32>,s32> find(in1, 5);

  Array<s16> inB(6,6,ms);

  s16 i1 = 0;
  for(s32 y=0; y<6; y++) {
    for(s32 x=0; x<6; x++) {
      inB[y][x] = i1++;
    }
  }

  // Case 1-1
  {
    PUSH_MEMORY_STACK(ms);

    Array<s16> outB = find.SetArray(inB, 0, ms);
    ASSERT_TRUE(outB.IsValid());

    ASSERT_TRUE(outB.get_size(0) == 3);
    ASSERT_TRUE(outB.get_size(1) == 6);

    for(s32 y=0; y<3; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE(outB[y][x] == inB[y+3][x]);
      }
    }
  } // PUSH_MEMORY_STACK(memory);

  // Case 1-2
  {
    PUSH_MEMORY_STACK(ms);

    Array<s16> outB = find.SetArray(inB, 1, ms);
    ASSERT_TRUE(outB.IsValid());

    ASSERT_TRUE(outB.get_size(0) == 6);
    ASSERT_TRUE(outB.get_size(1) == 3);

    for(s32 y=0; y<6; y++) {
      for(s32 x=0; x<3; x++) {
        ASSERT_TRUE(outB[y][x] == inB[y][x+3]);
      }
    }
  } // PUSH_MEMORY_STACK(memory);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Find_SetArray2)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<f32> in1(1,6,ms);
  Array<f32> in2(1,6,ms);

  ASSERT_TRUE(in1.IsValid());
  ASSERT_TRUE(in2.IsValid());

  for(s32 x=0; x<6; x++) {
    in1[0][x] = float(x) + 0.1f;
    in2[0][x] = float(x)*100.0f;
  }

  in2(0,0,0,2).Set(0.0f);

  in1.Print("in1");
  in2.Print("in2");

  Find<f32,Comparison::LessThanOrEqual<f32,f32>,f32> find(in1, in2);

  Array<s16> inB(6,6,ms);

  {
    s16 i1 = 0;
    for(s32 y=0; y<6; y++) {
      for(s32 x=0; x<6; x++) {
        inB[y][x] = i1++;
      }
    }
  }

  // Case 2-1
  {
    PUSH_MEMORY_STACK(ms);

    Array<s16> outB = find.SetArray(inB, 0, ms);
    ASSERT_TRUE(outB.IsValid());

    ASSERT_TRUE(outB.get_size(0) == 3);
    ASSERT_TRUE(outB.get_size(1) == 6);

    for(s32 y=0; y<3; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE(outB[y][x] == inB[y+3][x]);
      }
    }
  } // PUSH_MEMORY_STACK(memory);

  // Case 2-2
  {
    PUSH_MEMORY_STACK(ms);

    Array<s16> outB = find.SetArray(inB, 1, ms);
    ASSERT_TRUE(outB.IsValid());

    ASSERT_TRUE(outB.get_size(0) == 6);
    ASSERT_TRUE(outB.get_size(1) == 3);

    for(s32 y=0; y<6; y++) {
      for(s32 x=0; x<3; x++) {
        ASSERT_TRUE(outB[y][x] == inB[y][x+3]);
      }
    }
  } // PUSH_MEMORY_STACK(memory);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Find_SetValue)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<u16> in1(5,6,ms);
  Array<u16> in2(5,6,ms);

  ASSERT_TRUE(in1.IsValid());
  ASSERT_TRUE(in2.IsValid());

  //in1.Show("in1", true, true);

  u16 i1 = 0;
  u16 i2 = 0;
  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<6; x++) {
      in1[y][x] = i1++;
      in2[y][x] = i2*100;
      i2++;
    }
  }

  in1(2,-1,0,-1).Set(1600);

  Find<u16,Comparison::Equal<u16,u16>,u16> find(in1, in2);

  ASSERT_TRUE(find.IsValid());

  ASSERT_TRUE(find.SetArray<u16>(in1, 4242) == RESULT_OK);

  const u16 in1_groundTruth[5][6] = {
    {4242, 1, 2, 3, 4, 5},
    {6, 7, 8, 9, 10, 11},
    {1600, 1600, 1600, 1600, 4242, 1600},
    {1600, 1600, 1600, 1600, 1600, 1600},
    {1600, 1600, 1600, 1600, 1600, 1600}};

  //in1.Print("in1");

  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<6; x++) {
      ASSERT_TRUE(in1[y][x] == in1_groundTruth[y][x]);
    }
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ZeroSizedArray)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<s32> in1(1,6,ms);
  Array<s32> in2(1,6,ms);

  ASSERT_TRUE(in1.IsValid());
  ASSERT_TRUE(in2.IsValid());

  for(s32 x=0; x<6; x++) {
    in1[0][x] = x;
    in2[0][x] = x*100;
  }

  // This is always false
  Find<s32,Comparison::GreaterThan<s32,s32>,s32> find(in1, in2);

  Array<s32> yIndexes;
  Array<s32> xIndexes;

  ASSERT_TRUE(find.Evaluate(yIndexes, xIndexes, ms) == RESULT_OK);

  ASSERT_TRUE(!yIndexes.IsValid());
  ASSERT_TRUE(!xIndexes.IsValid());
  ASSERT_TRUE(yIndexes.get_size(0) == 1);
  ASSERT_TRUE(yIndexes.get_size(1) == 0);
  ASSERT_TRUE(xIndexes.get_size(0) == 1);
  ASSERT_TRUE(xIndexes.get_size(1) == 0);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Find_Evaluate1D)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<s32> in1(1,6,ms);
  Array<s32> in2(1,6,ms);

  Array<s32> in1b(6,1,ms);
  Array<s32> in2b(6,1,ms);

  ASSERT_TRUE(in1.IsValid());
  ASSERT_TRUE(in2.IsValid());

  ASSERT_TRUE(in1b.IsValid());
  ASSERT_TRUE(in2b.IsValid());

  for(s32 x=0; x<6; x++) {
    in1[0][x] = x;
    in2[0][x] = x*100;

    in1b[x][0] = x;
    in2b[x][0] = x*100;
  }

  in1(0,0,0,2).Set(2000);
  in1b(0,2,0,0).Set(2000);

  //in1:
  //2000 2000 2000 3 4 5

  //in2:
  //0 100 200 300 400 500

  Find<s32,Comparison::LessThanOrEqual<s32,s32>,s32> find(in1, in2);
  Find<s32,Comparison::LessThanOrEqual<s32,s32>,s32> findB(in1b, in2b);

  ASSERT_TRUE(find.IsValid());

  // 2D indexes horizontal
  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> yIndexes;
    Array<s32> xIndexes;

    ASSERT_TRUE(find.Evaluate(yIndexes, xIndexes, ms) == RESULT_OK);

    ASSERT_TRUE(yIndexes.get_size(0) == 1);
    ASSERT_TRUE(yIndexes.get_size(1) == 3);

    ASSERT_TRUE(xIndexes.get_size(0) == 1);
    ASSERT_TRUE(xIndexes.get_size(1) == 3);

    //yIndexes.Print("yIndexes");
    //xIndexes.Print("xIndexes");

    const s32 yIndexes_groundTruth[3] = {0, 0, 0};
    const s32 xIndexes_groundTruth[3] = {3, 4, 5};

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(yIndexes[0][i] == yIndexes_groundTruth[i]);
      ASSERT_TRUE(xIndexes[0][i] == xIndexes_groundTruth[i]);
    }
  } // PUSH_MEMORY_STACK(ms)

  // 1D indexes horizontal
  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> indexes;

    ASSERT_TRUE(find.Evaluate(indexes, ms) == RESULT_OK);

    ASSERT_TRUE(indexes.get_size(0) == 1);
    ASSERT_TRUE(indexes.get_size(1) == 3);

    //indexes.Print("indexes");

    const s32 indexes_groundTruth[3] = {3, 4, 5};

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(indexes[0][i] == indexes_groundTruth[i]);
    }
  } // PUSH_MEMORY_STACK(ms)

  // 2D indexes vertical
  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> yIndexes;
    Array<s32> xIndexes;

    ASSERT_TRUE(findB.Evaluate(yIndexes, xIndexes, ms) == RESULT_OK);

    ASSERT_TRUE(yIndexes.get_size(0) == 1);
    ASSERT_TRUE(yIndexes.get_size(1) == 3);

    ASSERT_TRUE(xIndexes.get_size(0) == 1);
    ASSERT_TRUE(xIndexes.get_size(1) == 3);

    //yIndexes.Print("yIndexes");
    //xIndexes.Print("xIndexes");

    const s32 xIndexes_groundTruth[3] = {0, 0, 0};
    const s32 yIndexes_groundTruth[3] = {3, 4, 5};

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(yIndexes[0][i] == yIndexes_groundTruth[i]);
      ASSERT_TRUE(xIndexes[0][i] == xIndexes_groundTruth[i]);
    }
  } // PUSH_MEMORY_STACK(ms)

  // 1D indexes vertical
  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> indexes;

    ASSERT_TRUE(findB.Evaluate(indexes, ms) == RESULT_FAIL);
  } // PUSH_MEMORY_STACK(ms)

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Find_Evaluate2D)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<s32> in1(5,6,ms);
  Array<s32> in2(5,6,ms);

  ASSERT_TRUE(in1.IsValid());
  ASSERT_TRUE(in2.IsValid());

  s32 i1 = 0;
  s32 i2 = 0;
  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<6; x++) {
      in1[y][x] = i1++;
      in2[y][x] = i2*100;
      i2++;
    }
  }

  in1(2,-1,0,-1).Set(2000);

  //in1:
  //0 1 2 3 4 5
  //6 7 8 9 10 11
  //2000 2000 2000 2000 2000 2000
  //2000 2000 2000 2000 2000 2000
  //2000 2000 2000 2000 2000 2000

  //in2:
  //0 100 200 300 400 500
  //600 700 800 900 1000 1100
  //1200 1300 1400 1500 1600 1700
  //1800 1900 2000 2100 2200 2300
  //2400 2500 2600 2700 2800 2900

  Find<s32,Comparison::LessThanOrEqual<s32,s32>,s32> find(in1, in2);

  ASSERT_TRUE(find.IsValid());

  Array<s32> yIndexes;
  Array<s32> xIndexes;

  ASSERT_TRUE(find.Evaluate(yIndexes, ms) == RESULT_FAIL);
  ASSERT_TRUE(find.Evaluate(yIndexes, xIndexes, ms) == RESULT_OK);

  ASSERT_TRUE(yIndexes.get_size(0) == 1);
  ASSERT_TRUE(yIndexes.get_size(1) == 22);

  ASSERT_TRUE(xIndexes.get_size(0) == 1);
  ASSERT_TRUE(xIndexes.get_size(1) == 22);

  //yIndexes.Print("yIndexes");
  //xIndexes.Print("xIndexes");

  const s32 yIndexes_groundTruth[22] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4};
  const s32 xIndexes_groundTruth[22] = {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5};

  for(s32 i=0; i<22; i++) {
    ASSERT_TRUE(yIndexes[0][i] == yIndexes_groundTruth[i]);
    ASSERT_TRUE(xIndexes[0][i] == xIndexes_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, Find_NumMatchesAndBoundingRectangle)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<s32> in1(5,6,ms);
  Array<s32> in2(5,6,ms);
  Array<s32> out(5,6,ms);

  ASSERT_TRUE(in1.IsValid());
  ASSERT_TRUE(in2.IsValid());
  ASSERT_TRUE(out.IsValid());

  s32 i1 = 0;
  s32 i2 = 0;
  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<6; x++) {
      in1[y][x] = i1++;
      in2[y][x] = i2*100;
      i2++;
    }
  }

  in1(2,-1,0,-1).Set(2000);

  //in1:
  //0 1 2 3 4 5
  //6 7 8 9 10 11
  //2000 2000 2000 2000 2000 2000
  //2000 2000 2000 2000 2000 2000
  //2000 2000 2000 2000 2000 2000

  //in2:
  //0 100 200 300 400 500
  //600 700 800 900 1000 1100
  //1200 1300 1400 1500 1600 1700
  //1800 1900 2000 2100 2200 2300
  //2400 2500 2600 2700 2800 2900

  Find<s32,Comparison::GreaterThan<s32,s32>,s32> find(in1, in2);

  ASSERT_TRUE(find.IsValid());

  ASSERT_TRUE(find.get_numMatches() == 8);

  ASSERT_TRUE(find.get_limits() == Rectangle<s32>(0,5,2,3));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MatrixElementwise)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<s32> in1(5,6,ms);
  Array<s32> in2(5,6,ms);
  Array<s32> out(5,6,ms);

  ASSERT_TRUE(in1.IsValid());
  ASSERT_TRUE(in2.IsValid());
  ASSERT_TRUE(out.IsValid());

  s32 i1 = 0;
  s32 i2 = 0;
  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<6; x++) {
      in1[y][x] = i1++;
      in2[y][x] = i2*100 + 1;
      i2++;
    }
  }

  // Test normal elementwise addition
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Add<s32,s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(in1[y][x] + in2[y][x]));
      }
    }
  }

  // Test normal elementwise addition with a scalar as parameter 1
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Add<s32,s32,s32>(in1, 5, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(in1[y][x] + 5));
      }
    }
  }

  // Test normal elementwise addition with a scalar as parameter 2
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Add<s32,s32,s32>(-4, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(-4 + in2[y][x]));
      }
    }
  }

  // Test normal elementwise subtraction
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Subtract<s32,s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(in1[y][x] - in2[y][x]));
      }
    }
  }

  // Test normal elementwise subtraction with a scalar as parameter 1
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Subtract<s32,s32,s32>(100, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(100 - in2[y][x]));
      }
    }
  }

  // Test normal elementwise subtraction with a scalar as parameter 2
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Subtract<s32,s32,s32>(in1, 1, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(in1[y][x] - 1));
      }
    }
  }

  // Test normal elementwise multiplication
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::DotMultiply<s32,s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(in1[y][x] * in2[y][x]));
      }
    }
  }

  // Test normal elementwise multiplication with a scalar as parameter 1
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::DotMultiply<s32,s32,s32>(in1, 2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(in1[y][x] * 2));
      }
    }
  }

  // Test normal elementwise multiplication with a scalar as parameter 2
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::DotMultiply<s32,s32,s32>(-2, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)((-2) * in2[y][x]));
      }
    }
  }

  // Test normal elementwise division
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::DotDivide<s32,s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(in1[y][x] / in2[y][x]));
      }
    }
  }

  // Test normal elementwise division with a scalar as parameter 1
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::DotDivide<s32,s32,s32>(in1, 2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(in1[y][x] / 2));
      }
    }
  }

  // Test normal elementwise division with a scalar as parameter 2
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::DotDivide<s32,s32,s32>(10, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE((s32)out[y][x] == (s32)(10 / in2[y][x]));
      }
    }
  }

  // Test slice transpose in1 elementwise addition
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Add<s32,s32,s32>(in1(0,2,4,0,2,0).Transpose(), in2(0,1,0,0,2,4), out(0,1,0,0,2,4));
    ASSERT_TRUE(result == RESULT_OK);

    ASSERT_TRUE((s32)out[0][0] == (s32)(in1[0][0] + in2[0][0]));
    ASSERT_TRUE((s32)out[0][2] == (s32)(in1[2][0] + in2[0][2]));
    ASSERT_TRUE((s32)out[0][4] == (s32)(in1[4][0] + in2[0][4]));

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        if(!(y==0 && (x==0 || x==2 || x==4))) {
          ASSERT_TRUE(out[y][x] == 0);
        }
      }
    }
  }

  // Test slice transpose in2 elementwise addition
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Add<s32,s32,s32>(in1(0,1,0,0,2,4), in2(0,2,4,0,2,0).Transpose(), out(0,1,0,0,2,4));
    ASSERT_TRUE(result == RESULT_OK);

    ASSERT_TRUE((s32)out[0][0] == (s32)(in1[0][0] + in2[0][0]));
    ASSERT_TRUE((s32)out[0][2] == (s32)(in1[0][2] + in2[2][0]));
    ASSERT_TRUE((s32)out[0][4] == (s32)(in1[0][4] + in2[4][0]));

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        if(!(y==0 && (x==0 || x==2 || x==4))) {
          ASSERT_TRUE(out[y][x] == 0);
        }
      }
    }
  }

  // Test slice transpose in1 and in2 elementwise addition
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Add<s32,s32,s32>(in1(0,2,4,0,2,0).Transpose(), in2(0,2,4,0,2,0).Transpose(), out(0,1,0,0,2,4));
    ASSERT_TRUE(result == RESULT_OK);

    ASSERT_TRUE((s32)out[0][0] == (s32)(in1[0][0] + in2[0][0]));
    ASSERT_TRUE((s32)out[0][2] == (s32)(in1[2][0] + in2[2][0]));
    ASSERT_TRUE((s32)out[0][4] == (s32)(in1[4][0] + in2[4][0]));

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        if(!(y==0 && (x==0 || x==2 || x==4))) {
          ASSERT_TRUE(out[y][x] == 0);
        }
      }
    }
  }

  // Test slice transpose in1 and in2 elementwise subtraction
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::Subtract<s32,s32,s32>(in1(0,2,4,0,2,0).Transpose(), in2(0,2,4,0,2,0).Transpose(), out(0,1,0,0,2,4));
    ASSERT_TRUE(result == RESULT_OK);

    ASSERT_TRUE((s32)out[0][0] == (s32)(in1[0][0] - in2[0][0]));
    ASSERT_TRUE((s32)out[0][2] == (s32)(in1[2][0] - in2[2][0]));
    ASSERT_TRUE((s32)out[0][4] == (s32)(in1[4][0] - in2[4][0]));

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        if(!(y==0 && (x==0 || x==2 || x==4))) {
          ASSERT_TRUE(out[y][x] == 0);
        }
      }
    }
  }

  // Test slice transpose in1 and in2 elementwise multiply
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::DotMultiply<s32,s32,s32>(in1(0,2,4,0,2,0).Transpose(), in2(0,2,4,0,2,0).Transpose(), out(0,1,0,0,2,4));
    ASSERT_TRUE(result == RESULT_OK);

    ASSERT_TRUE((s32)out[0][0] == (s32)(in1[0][0] * in2[0][0]));
    ASSERT_TRUE((s32)out[0][2] == (s32)(in1[2][0] * in2[2][0]));
    ASSERT_TRUE((s32)out[0][4] == (s32)(in1[4][0] * in2[4][0]));

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        if(!(y==0 && (x==0 || x==2 || x==4))) {
          ASSERT_TRUE(out[y][x] == 0);
        }
      }
    }
  }

  // Test slice transpose in1 and in2 elementwise division
  {
    ASSERT_TRUE(out.SetZero() != 0);
    const Result result = Matrix::DotDivide<s32,s32,s32>(in1(0,2,4,0,2,0).Transpose(), in2(0,2,4,0,2,0).Transpose(), out(0,1,0,0,2,4));
    ASSERT_TRUE(result == RESULT_OK);

    ASSERT_TRUE((s32)out[0][0] == (s32)(in1[0][0] / in2[0][0]));
    ASSERT_TRUE((s32)out[0][2] == (s32)(in1[2][0] / in2[2][0]));
    ASSERT_TRUE((s32)out[0][4] == (s32)(in1[4][0] / in2[4][0]));

    for(s32 y=0; y<5; y++) {
      for(s32 x=0; x<6; x++) {
        if(!(y==0 && (x==0 || x==2 || x==4))) {
          ASSERT_TRUE(out[y][x] == 0);
        }
      }
    }
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, SliceArrayAssignment)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> array1(5,6,ms);
  Array<u8> array2(5,6,ms);

  ASSERT_TRUE(array1.IsValid());
  ASSERT_TRUE(array2.IsValid());

  s32 i = 0;
  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<6; x++) {
      array2[y][x] = i++;
    }
  }

  // Test the non-transposed Set()
  ASSERT_TRUE(array1.SetZero() != 0);
  ASSERT_TRUE(array1(0,2,0,3).Set(array2(1,3,1,4)) == 12);

  //array1.Print("array1");
  //array2.Print("array2");

  for(s32 y=0; y<=2; y++) {
    for(s32 x=0; x<=3; x++) {
      ASSERT_TRUE(array1[y][x] == array2[y+1][x+1]);
    }
    for(s32 x=4; x<6; x++) {
      ASSERT_TRUE(array1[y][x] == 0);
    }
  }
  for(s32 y=3; y<5; y++) {
    for(s32 x=0; x<6; x++) {
      ASSERT_TRUE(array1[y][x] == 0);
    }
  }

  // Test the automatically transposed Set()
  ASSERT_TRUE(array1.SetZero() != 0);
  ASSERT_TRUE(array1(0,-1,0,2).Set(array2(1,3,0,4)) == 15);

  //array1.Print("array1");
  //array2.Print("array2");

  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<=2; x++) {
      ASSERT_TRUE(array1[y][x] == array2[x+1][y]);
    }
    for(s32 x=3; x<6; x++) {
      ASSERT_TRUE(array1[y][x] == 0);
    }
  }

  // Test for a failure in the manually transposed Set()
  ASSERT_TRUE(array1.SetZero() != 0);
  ASSERT_TRUE(array1(0,-1,0,2).Set(array2(1,3,0,4), false) == 0);

  // Test the manually transposed Set()
  ASSERT_TRUE(array1.SetZero() != 0);
  ASSERT_TRUE(array1(0,-1,0,2).Set(array2(1,3,0,4).Transpose(), false) == 15);

  //array1.Print("array1");
  //array2.Print("array2");

  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<=2; x++) {
      ASSERT_TRUE(array1[y][x] == array2[x+1][y]);
    }
    for(s32 x=3; x<6; x++) {
      ASSERT_TRUE(array1[y][x] == 0);
    }
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, SliceArrayCompileTest)
{
  // This is just a compile test, and should always pass unless there's a very serious error
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> array1(20,30,ms);

  // This is okay
  ArraySlice<u8> slice1 = array1(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  const Array<u8> array2(20,30,ms);

  // Will not compile
  //ArraySlice<u8> slice2 = array2(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  // This is okay
  ConstArraySlice<u8> slice1b = array1(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  // This is okay
  ConstArraySlice<u8> slice2 = array2(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  //printf("%d %d %d\n", slice1.get_xSlice().get_start(), slice1.get_xSlice().get_end(), *slice1.get_array().Pointer(0,0));
  //printf("%d %d %d\n", slice1b.get_xSlice().get_start(), slice1b.get_xSlice().get_end(), *slice1b.get_array().Pointer(0,0));
  //printf("%d %d %d\n", slice2.get_xSlice().get_start(), slice2.get_xSlice().get_end(), *slice2.get_array().Pointer(0,0));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MatrixMinAndMaxAndSum)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> array(5,5,ms);

  s32 i = 0;
  for(s32 y=0; y<5; y++) {
    for(s32 x=0; x<5; x++) {
      array[y][x] = i++;
    }
  }

  const s32 minValue1 = Matrix::Min<u8>(array);
  const s32 minValue2 = Matrix::Min<u8>(array(2,-1,2,-1));
  const s32 minValue3 = Matrix::Min<u8>(array(2,-1,-1,-1));

  const s32 maxValue1 = Matrix::Max<u8>(array);
  const s32 maxValue2 = Matrix::Max<u8>(array(0,-3,0,-1));
  const s32 maxValue3 = Matrix::Max<u8>(array(0,-1,-2,-2));

  const s32 sum1 = Matrix::Sum<u8,s32>(array);
  const s32 sum2 = Matrix::Sum<u8,s32>(array(0,-3,0,-1));
  const s32 sum3 = Matrix::Sum<u8,s32>(array(0,-1,-2,-2));

  ASSERT_TRUE(minValue1 == 0);
  ASSERT_TRUE(minValue2 == 12);
  ASSERT_TRUE(minValue3 == 14);

  ASSERT_TRUE(maxValue1 == 24);
  ASSERT_TRUE(maxValue2 == 14);
  ASSERT_TRUE(maxValue3 == 23);

  ASSERT_TRUE(sum1 == 300);
  ASSERT_TRUE(sum2 == 105);
  ASSERT_TRUE(sum3 == 65);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ReallocateArray)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> array1(20,30,ms);
  Array<u8> array2(20,30,ms);
  Array<u8> array3(20,30,ms);

  ASSERT_TRUE(array1.IsValid());
  ASSERT_TRUE(array2.IsValid());
  ASSERT_TRUE(array3.IsValid());

  ASSERT_TRUE(array1.IsValid());
  ASSERT_TRUE(array1.Resize(20,15,ms) == RESULT_FAIL);
  ASSERT_TRUE(!array1.IsValid());

  ASSERT_TRUE(array3.IsValid());
  ASSERT_TRUE(array3.Resize(20,15,ms) == RESULT_OK);
  ASSERT_TRUE(array3.IsValid());

  ASSERT_TRUE(array2.IsValid());
  ASSERT_TRUE(array2.Resize(20,15,ms) == RESULT_FAIL);
  ASSERT_TRUE(!array2.IsValid());

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ReallocateMemoryStack)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  void *memory1 = ms.Allocate(100);
  void *memory2 = ms.Allocate(100);
  void *memory3 = ms.Allocate(100);

  ASSERT_TRUE(memory3 != NULL);
  memory3 = ms.Reallocate(memory3, 50);
  ASSERT_TRUE(memory3 != NULL);

  ASSERT_TRUE(memory2 != NULL);
  memory2 = ms.Reallocate(memory2, 50);
  ASSERT_TRUE(memory2 == NULL);

  ASSERT_TRUE(memory1 != NULL);
  memory1 = ms.Reallocate(memory1, 50);
  ASSERT_TRUE(memory1 == NULL);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, LinearSequence)
{
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, MAX_BYTES);
  ASSERT_TRUE(ms.IsValid());

  // Test s32 sequences
  const s32 sequenceLimits1[15][3] = {{0,1,1}, {0,2,1}, {-1,1,1}, {-1,2,1}, {-1,3,1},
  {-50,2,-3}, {-3,3,-50}, {10,-4,1}, {10,3,1}, {-10,3,1},
  {-7,7,0}, {12,-4,-10}, {100,-1,0}, {0,3,100}, {0,2,10000}};

  // [length(0:1:1), length(0:2:1), length(-1:1:1), length(-1:2:1), length(-1:3:1), length(-50:2:-3), length(-3:3:-50), length(10:-4:1), length(10:3:1), length(-10:3:1), length(-7:7:0), length(12:-4:-10), length(100:-1:0), length(0:3:100), length(0:2:10000)]'
  const s32 length1_groundTruth[15] = {2, 1, 3, 2, 1, 24, 0, 3, 0, 4, 2, 6, 101, 34, 5001};

  for(s32 i=0; i<15; i++) {
    LinearSequence<s32> sequence(sequenceLimits1[i][0], sequenceLimits1[i][1], sequenceLimits1[i][2]);
    //printf("A %d) %d %d\n", i, sequence.get_size(), length1_groundTruth[i]);
    ASSERT_TRUE(sequence.get_size() == length1_groundTruth[i]);
  }

  // Tests f32 sequences
  const f32 sequenceLimits2[25][3] = {{0.0f,1.0f,1.0f}, {0.0f,2.0f,1.0f}, {-1.0f,1.0f,1.0f}, {-1.0f,2.0f,1.0f}, {-1.0f,3.0f,1.0f},
  {-50.0f,2.0f,-3.0f}, {-3.0f,3.0f,-50.0f}, {10.0f,-4.0f,1.0f}, {10.0f,3.0f,1.0f}, {-10.0f,3.0f,1.0f},
  {-7.0f,7.0f,0.0f}, {12.0f,-4.0f,-10.0f}, {100.0f,-1.0f,0.0f}, {0.0f,3.0f,100.0f}, {0.0f,2.0f,10000.0f},
  {0.1f,0.1f,0.1f}, {0.1f,0.11f,0.3f}, {0.1f,0.09f,0.3f}, {-0.5f,0.1f,0.5f}, {0.3f,0.1f,0.2f},
  {-0.5f,-0.05f,-0.4f}, {-0.5f,0.6f,0.0f}, {-0.5f,0.25f,0.0f}, {-4.0f,0.01f,100.0f}, {0.1f,-0.1f,-0.4f}};

  // [length(0.1:0.1:0.1), length(0.1:0.11:0.3), length(0.1:0.09:0.3), length(-0.5:0.1:0.5), length(0.3:0.1:0.2), length(-0.5:-0.05:-0.4), length(-0.5:0.6:0.0), length(-0.5:0.25:0.0), length(-4.0:0.01:100.0), length(0.1:-0.1:-0.4)]'
  const s32 length2_groundTruth[25] = {2, 1, 3, 2, 1, 24, 0, 3, 0, 4, 2, 6, 101, 34, 5001, 1, 2, 3, 11, 0, 0, 1, 3, 10401, 6};

  for(s32 i=0; i<25; i++) {
    LinearSequence<f32> sequence(sequenceLimits2[i][0], sequenceLimits2[i][1], sequenceLimits2[i][2]);
    //printf("B %d) %d %d\n", i, sequence.get_size(), length2_groundTruth[i]);
    ASSERT_TRUE(sequence.get_size() == length2_groundTruth[i]);
  }

  // Test sequence assignment to an Array
  const LinearSequence<s32> sequence(-4,2,4);

  // Test a normal Array
  {
    PUSH_MEMORY_STACK(ms);
    Array<s32> sequenceArray = sequence.Evaluate(ms);

    ASSERT_TRUE(sequenceArray.IsValid());

    const s32 sequenceArray_groundTruth[5] = {-4, -2, 0, 2, 4};

    ASSERT_TRUE(sequenceArray.get_size(0) == 1);
    ASSERT_TRUE(sequenceArray.get_size(1) == 5);

    for(s32 i=0; i<5; i++) {
      ASSERT_TRUE(sequenceArray[0][i] == sequenceArray_groundTruth[i]);
    }
  } // PUSH_MEMORY_STACK(ms)

  // Test an ArraySlice
  {
    PUSH_MEMORY_STACK(ms);

    Array<s32> sequenceArray(4,100,ms);

    ASSERT_TRUE(sequenceArray.IsValid());

    ASSERT_TRUE(sequence.Evaluate(sequenceArray(2,2,4,8)) == RESULT_OK);

    const s32 sequenceArray_groundTruth[5] = {-4, -2, 0, 2, 4};

    for(s32 i=0; i<=3; i++) {
      ASSERT_TRUE(sequenceArray[2][i] == 0);
    }

    for(s32 i=4; i<=8; i++) {
      ASSERT_TRUE(sequenceArray[2][i] == sequenceArray_groundTruth[i-4]);
    }

    for(s32 i=9; i<100; i++) {
      ASSERT_TRUE(sequenceArray[2][i] == 0);
    }

    for(s32 x=0; x<100; x++) {
      for(s32 y=0; y<=1; y++) {
        ASSERT_TRUE(sequenceArray[y][x] == 0);
      }
      for(s32 y=3; y<=3; y++) {
        ASSERT_TRUE(sequenceArray[y][x] == 0);
      }
    }
  } // PUSH_MEMORY_STACK(ms)

  GTEST_RETURN_HERE;
}

// This test requires a stopwatch, and takes about ten seconds to do manually
//#define TEST_BENCHMARKING
#ifdef TEST_BENCHMARKING
GTEST_TEST(CoreTech_Common, Benchmarking)
{
  InitBenchmarking();

  BeginBenchmark("testOuter");

  const double startTime0 = GetTime();
  while((GetTime() - startTime0) < 1.0) {}

  BeginBenchmark("testInner");
  const double startTime1 = GetTime();
  while((GetTime() - startTime1) < 1.0) {}
  EndBenchmark("testInner");

  BeginBenchmark("testInner");
  const double startTime2 = GetTime();
  while((GetTime() - startTime2) < 2.0) {}
  EndBenchmark("testInner");

  BeginBenchmark("testInner");
  const double startTime3 = GetTime();
  while((GetTime() - startTime3) < 3.0) {}
  EndBenchmark("testInner");

  EndBenchmark("testOuter");

  PrintBenchmarkResults();

  printf("Done with benchmarking test\n");

  GTEST_RETURN_HERE;
}
#endif //#ifdef TEST_BENCHMARKING

GTEST_TEST(CoreTech_Common, MemoryStackId)
{
  //printf("%f %f %f %f %f\n", 43423442334324.010203, 15.500, 15.0, 0.05, 0.12004333);

  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

  MemoryStack ms1(buffer, 50);
  MemoryStack ms2(buffer+50, 50);
  MemoryStack ms2b(ms2);
  MemoryStack ms2c(ms2b);
  MemoryStack ms3(buffer+50*2, 50);
  MemoryStack ms4(buffer+50*3, 50);
  MemoryStack ms5(buffer+50*4, 50);

  const s32 id1 = ms1.get_id();
  const s32 id2 = ms2.get_id() - id1;
  const s32 id2b = ms2b.get_id() - id1;
  const s32 id2c = ms2c.get_id() - id1;
  const s32 id3 = ms3.get_id() - id1;
  const s32 id4 = ms4.get_id() - id1;
  const s32 id5 = ms5.get_id() - id1;

  ASSERT_TRUE(id2 == 1);
  ASSERT_TRUE(id2b == 1);
  ASSERT_TRUE(id2c == 1);
  ASSERT_TRUE(id3 == 2);
  ASSERT_TRUE(id4 == 3);
  ASSERT_TRUE(id5 == 4);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ApproximateExp)
{
  // To compute the ground truth, in Matlab, type:
  // x = logspace(-5, 5, 11); yPos = exp(x); yNeg = exp(-x);

  // 0.00001, 0.0001, 0.001, 0.01, 0.1, 1, 10
  // 1.00001000005000, 1.00010000500017, 1.00100050016671, 1.01005016708417, 1.10517091807565, 2.71828182845905, 22026.4657948067
  // 0.999990000050000	0.999900004999833	0.999000499833375	0.990049833749168	0.904837418035960	0.367879441171442	.0000453999297624849

  const f64 valP0 = approximateExp(0.00001);
  const f64 valP1 = approximateExp(0.0001);
  const f64 valP2 = approximateExp(0.001);
  const f64 valP3 = approximateExp(0.01);
  const f64 valP4 = approximateExp(0.1);
  const f64 valP5 = approximateExp(0.0);
  const f64 valP6 = approximateExp(1.0);

  const f64 valN0 = approximateExp(-0.00001);
  const f64 valN1 = approximateExp(-0.0001);
  const f64 valN2 = approximateExp(-0.001);
  const f64 valN3 = approximateExp(-0.01);
  const f64 valN4 = approximateExp(-0.1);
  const f64 valN5 = approximateExp(-1.0);

  ASSERT_TRUE(ABS(valP0 - 1.00001000005000) < .0001);
  ASSERT_TRUE(ABS(valP1 - 1.00010000500017) < .0001);
  ASSERT_TRUE(ABS(valP2 - 1.00100050016671) < .0001);
  ASSERT_TRUE(ABS(valP3 - 1.01005016708417) < .0001);
  ASSERT_TRUE(ABS(valP4 - 1.10517091807565) < .0001);
  ASSERT_TRUE(ABS(valP5 - 1.0) < .0001);
  ASSERT_TRUE(ABS(valP6 - 2.71828182845905) < .0001);

  ASSERT_TRUE(ABS(valN0 - 0.999990000050000) < .0001);
  ASSERT_TRUE(ABS(valN1 - 0.999900004999833) < .0001);
  ASSERT_TRUE(ABS(valN2 - 0.999000499833375) < .0001);
  ASSERT_TRUE(ABS(valN3 - 0.990049833749168) < .0001);
  ASSERT_TRUE(ABS(valN4 - 0.904837418035960) < .0001);
  ASSERT_TRUE(ABS(valN5 - 0.367879441171442) < .0001);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MatrixMultiply)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

#define MatrixMultiply_mat1DataLength 10
#define MatrixMultiply_mat2DataLength 15
#define MatrixMultiply_matOutGroundTruthDataLength 6

  const f64 mat1Data[MatrixMultiply_mat1DataLength] = {
    1, 2, 3, 5, 7,
    11, 13, 17, 19, 23};

  const f64 mat2Data[MatrixMultiply_mat2DataLength] = {
    29, 31, 37,
    41, 43, 47,
    53, 59, 61,
    67, 71, 73,
    79, 83, 89};

  const f64 matOutGroundTruthData[MatrixMultiply_matOutGroundTruthDataLength] = {
    1158, 1230, 1302,
    4843, 5161, 5489};

  Array<f64> mat1(2, 5, ms);
  Array<f64> mat2(5, 3, ms);
  Array<f64> matOut(2, 3, ms);
  Array<f64> matOut_groundTruth(2, 3, ms);

  ASSERT_TRUE(mat1.IsValid());
  ASSERT_TRUE(mat2.IsValid());
  ASSERT_TRUE(matOut.IsValid());
  ASSERT_TRUE(matOut_groundTruth.IsValid());

  mat1.Set(mat1Data, MatrixMultiply_mat1DataLength);
  mat2.Set(mat2Data, MatrixMultiply_mat2DataLength);
  matOut_groundTruth.Set(matOutGroundTruthData, MatrixMultiply_matOutGroundTruthDataLength);

  const Result result = Matrix::Multiply<f64>(mat1, mat2, matOut);

  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(AreElementwiseEqual(matOut, matOut_groundTruth));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ComputeHomography)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

  Array<f64> homography_groundTruth(3, 3, ms);
  Array<f64> homography(3, 3, ms);
  Array<f64> originalPoints(3, 4, ms);
  Array<f64> transformedPoints(3, 4, ms);

  FixedLengthList<Point<f64> > originalPointsList(4, ms);
  FixedLengthList<Point<f64> > transformedPointsList(4, ms);

  ASSERT_TRUE(homography_groundTruth.IsValid());
  ASSERT_TRUE(homography.IsValid());
  ASSERT_TRUE(originalPoints.IsValid());
  ASSERT_TRUE(originalPointsList.IsValid());
  ASSERT_TRUE(transformedPointsList.IsValid());

#define ComputeHomography_homographyGroundTruthDataLength 9
#define ComputeHomography_originalPointsDataLength 12

  const f64 homographyGroundTruthData[ComputeHomography_homographyGroundTruthDataLength] = {
    5.5, -0.3, 5.5,
    0.5, 0.5, 3.3,
    0.001, 0.0, 1.0};

  const f64 originalPointsData[ComputeHomography_originalPointsDataLength] = {
    0, 1, 1, 0,
    0, 0, 1, 1,
    1, 1, 1, 1};

  homography_groundTruth.Set(homographyGroundTruthData, ComputeHomography_homographyGroundTruthDataLength);
  originalPoints.Set(originalPointsData, ComputeHomography_originalPointsDataLength);

  Matrix::Multiply<f64>(homography_groundTruth, originalPoints, transformedPoints);

  for(s32 i=0; i<originalPoints.get_size(1); i++) {
    const f64 x0 = (*originalPoints.Pointer(0,i)) / (*originalPoints.Pointer(2,i));
    const f64 y0 = (*originalPoints.Pointer(1,i)) / (*originalPoints.Pointer(2,i));

    const f64 x1 = (*transformedPoints.Pointer(0,i)) / (*transformedPoints.Pointer(2,i));
    const f64 y1 = (*transformedPoints.Pointer(1,i)) / (*transformedPoints.Pointer(2,i));

    originalPointsList.PushBack(Point<f64>(x0, y0));
    transformedPointsList.PushBack(Point<f64>(x1, y1));
  }

  originalPoints.Print("originalPoints");
  transformedPoints.Print("transformedPoints");

  originalPointsList.Print("originalPointsList");
  transformedPointsList.Print("transformedPointsList");

  const Result result = EstimateHomography(originalPointsList, transformedPointsList, homography, ms);

  ASSERT_TRUE(result == RESULT_OK);

  homography.Print("homography");

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(homography, homography_groundTruth, .01, .01));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, SVD32)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

#define SVD32_aDataLength 16
#define SVD32_uTGroundTruthDataLength 4
#define SVD32_wGroundTruthDataLength 8
#define SVD32_vTGroundTruthDataLength 64

  const f32 uTGroundTruthData[] = {
    -0.237504316543999f,  -0.971386483137875f,
    0.971386483137874f,  -0.237504316543999f};

  // This is not always initializing correctly on Leon. Why?
  const f32 aData[] = {
    1.0f, 2.0f, 3.0f, 5.0f, 7.0f, 11.0f, 13.0f, 17.0f,
    19.0f, 23.0f, 29.0f, 31.0f, 37.0f, 41.0f, 43.0f, 47.0f};

  const f32 wGroundTruthData[] = {
    101.885662808124f, 9.29040979446927f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

  const f32 vTGroundTruthData[] = {
    -0.183478685625953f, -0.223946108965585f, -0.283481700610061f, -0.307212042374800f, -0.369078720749224f, -0.416539404278702f, -0.440269746043441f, -0.487730429572919f,
    0.381166774075590f, 0.378866636898153f, 0.427695421221120f, 0.269708382365037f, 0.213979186509760f, -0.101994891202405f, -0.259981930058488f, -0.575956007770654f,
    -0.227185052857744f, -0.469620636740071f, 0.828412170627898f, -0.133259849703043f, -0.130174916014859f, -0.0535189566767408f, -0.0151909770076815f, 0.0614649823304370f,
    -0.262651399482480f, -0.301123351378262f, -0.120288604832971f, 0.894501576357598f, -0.112627300393190f, -0.0830469380120526f, -0.0682567568214837f, -0.0386763944403458f,
    -0.324884751171929f, -0.243759383675343f, -0.107225790475501f, -0.104644995857761f, 0.880843955313500f, -0.113994455451022f, -0.111413660833282f, -0.106252071597804f,
    -0.395817444421400f, 0.0932351870482750f, -0.00462734139723796f, -0.0491221437364792f, -0.0840608134431630f, 0.826949581878355f, -0.217545220460887f, -0.306534825139369f,
    -0.431283791046136f, 0.261732472410084f, 0.0466718831418935f, -0.0213607176758380f, -0.0665131978214941f, -0.202578399456957f, 0.729388999725311f, -0.406676201910152f,
    -0.502216484295607f, 0.598727043133703f, 0.149270332220156f, 0.0341621344454443f, -0.0314179665781565f, -0.261634362127581f, -0.376742559902293f, 0.393041044548282f};

  const s32 m = 2, n = 8;

#ifdef USING_MOVIDIUS_GCC_COMPILER
  swcLeonFlushCaches();
#endif

  Array<f32> a(m, n, ms);
  Array<f32> w(1, n, ms);
  Array<f32> uT(m, m, ms);
  Array<f32> vT(n, n, ms);
  Array<f32> w_groundTruth(1, n, ms);
  Array<f32> uT_groundTruth(m, m, ms);
  Array<f32> vT_groundTruth(n, n, ms);

  ASSERT_TRUE(a.IsValid());
  ASSERT_TRUE(w.IsValid());
  ASSERT_TRUE(uT.IsValid());
  ASSERT_TRUE(vT.IsValid());
  ASSERT_TRUE(w_groundTruth.IsValid());
  ASSERT_TRUE(uT_groundTruth.IsValid());
  ASSERT_TRUE(vT_groundTruth.IsValid());

  void * scratch = ms.Allocate(sizeof(float)*(2*n + 2*m + 64));
  //void * scratch = ms.Allocate(2*32 + 2*32 + 64);
  ASSERT_TRUE(scratch != NULL);

  //swcLeonFlushDcache();
  //swcLeonDisableCaches();
  /*swcLeonFlushCaches();
  while(swcLeonIsCacheFlushPending()) { printf("a");}*/

  a.Set(&aData[0], SVD32_aDataLength);

  w_groundTruth.Set(&wGroundTruthData[0], SVD32_uTGroundTruthDataLength);

  uT_groundTruth.Set(&uTGroundTruthData[0], SVD32_wGroundTruthDataLength);
  vT_groundTruth.Set(&vTGroundTruthData[0], SVD32_vTGroundTruthDataLength);

  /*a[0][0] = 1.0f; a[0][1] = 2.0f; a[0][2] = 3.0f; a[0][3] = 5.0f; a[0][4] = 7.0f; a[0][5] = 11.0f; a[0][6] = 13.0f; a[0][7] = 17.0f;
  a[1][0] = 19.0f; a[1][1] = 23.0f; a[1][2] = 29.0f; a[1][3] = 31.0f; a[1][4] = 37.0f; a[1][5] = 41.0f; a[1][6] = 43.0f; a[1][7] = 47.0f;*/

  const Result result = svd_f32(a, w, uT, vT, scratch);

  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(ms.IsValid());

  //PrintfOneArray_f32(a, "a");

  //PrintfOneArray_f32(w, "w   ");
  //PrintfOneArray_f32(w_groundTruth, "w_groundTruth");

  PrintfOneArray_f32(uT, "uT");
  PrintfOneArray_f32(uT_groundTruth, "uT_groundTruth");

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(w, w_groundTruth, .05, .001));
  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(uT, uT_groundTruth, .05, .001));

  // I don't know why, but the v-transpose for this SVD doesn't match Matlab's. Probably this version is more efficient, in either memory or computation.
  //ASSERT_TRUE(vT.IsElementwiseEqual_PercentThreshold(vT_groundTruth, .05, .001));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, SVD64)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

#define SVD64_aDataLength 16
#define SVD64_uTGroundTruthDataLength 4
#define SVD64_wGroundTruthDataLength 8
#define SVD64_vTGroundTruthDataLength 64

  // This is not initializing correctly on Leon. Why?
  //const f64 aData[SVD64_aDataLength] = {
  //  1, 2, 3, 5, 7, 11, 13, 17,
  //  19, 23, 29, 31, 37, 41, 43, 47};

  const f64 uTGroundTruthData[] = {
    -0.237504316543999,  -0.971386483137875,
    0.971386483137874,  -0.237504316543999};

  const f64 wGroundTruthData[] = {
    101.885662808124, 9.29040979446927, 0, 0, 0, 0, 0, 0};

  const f64 vTGroundTruthData[] = {
    -0.183478685625953, -0.223946108965585, -0.283481700610061, -0.307212042374800, -0.369078720749224, -0.416539404278702, -0.440269746043441, -0.487730429572919,
    0.381166774075590, 0.378866636898153, 0.427695421221120, 0.269708382365037, 0.213979186509760, -0.101994891202405, -0.259981930058488, -0.575956007770654,
    -0.227185052857744, -0.469620636740071, 0.828412170627898, -0.133259849703043, -0.130174916014859, -0.0535189566767408, -0.0151909770076815, 0.0614649823304370,
    -0.262651399482480, -0.301123351378262, -0.120288604832971, 0.894501576357598, -0.112627300393190, -0.0830469380120526, -0.0682567568214837, -0.0386763944403458,
    -0.324884751171929, -0.243759383675343, -0.107225790475501, -0.104644995857761, 0.880843955313500, -0.113994455451022, -0.111413660833282, -0.106252071597804,
    -0.395817444421400, 0.0932351870482750, -0.00462734139723796, -0.0491221437364792, -0.0840608134431630, 0.826949581878355, -0.217545220460887, -0.306534825139369,
    -0.431283791046136, 0.261732472410084, 0.0466718831418935, -0.0213607176758380, -0.0665131978214941, -0.202578399456957, 0.729388999725311, -0.406676201910152,
    -0.502216484295607, 0.598727043133703, 0.149270332220156, 0.0341621344454443, -0.0314179665781565, -0.261634362127581, -0.376742559902293, 0.393041044548282};

  const s32 m = 2, n = 8;

  Array<f64> a(m, n, ms);
  Array<f64> w(1, n, ms);
  Array<f64> uT(m, m, ms);
  Array<f64> vT(n, n, ms);
  Array<f64> w_groundTruth(1, n, ms);
  Array<f64> uT_groundTruth(m, m, ms);
  Array<f64> vT_groundTruth(n, n, ms);

  ASSERT_TRUE(a.IsValid());
  ASSERT_TRUE(w.IsValid());
  ASSERT_TRUE(uT.IsValid());
  ASSERT_TRUE(vT.IsValid());
  ASSERT_TRUE(w_groundTruth.IsValid());
  ASSERT_TRUE(uT_groundTruth.IsValid());
  ASSERT_TRUE(vT_groundTruth.IsValid());

  void * scratch = ms.Allocate(sizeof(float)*(2*n + 2*m + 64));
  ASSERT_TRUE(scratch != NULL);

  a[0][0] = 1.0f; a[0][1] = 2.0f; a[0][2] = 3.0f; a[0][3] = 5.0f; a[0][4] = 7.0f; a[0][5] = 11.0f; a[0][6] = 13.0f; a[0][7] = 17.0f;
  a[1][0] = 19.0f; a[1][1] = 23.0f; a[1][2] = 29.0f; a[1][3] = 31.0f; a[1][4] = 37.0f; a[1][5] = 41.0f; a[1][6] = 43.0f; a[1][7] = 47.0f;

  //a.Set(aData, SVD64_aDataLength);
  w_groundTruth.Set(wGroundTruthData, SVD64_uTGroundTruthDataLength);
  uT_groundTruth.Set(uTGroundTruthData, SVD64_wGroundTruthDataLength);
  vT_groundTruth.Set(vTGroundTruthData, SVD64_vTGroundTruthDataLength);

  const Result result = svd_f64(a, w, uT, vT, scratch);

  ASSERT_TRUE(result == RESULT_OK);

  //  w.Print("w");
  //  w_groundTruth.Print("w_groundTruth");
  //PrintfOneArray_f64(w, "w");
  //PrintfOneArray_f64(w_groundTruth, "w_groundTruth");

  //  uT.Print("uT");
  //  uT_groundTruth.Print("uT_groundTruth");
  PrintfOneArray_f64(uT, "uT");
  PrintfOneArray_f64(uT_groundTruth, "uT_groundTruth");

  //  vT.Print("vT");
  //  vT_groundTruth.Print("vT_groundTruth");

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(w, w_groundTruth, .05, .001));
  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(uT, uT_groundTruth, .05, .001));

  // I don't know why, but the v-transpose for this SVD doesn't match Matlab's. Probably this version's is more efficient, in either memory or computation.
  //ASSERT_TRUE(vT.IsElementwiseEqual_PercentThreshold(vT_groundTruth, .05, .001));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MemoryStack)
{
  ASSERT_TRUE(MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(MAX_BYTES, 200);
  void * alignedBuffer = reinterpret_cast<void*>(RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT));
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(alignedBuffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

  void * const buffer1 = ms.Allocate(5);
  void * const buffer2 = ms.Allocate(16);
  void * const buffer3 = ms.Allocate(17);

  ASSERT_TRUE(buffer1 != NULL);
  ASSERT_TRUE(buffer2 != NULL);
  ASSERT_TRUE(buffer3 != NULL);

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS
  void * const buffer4 = ms.Allocate(0);
  void * const buffer5 = ms.Allocate(100);
  void * const buffer6 = ms.Allocate(0x3FFFFFFF);
  void * const buffer7 = ms.Allocate(u32_MAX);

  ASSERT_TRUE(buffer4 == NULL);
  ASSERT_TRUE(buffer5 == NULL);
  ASSERT_TRUE(buffer6 == NULL);
  ASSERT_TRUE(buffer7 == NULL);

  const size_t alignedBufferSizeT = reinterpret_cast<size_t>(alignedBuffer);
  const size_t buffer1SizeT = reinterpret_cast<size_t>(buffer1);
  const size_t buffer2SizeT = reinterpret_cast<size_t>(buffer2);
  const size_t buffer3SizeT = reinterpret_cast<size_t>(buffer3);

  ASSERT_TRUE((buffer1SizeT-alignedBufferSizeT) == 16);
  ASSERT_TRUE((buffer2SizeT-alignedBufferSizeT) == 48);
  ASSERT_TRUE((buffer3SizeT-alignedBufferSizeT) == 80);

  ASSERT_TRUE(ms.IsValid());

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS
#define NUM_EXPECTED_RESULTS 116
  const bool expectedResults[NUM_EXPECTED_RESULTS] = {
    // Allocation 1
    true,  true,  true,  true,  true,  true,  true,  true,  // Unused space
    false, false, false, false, false, false, false, false, // Header
    true,  true,  true,  true,  true,  true,  true,  true,  // Data
    true,  true,  true,  true,  true,  true,  true,  true,  // Data
    false, false, false, false,                             // Footer

    // Allocation 2
    true,  true,  true,  true,                              // Unused space
    false, false, false, false, false, false, false, false, // Header
    true,  true,  true,  true,  true,  true,  true,  true,  // Data
    true,  true,  true,  true,  true,  true,  true,  true,  // Data
    false, false, false, false,                             // Footer

    // Allocation 3
    true,  true,  true,  true,                              // Unused space
    false, false, false, false, false, false, false, false, // Header
    true,  true,  true,  true,  true,  true,  true,  true,  // Data
    true,  true,  true,  true,  true,  true,  true,  true,  // Data
    true,  true,  true,  true,  true,  true,  true,  true,  // Data
    true,  true,  true,  true,  true,  true,  true,  true,  // Data
    false, false, false, false};                            // Footer

  for(s32 i=0; i<NUM_EXPECTED_RESULTS; i++) {
    ASSERT_TRUE(ms.IsValid());
    (reinterpret_cast<char*>(alignedBuffer)[i])++;
    ASSERT_TRUE(ms.IsValid() == expectedResults[i]);
    (reinterpret_cast<char*>(alignedBuffer)[i])--;
  }
#endif // #if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS
#endif // #if ANKI_DEBUG_LEVEL > ANKI_DEBUG_ERRORS

  GTEST_RETURN_HERE;
}

s32 CheckMemoryStackUsage(MemoryStack ms, s32 numBytes)
{
  ms.Allocate(numBytes);
  return ms.get_usedBytes();
}

s32 CheckConstCasting(const MemoryStack ms, s32 numBytes)
{
  // ms.Allocate(1); // Will not compile

  return CheckMemoryStackUsage(ms, numBytes);
}

GTEST_TEST(CoreTech_Common, MemoryStack_call)
{
  const s32 numBytes = MIN(MAX_BYTES, 100);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);

  ASSERT_TRUE(ms.get_usedBytes() == 0);

  ms.Allocate(16);
  const s32 originalUsedBytes = ms.get_usedBytes();
  ASSERT_TRUE(originalUsedBytes >= 28);

  const s32 inFunctionUsedBytes = CheckMemoryStackUsage(ms, 32);
  ASSERT_TRUE(inFunctionUsedBytes == (originalUsedBytes+48));
  ASSERT_TRUE(ms.get_usedBytes() == originalUsedBytes);

  const s32 inFunctionUsedBytes2 = CheckConstCasting(ms, 32);
  ASSERT_TRUE(inFunctionUsedBytes2 == (originalUsedBytes+48));
  ASSERT_TRUE(ms.get_usedBytes() == originalUsedBytes);

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1)
{
  ASSERT_TRUE(MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(MAX_BYTES, 104); // 12*9 = 104
  ASSERT_TRUE(buffer != NULL);

  void * alignedBuffer = reinterpret_cast<void*>(RoundUp<size_t>(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT));

  const size_t bufferShift = reinterpret_cast<size_t>(alignedBuffer) - reinterpret_cast<size_t>(buffer);
  ASSERT_TRUE(bufferShift < static_cast<size_t>(MEMORY_ALIGNMENT));

  MemoryStack ms(alignedBuffer, numBytes);
  const s32 largestPossibleAllocation1 = ms.ComputeLargestPossibleAllocation();
  ASSERT_TRUE(largestPossibleAllocation1 == 80);

  const void * const allocatedBuffer1 = ms.Allocate(1);
  const s32 largestPossibleAllocation2 = ms.ComputeLargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer1 != NULL);
  ASSERT_TRUE(largestPossibleAllocation2 == 48);

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS
  const void * const allocatedBuffer2 = ms.Allocate(49);
  const s32 largestPossibleAllocation3 = ms.ComputeLargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer2 == NULL);
  ASSERT_TRUE(largestPossibleAllocation3 == 48);

  const void * const allocatedBuffer3 = ms.Allocate(48);
  const s32 largestPossibleAllocation4 = ms.ComputeLargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer3 != NULL);
  ASSERT_TRUE(largestPossibleAllocation4 == 0);
#endif // #if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS

  GTEST_RETURN_HERE;
}

#if ANKICORETECH_EMBEDDED_USE_MATLAB
GTEST_TEST(CoreTech_Common, SimpleMatlabTest1)
{
  matlab.EvalStringEcho("simpleVector = double([1.1,2.1,3.1,4.1,5.1]);");
  double *simpleVector = matlab.Get<double>("simpleVector");

  printf("simple vector:\n%f %f %f %f %f\n", simpleVector[0], simpleVector[1], simpleVector[2], simpleVector[3], simpleVector[4]);

  ASSERT_EQ(1.1, simpleVector[0]);
  ASSERT_EQ(2.1, simpleVector[1]);
  ASSERT_EQ(3.1, simpleVector[2]);
  ASSERT_EQ(4.1, simpleVector[3]);
  ASSERT_EQ(5.1, simpleVector[4]);

  free(simpleVector); simpleVector = NULL;

  GTEST_RETURN_HERE;
}
#endif //#if ANKICORETECH_EMBEDDED_USE_MATLAB

#if ANKICORETECH_EMBEDDED_USE_MATLAB
GTEST_TEST(CoreTech_Common, SimpleMatlabTest2)
{
  matlab.EvalStringEcho("simpleArray = int16([1,2,3,4,5;6,7,8,9,10]);");
  Array<s16> simpleArray = matlab.GetArray<s16>("simpleArray");
  printf("simple matrix:\n");
  simpleArray.Print();

  ASSERT_EQ(1, *simpleArray.Pointer(0,0));
  ASSERT_EQ(2, *simpleArray.Pointer(0,1));
  ASSERT_EQ(3, *simpleArray.Pointer(0,2));
  ASSERT_EQ(4, *simpleArray.Pointer(0,3));
  ASSERT_EQ(5, *simpleArray.Pointer(0,4));
  ASSERT_EQ(6, *simpleArray.Pointer(1,0));
  ASSERT_EQ(7, *simpleArray.Pointer(1,1));
  ASSERT_EQ(8, *simpleArray.Pointer(1,2));
  ASSERT_EQ(9, *simpleArray.Pointer(1,3));
  ASSERT_EQ(10, *simpleArray.Pointer(1,4));

  free(simpleArray.get_rawDataPointer());

  GTEST_RETURN_HERE;
}
#endif //#if ANKICORETECH_EMBEDDED_USE_MATLAB

#if ANKICORETECH_EMBEDDED_USE_OPENCV
GTEST_TEST(CoreTech_Common, SimpleOpenCVTest)
{
  cv::Mat src, dst;

  const double sigma = 1.5;
  const double numSigma = 2.0;
  const int k = 2*int(std::ceil(double(numSigma)*sigma)) + 1;

  src = cv::Mat(100, 6, CV_64F, 0.0);
  dst = cv::Mat(100, 6, CV_64F);

  cv::Size ksize(k,k);

  src.at<double>(50, 0) = 5;
  src.at<double>(50, 1) = 6;
  src.at<double>(50, 2) = 7;

  cv::GaussianBlur(src, dst, ksize, sigma, sigma, cv::BORDER_REFLECT_101);

  printf("%f %f\n%f %f\n%f %f\n",
    src.at<double>(50, 0), src.at<double>(50, 1), *( ((double*)src.data) + 50*6), *( ((double*)src.data) + 50*6 + 1), dst.at<double>(50, 0), dst.at<double>(50, 1));
  /*std::cout << src.at<double>(50, 0) << " "
  << src.at<double>(50, 1) << "\n"
  << *( ((double*)src.data) + 50*6) << " "
  << *( ((double*)src.data) + 50*6 + 1) << "\n"
  << dst.at<double>(50, 0) << " "
  << dst.at<double>(50, 1) << "\n";*/

  ASSERT_EQ(5, src.at<double>(50, 0));
  ASSERT_EQ(6, src.at<double>(50, 1));
  ASSERT_TRUE(abs(1.49208 - dst.at<double>(50, 0)) < .00001);
  ASSERT_TRUE(abs(1.39378 - dst.at<double>(50, 1)) < .00001);

  GTEST_RETURN_HERE;
}
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest)
{
  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);

  // Create a matrix, and manually set a few values
  Array<s16> simpleArray(10, 6, ms);
  ASSERT_TRUE(simpleArray.get_rawDataPointer() != NULL);
  *simpleArray.Pointer(0,0) = 1;
  *simpleArray.Pointer(0,1) = 2;
  *simpleArray.Pointer(0,2) = 3;
  *simpleArray.Pointer(0,3) = 4;
  *simpleArray.Pointer(0,4) = 5;
  *simpleArray.Pointer(0,5) = 6;
  *simpleArray.Pointer(2,0) = 7;
  *simpleArray.Pointer(2,1) = 8;
  *simpleArray.Pointer(2,2) = 9;
  *simpleArray.Pointer(2,3) = 10;
  *simpleArray.Pointer(2,4) = 11;
  *simpleArray.Pointer(2,5) = 12;

#if ANKICORETECH_EMBEDDED_USE_MATLAB
  // Check that the Matlab transfer works (you need to check the Matlab window to verify that this works)
  matlab.PutArray<s16>(simpleArray, "simpleArray");
#endif //#if ANKICORETECH_EMBEDDED_USE_MATLAB

#if ANKICORETECH_EMBEDDED_USE_OPENCV
  // Check that the templated OpenCV matrix works
  {
    cv::Mat_<s16> &simpleArray_cvMat = simpleArray.get_CvMat_();
    printf("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    printf("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat(2,0));

    ASSERT_EQ(7, *simpleArray.Pointer(2,0));
    ASSERT_EQ(7, simpleArray_cvMat(2,0));

    printf("Setting OpenCV matrix\n");
    simpleArray_cvMat(2,0) = 100;
    printf("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));

    printf("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat(2,0));
    ASSERT_EQ(100, *simpleArray.Pointer(2,0));
    ASSERT_EQ(100, simpleArray_cvMat(2,0));

    printf("Setting CoreTech_Common matrix\n");
    *simpleArray.Pointer(2,0) = 42;
    printf("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    printf("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat(2,0));
    ASSERT_EQ(42, *simpleArray.Pointer(2,0));
    ASSERT_EQ(42, simpleArray_cvMat(2,0));
  }

  printf("\n\n");

  // Check that the non-templated OpenCV matrix works
  {
    cv::Mat &simpleArray_cvMat = simpleArray.get_CvMat_();
    printf("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    printf("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat.at<s16>(2,0));
    ASSERT_EQ(42, *simpleArray.Pointer(2,0));
    ASSERT_EQ(42, simpleArray_cvMat.at<s16>(2,0));

    printf("Setting OpenCV matrix\n");
    simpleArray_cvMat.at<s16>(2,0) = 300;
    printf("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    printf("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat.at<s16>(2,0));
    ASSERT_EQ(300, *simpleArray.Pointer(2,0));
    ASSERT_EQ(300, simpleArray_cvMat.at<s16>(2,0));

    printf("Setting CoreTech_Common matrix\n");
    *simpleArray.Pointer(2,0) = 90;
    printf("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    printf("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat.at<s16>(2,0));
    ASSERT_EQ(90, *simpleArray.Pointer(2,0));
    ASSERT_EQ(90, simpleArray_cvMat.at<s16>(2,0));
  }
#endif //#if ANKICORETECH_EMBEDDED_USE_OPENCV

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ArraySpecifiedClass)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  ASSERT_TRUE(buffer != NULL);

  MemoryStack ms(buffer, numBytes);

  Array<u8> simpleArray(3, 3, ms);

#define ArraySpecifiedClass_imgDataLength 9
  const s32 imgData[ArraySpecifiedClass_imgDataLength] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1};

  simpleArray.SetCast<s32>(imgData, ArraySpecifiedClass_imgDataLength);

  printf("*simpleArray.Pointer(0,0) = %d\n", *simpleArray.Pointer(0,0));

  simpleArray.Print("simpleArray");

  ASSERT_TRUE((*simpleArray.Pointer(0,0)) == 1); // If the templating fails, this will equal 49

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ArrayAlignment1)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    Array<s16> simpleArray(10, 6, alignedBufferAndOffset, numBytes-offset-8);

    const size_t trueLocation = reinterpret_cast<size_t>(simpleArray.Pointer(0,0));
    const size_t expectedLocation = RoundUp(reinterpret_cast<size_t>(alignedBufferAndOffset), MEMORY_ALIGNMENT);;

    ASSERT_TRUE(trueLocation ==  expectedLocation);
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MemoryStackAlignment)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    MemoryStack simpleMemoryStack(alignedBufferAndOffset, numBytes-offset-8);
    Array<s16> simpleArray(10, 6, simpleMemoryStack);

    const size_t matrixStart = reinterpret_cast<size_t>(simpleArray.Pointer(0,0));
    ASSERT_TRUE(matrixStart == RoundUp(matrixStart, MEMORY_ALIGNMENT));
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ArrayFillPattern)
{
  const s32 width = 6, height = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT) );

  MemoryStack ms(alignedBuffer, numBytes-MEMORY_ALIGNMENT);

  // Create a matrix, and manually set a few values
  Array<s16> simpleArray(height, width, ms, Flags::Buffer(true,true));
  ASSERT_TRUE(simpleArray.get_rawDataPointer() != NULL);

  ASSERT_TRUE(simpleArray.IsValid());

  char * curDataPointer = reinterpret_cast<char*>(simpleArray.get_rawDataPointer());

  // Unused space
  for(s32 i=0; i<8; i++) {
    ASSERT_TRUE(simpleArray.IsValid());
    (*curDataPointer)++;
    ASSERT_TRUE(simpleArray.IsValid());
    (*curDataPointer)--;

    curDataPointer++;
  }

  for(s32 y=0; y<height; y++) {
    // Header
    for(s32 x=0; x<8; x++) {
      ASSERT_TRUE(simpleArray.IsValid());
      (*curDataPointer)++;
      ASSERT_FALSE(simpleArray.IsValid());
      (*curDataPointer)--;

      curDataPointer++;
    }

    // Data
    for(s32 x=8; x<24; x++) {
      ASSERT_TRUE(simpleArray.IsValid());
      (*curDataPointer)++;
      ASSERT_TRUE(simpleArray.IsValid());
      (*curDataPointer)--;

      curDataPointer++;
    }

    // Footer
    for(s32 x=24; x<32; x++) {
      ASSERT_TRUE(simpleArray.IsValid());
      (*curDataPointer)++;
      ASSERT_FALSE(simpleArray.IsValid());
      (*curDataPointer)--;

      curDataPointer++;
    }
  }

  GTEST_RETURN_HERE;
}

#if !defined(ANKICORETECH_EMBEDDED_USE_GTEST)
int RUN_ALL_TESTS()
{
  s32 numPassedTests = 0;
  s32 numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Common, ExplicitPrintf);
  CALL_GTEST_TEST(CoreTech_Common, MatrixSortWithIndexes);
  CALL_GTEST_TEST(CoreTech_Common, MatrixSort);
  CALL_GTEST_TEST(CoreTech_Common, LinearLeastSquares32);
  CALL_GTEST_TEST(CoreTech_Common, LinearLeastSquares64);
  CALL_GTEST_TEST(CoreTech_Common, MatrixMultiplyTranspose);
  CALL_GTEST_TEST(CoreTech_Common, Reshape);
  CALL_GTEST_TEST(CoreTech_Common, ArrayPatterns);
  CALL_GTEST_TEST(CoreTech_Common, Interp2_twoDimensional);
  CALL_GTEST_TEST(CoreTech_Common, Interp2_oneDimensional);
  CALL_GTEST_TEST(CoreTech_Common, Meshgrid_twoDimensional);
  CALL_GTEST_TEST(CoreTech_Common, Meshgrid_oneDimensional);
  CALL_GTEST_TEST(CoreTech_Common, Linspace);
  CALL_GTEST_TEST(CoreTech_Common, Find_SetArray1);
  CALL_GTEST_TEST(CoreTech_Common, Find_SetArray2);
  CALL_GTEST_TEST(CoreTech_Common, Find_SetValue);
  CALL_GTEST_TEST(CoreTech_Common, ZeroSizedArray);
  CALL_GTEST_TEST(CoreTech_Common, Find_Evaluate1D);
  CALL_GTEST_TEST(CoreTech_Common, Find_Evaluate2D);
  CALL_GTEST_TEST(CoreTech_Common, Find_NumMatchesAndBoundingRectangle);
  CALL_GTEST_TEST(CoreTech_Common, MatrixElementwise);
  CALL_GTEST_TEST(CoreTech_Common, SliceArrayAssignment);
  CALL_GTEST_TEST(CoreTech_Common, SliceArrayCompileTest);
  CALL_GTEST_TEST(CoreTech_Common, MatrixMinAndMaxAndSum);
  CALL_GTEST_TEST(CoreTech_Common, LinearSequence);
  CALL_GTEST_TEST(CoreTech_Common, ReallocateArray);
  CALL_GTEST_TEST(CoreTech_Common, ReallocateMemoryStack);

#ifdef TEST_BENCHMARKING
  CALL_GTEST_TEST(CoreTech_Common, Benchmarking);
#endif

  CALL_GTEST_TEST(CoreTech_Common, MemoryStackId);
  CALL_GTEST_TEST(CoreTech_Common, ApproximateExp);
  CALL_GTEST_TEST(CoreTech_Common, MatrixMultiply);
  CALL_GTEST_TEST(CoreTech_Common, ComputeHomography);
  CALL_GTEST_TEST(CoreTech_Common, SVD32);
  CALL_GTEST_TEST(CoreTech_Common, SVD64);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack_call);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1);
  //CALL_GTEST_TEST(CoreTech_Common, SimpleMatlabTest1);
  //CALL_GTEST_TEST(CoreTech_Common, SimpleMatlabTest2);
  //CALL_GTEST_TEST(CoreTech_Common, SimpleOpenCVTest);
  CALL_GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest);
  CALL_GTEST_TEST(CoreTech_Common, ArraySpecifiedClass);
  CALL_GTEST_TEST(CoreTech_Common, ArrayAlignment1);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStackAlignment);
  CALL_GTEST_TEST(CoreTech_Common, ArrayFillPattern);

  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);

  return numFailedTests;
} // int RUN_ALL_TESTS()
#endif // #if !defined(ANKICORETECH_EMBEDDED_USE_GTEST)