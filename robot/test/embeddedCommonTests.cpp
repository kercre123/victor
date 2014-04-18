/**
File: embeddedCommonTests.cpp
Author: Peter Barnum
Created: 2013

Various tests of the coretech common embedded library.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef COZMO_ROBOT
#define COZMO_ROBOT
#endif

#include "anki/common/robot/config.h"
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
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"
#include "anki/common/robot/compress.h"

#include "embeddedTests.h"

using namespace Anki::Embedded;

GTEST_TEST(CoreTech_Common, RoundAndSaturate)
{
  ASSERT_TRUE(Round<u32>(-1.6f) == 0);  ASSERT_TRUE(Round<u32>(-1.1f) == 0);  ASSERT_TRUE(Round<u32>(0.0f) == 0); ASSERT_TRUE(Round<u32>(1.1f) == 1); ASSERT_TRUE(Round<u32>(1.6f) == 2);
  ASSERT_TRUE(Round<s32>(-1.6f) == -2); ASSERT_TRUE(Round<s32>(-1.1f) == -1); ASSERT_TRUE(Round<s32>(0.0f) == 0); ASSERT_TRUE(Round<s32>(1.1f) == 1); ASSERT_TRUE(Round<s32>(1.6f) == 2);
  ASSERT_TRUE(Round<u64>(-1.6f) == 0);  ASSERT_TRUE(Round<u64>(-1.1f) == 0);  ASSERT_TRUE(Round<u64>(0.0f) == 0); ASSERT_TRUE(Round<u64>(1.1f) == 1); ASSERT_TRUE(Round<u64>(1.6f) == 2);
  ASSERT_TRUE(Round<s64>(-1.6f) == -2); ASSERT_TRUE(Round<s64>(-1.1f) == -1); ASSERT_TRUE(Round<s64>(0.0f) == 0); ASSERT_TRUE(Round<s64>(1.1f) == 1); ASSERT_TRUE(Round<s64>(1.6f) == 2);
  ASSERT_TRUE(Round<f32>(-1.6f) == -2); ASSERT_TRUE(Round<f32>(-1.1f) == -1); ASSERT_TRUE(Round<f32>(0.0f) == 0); ASSERT_TRUE(Round<f32>(1.1f) == 1); ASSERT_TRUE(Round<f32>(1.6f) == 2);
  ASSERT_TRUE(Round<f64>(-1.6f) == -2); ASSERT_TRUE(Round<f64>(-1.1f) == -1); ASSERT_TRUE(Round<f64>(0.0f) == 0); ASSERT_TRUE(Round<f64>(1.1f) == 1); ASSERT_TRUE(Round<f64>(1.6f) == 2);

  ASSERT_TRUE(Round<u32>(-1.6) == 0);  ASSERT_TRUE(Round<u32>(-1.1) == 0);  ASSERT_TRUE(Round<u32>(0.0) == 0); ASSERT_TRUE(Round<u32>(1.1) == 1); ASSERT_TRUE(Round<u32>(1.6) == 2);
  ASSERT_TRUE(Round<s32>(-1.6) == -2); ASSERT_TRUE(Round<s32>(-1.1) == -1); ASSERT_TRUE(Round<s32>(0.0) == 0); ASSERT_TRUE(Round<s32>(1.1) == 1); ASSERT_TRUE(Round<s32>(1.6) == 2);
  ASSERT_TRUE(Round<u64>(-1.6) == 0);  ASSERT_TRUE(Round<u64>(-1.1) == 0);  ASSERT_TRUE(Round<u64>(0.0) == 0); ASSERT_TRUE(Round<u64>(1.1) == 1); ASSERT_TRUE(Round<u64>(1.6) == 2);
  ASSERT_TRUE(Round<s64>(-1.6) == -2); ASSERT_TRUE(Round<s64>(-1.1) == -1); ASSERT_TRUE(Round<s64>(0.0) == 0); ASSERT_TRUE(Round<s64>(1.1) == 1); ASSERT_TRUE(Round<s64>(1.6) == 2);
  ASSERT_TRUE(Round<f32>(-1.6) == -2); ASSERT_TRUE(Round<f32>(-1.1) == -1); ASSERT_TRUE(Round<f32>(0.0) == 0); ASSERT_TRUE(Round<f32>(1.1) == 1); ASSERT_TRUE(Round<f32>(1.6) == 2);
  ASSERT_TRUE(Round<f64>(-1.6) == -2); ASSERT_TRUE(Round<f64>(-1.1) == -1); ASSERT_TRUE(Round<f64>(0.0) == 0); ASSERT_TRUE(Round<f64>(1.1) == 1); ASSERT_TRUE(Round<f64>(1.6) == 2);

  /**/                                                         ASSERT_TRUE(saturate_cast<u8>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<u8> (0xFF)) == 0xFF);
  ASSERT_TRUE(saturate_cast<u8>(static_cast<s8> (-128)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<s8> (0x7F)) == 0x7F);
  /**/                                                         ASSERT_TRUE(saturate_cast<u8>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<u16>(500)) == 0xFF);
  ASSERT_TRUE(saturate_cast<u8>(static_cast<s16>(-128)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<s16>(500)) == 0xFF);
  /**/                                                         ASSERT_TRUE(saturate_cast<u8>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<u32>(500)) == 0xFF);
  ASSERT_TRUE(saturate_cast<u8>(static_cast<s32>(-128)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<s32>(500)) == 0xFF);
  /**/                                                         ASSERT_TRUE(saturate_cast<u8>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<u64>(500)) == 0xFF);
  ASSERT_TRUE(saturate_cast<u8>(static_cast<s64>(-128)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<s64>(500)) == 0xFF);
  ASSERT_TRUE(saturate_cast<u8>(static_cast<f32>(-128)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<f32>(500)) == 0xFF);
  ASSERT_TRUE(saturate_cast<u8>(static_cast<f64>(-128)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<f64>(500)) == 0xFF);

  /**/                                                            ASSERT_TRUE(saturate_cast<s8>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<u8> (0xFF)) == 0x7F);
  ASSERT_TRUE(saturate_cast<s8>(static_cast<s8> (-128)) == -128); ASSERT_TRUE(saturate_cast<s8>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<s8> (0x7F)) == 0x7F);
  /**/                                                            ASSERT_TRUE(saturate_cast<s8>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<u16>(500)) == 0x7F);
  ASSERT_TRUE(saturate_cast<s8>(static_cast<s16>(-500)) == -128); ASSERT_TRUE(saturate_cast<s8>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<s16>(500)) == 0x7F);
  /**/                                                            ASSERT_TRUE(saturate_cast<s8>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<u32>(500)) == 0x7F);
  ASSERT_TRUE(saturate_cast<s8>(static_cast<s32>(-500)) == -128); ASSERT_TRUE(saturate_cast<s8>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<s32>(500)) == 0x7F);
  /**/                                                            ASSERT_TRUE(saturate_cast<s8>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<u64>(500)) == 0x7F);
  ASSERT_TRUE(saturate_cast<s8>(static_cast<s64>(-500)) == -128); ASSERT_TRUE(saturate_cast<s8>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<s64>(500)) == 0x7F);
  ASSERT_TRUE(saturate_cast<s8>(static_cast<f32>(-500)) == -128); ASSERT_TRUE(saturate_cast<s8>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<f32>(500)) == 0x7F);
  ASSERT_TRUE(saturate_cast<s8>(static_cast<f64>(-500)) == -128); ASSERT_TRUE(saturate_cast<s8>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<s8>(static_cast<f64>(500)) == 0x7F);

  /**/                                                          ASSERT_TRUE(saturate_cast<u16>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<u8> (0xFF))    == 0xFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<s8> (-128)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s8> (0x7F))    == 0x7F);
  /**/                                                          ASSERT_TRUE(saturate_cast<u16>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<u16>(0xFFFF))  == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<s16>(-128)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s16>(32767))  == 32767);
  /**/                                                          ASSERT_TRUE(saturate_cast<u16>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<u32>(500000)) == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<s32>(-128)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s32>(500000)) == 0xFFFF);
  /**/                                                          ASSERT_TRUE(saturate_cast<u16>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<u64>(500000)) == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<s64>(-128)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s64>(500000)) == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<f32>(-128)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<f32>(500000)) == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<f64>(-128)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<f64>(500000)) == 0xFFFF);

  /**/                                                                  ASSERT_TRUE(saturate_cast<s16>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<u8> (0xFF))    == 0xFF);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<s8> (-128)) == -128);      ASSERT_TRUE(saturate_cast<s16>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<s8> (0x7F))    == 0x7F);
  /**/                                                                  ASSERT_TRUE(saturate_cast<s16>(static_cast<u16>(0)) == 0);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<u16>(0xFFFF))  == 32767);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<s16>(-32768)) == -32768);  ASSERT_TRUE(saturate_cast<s16>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<s16>(32767))  == 32767);
  /**/                                                                  ASSERT_TRUE(saturate_cast<s16>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<u32>(500000)) == 32767);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<s32>(-500000)) == -32768); ASSERT_TRUE(saturate_cast<s16>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<s32>(500000)) == 32767);
  /**/                                                                  ASSERT_TRUE(saturate_cast<s16>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<u64>(500000)) == 32767);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<s64>(-500000)) == -32768); ASSERT_TRUE(saturate_cast<s16>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<s64>(500000)) == 32767);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<f32>(-500000)) == -32768); ASSERT_TRUE(saturate_cast<s16>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<f32>(500000)) == 32767);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<f64>(-500000)) == -32768); ASSERT_TRUE(saturate_cast<s16>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<f64>(500000)) == 32767);

  /**/                                                          ASSERT_TRUE(saturate_cast<u32>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<u8> (0xFF))        == 0xFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<s8> (-128)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s8> (0x7F))        == 0x7F);
  /**/                                                          ASSERT_TRUE(saturate_cast<u32>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<u16>(0xFFFF))      == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<s16>(-128)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s16>(0x7FFF))      == 0x7FFF);
  /**/                                                          ASSERT_TRUE(saturate_cast<u32>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<u32>(0xFFFFFFFF)) == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<s32>(-128)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s32>(0x7FFFFFFF)) == 0x7FFFFFFF);
  /**/                                                          ASSERT_TRUE(saturate_cast<u32>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<u64>(5000000000)) == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<s64>(-128)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s64>(5000000000)) == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<f32>(-128)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<f32>(5000000000)) == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<f64>(-128)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<f64>(5000000000)) == 0xFFFFFFFF);

  /**/                                                                           ASSERT_TRUE(saturate_cast<s32>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<u8> (0xFF))        == 0xFF);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<s8> (-128)) == -128);               ASSERT_TRUE(saturate_cast<s32>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<s8> (0x7F))        == 0x7F);
  /**/                                                                           ASSERT_TRUE(saturate_cast<s32>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<u16>(0xFFFF))      == 32767);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<s16>(-32768)) == -32768);           ASSERT_TRUE(saturate_cast<s32>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<s16>(32767))      == 32767);
  /**/                                                                           ASSERT_TRUE(saturate_cast<s32>(static_cast<u32>(0)) == 0);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<u32>(4294967296)) == 2147483647); ASSERT_TRUE(saturate_cast<s32>(static_cast<s32>(-2147483648LL)) == -2147483648LL); ASSERT_TRUE(saturate_cast<s32>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<s32>(2147483647)) == 2147483647);
  /**/                                                                           ASSERT_TRUE(saturate_cast<s32>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<u64>(5000000000LL)) == 2147483647);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<s64>(-5000000000LL)) == -2147483648LL); ASSERT_TRUE(saturate_cast<s32>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<s64>(5000000000LL)) == 2147483647);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<f32>(-5000000000LL)) == -2147483648LL); ASSERT_TRUE(saturate_cast<s32>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<f32>(5000000000LL)) == 2147483647);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<f64>(-5000000000LL)) == -2147483648LL); ASSERT_TRUE(saturate_cast<s32>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<f64>(5000000000LL)) == 2147483647);

  //ASSERT_TRUE(saturate_cast<u8>(static_cast<u8>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<u8>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<s8>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<s8>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<s8>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<u16>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<s16>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<s16>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<s16>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<u32>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<s32>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<s32>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<s32>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<u8>(static_cast<u64>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<s64>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<s64>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<s64>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<f32>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<f32>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<f32>()) == );
  //ASSERT_TRUE(saturate_cast<u8>(static_cast<f64>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<f64>()) == ); ASSERT_TRUE(saturate_cast<u8>(static_cast<f64>()) == );

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, RoundAndSaturate)

GTEST_TEST(CoreTech_Common, Rotate90)
{
  const s32 arrayHeight = 4;

  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<s16> in(arrayHeight, arrayHeight, ms);
  Array<s16> out(arrayHeight, arrayHeight, ms);
  Array<s16> out90_groundTruth(arrayHeight, arrayHeight, ms);
  Array<s16> out180_groundTruth(arrayHeight, arrayHeight, ms);
  Array<s16> out270_groundTruth(arrayHeight, arrayHeight, ms);

  in[0][0] = 0;  in[0][1] = 0;  in[0][2] = 2;  in[0][3] = 3;
  in[1][0] = 4;  in[1][1] = 5;  in[1][2] = 0;  in[1][3] = 7;
  in[2][0] = 8;  in[2][1] = 9;  in[2][2] = 10; in[2][3] = 0;
  in[3][0] = 12; in[3][1] = 13; in[3][2] = 14; in[3][3] = 15;

  out90_groundTruth[0][0] = 12; out90_groundTruth[0][1] = 8;  out90_groundTruth[0][2] = 4; out90_groundTruth[0][3] = 0;
  out90_groundTruth[1][0] = 13; out90_groundTruth[1][1] = 9;  out90_groundTruth[1][2] = 5; out90_groundTruth[1][3] = 0;
  out90_groundTruth[2][0] = 14; out90_groundTruth[2][1] = 10; out90_groundTruth[2][2] = 0; out90_groundTruth[2][3] = 2;
  out90_groundTruth[3][0] = 15; out90_groundTruth[3][1] = 0;  out90_groundTruth[3][2] = 7; out90_groundTruth[3][3] = 3;

  out180_groundTruth[0][0] = 15; out180_groundTruth[0][1] = 14; out180_groundTruth[0][2] = 13; out180_groundTruth[0][3] = 12;
  out180_groundTruth[1][0] = 0;  out180_groundTruth[1][1] = 10; out180_groundTruth[1][2] = 9;  out180_groundTruth[1][3] = 8;
  out180_groundTruth[2][0] = 7;  out180_groundTruth[2][1] = 0;  out180_groundTruth[2][2] = 5;  out180_groundTruth[2][3] = 4;
  out180_groundTruth[3][0] = 3;  out180_groundTruth[3][1] = 2;  out180_groundTruth[3][2] = 0;  out180_groundTruth[3][3] = 0;

  out270_groundTruth[0][0] = 3; out270_groundTruth[0][1] = 7; out270_groundTruth[0][2] = 0;  out270_groundTruth[0][3] = 15;
  out270_groundTruth[1][0] = 2; out270_groundTruth[1][1] = 0; out270_groundTruth[1][2] = 10; out270_groundTruth[1][3] = 14;
  out270_groundTruth[2][0] = 0; out270_groundTruth[2][1] = 5; out270_groundTruth[2][2] = 9;  out270_groundTruth[2][3] = 13;
  out270_groundTruth[3][0] = 0; out270_groundTruth[3][1] = 4; out270_groundTruth[3][2] = 8;  out270_groundTruth[3][3] = 12;

  {
    const Result result = Matrix::Rotate90(in, out);

    ASSERT_TRUE(result == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual<s16>(out, out90_groundTruth));
  }

  {
    const Result result = Matrix::Rotate180(in, out);

    ASSERT_TRUE(result == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual<s16>(out, out180_groundTruth));
  }

  {
    const Result result = Matrix::Rotate270(in, out);

    ASSERT_TRUE(result == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual<s16>(out, out270_groundTruth));
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, RunLengthEncode)
{
  const s32 arrayHeight = 7;
  const s32 arrayWidth = 16;
  const s32 maxCompressedLength = arrayHeight * arrayWidth;

  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> original(arrayHeight, arrayWidth, ms);
  u8 * compressed = reinterpret_cast<u8*>( ms.Allocate(maxCompressedLength) );

  original(1,3,1,5).Set(1);
  original(2,2,8,10).Set(1);
  original(2,2,13,13).Set(1);
  original(6,6,13,15).Set(1);

  s32 compressedLength;
  const Result result = EncodeRunLengthBinary<u8>(original, compressed, maxCompressedLength, compressedLength);

  ASSERT_TRUE(result == RESULT_OK);
  ASSERT_TRUE(compressedLength == 12);

  Array<u8> uncompressed = DecodeRunLengthBinary<u8>(
    compressed, compressedLength,
    arrayHeight, arrayWidth, Flags::Buffer(false,false,false),
    ms);

  ASSERT_TRUE(AreElementwiseEqual<u8>(original, uncompressed));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, IsConvex)
{
  Point<f32> p11(0.0f, 0.0f);
  Point<f32> p12(3.0f, 0.0f);
  Point<f32> p13(2.0f, 3.0f);
  Point<f32> p14(0.0f, 2.0f);
  Point<f32> p15(2.0f, 2.0f);

  Point<f32> p21(0.0f, 0.0f);
  Point<f32> p22(0.0f, 1.0f);
  Point<f32> p23(0.0f, 2.0f);
  Point<f32> p24(1.0f, 1.0f);

  Point<f32> p31(0.0f, 0.0f);
  Point<f32> p32(0.0f, 1.0f);
  Point<f32> p33(0.0f, 2.0f);
  Point<f32> p34(0.0f, 3.0f);

  Quadrilateral<f32> quad11(p11, p12, p13, p14);
  Quadrilateral<f32> quad12(p12, p13, p14, p15);

  Quadrilateral<f32> quad21(p21, p22, p23, p24);

  Quadrilateral<f32> quad31(p31, p32, p33, p34);

  ASSERT_TRUE(quad11.IsConvex());

  ASSERT_FALSE(quad12.IsConvex());

  ASSERT_TRUE(quad21.IsConvex());
  ASSERT_TRUE(quad31.IsConvex());

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, RoundFloat)
{
  const s32 numNumbers = 12;
  const f32 f32s[numNumbers] =             {-1.1f, -1.0f, -0.9f, -0.5f, -0.4f, -0.0f, 0.0f, 0.2f, 0.5f, 0.6f, 1.0f, 1.001f};
  const f32 s32s_groundTruth[numNumbers] = {-1,       -1,    -1,    -1,     0,     0,    0,    0,    1,    1,    1,      1};
  s32 s32s[numNumbers];

  //const f32 pointFive = 0.5f;
  for(s32 i=0; i<numNumbers; i++) {
    s32s[i] = Round<s32>(f32s[i]);

    //printf("%f = %d\n", f32s[i], s32s[i]);
    ASSERT_TRUE(s32s[i] == s32s_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, CompressArray)
{
  const s32 arrayHeight = 3;
  const s32 arrayWidth = 4;

  //printf("Compressor size: %d Decompressor size:%d\n", sizeof(heatshrink_encoder), sizeof(heatshrink_decoder));

  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<s16> original(arrayHeight, arrayWidth, ms);

  original.SetZero();

  const s32 compressedBufferLength = 1000;
  void * compressedBuffer = ms.Allocate(compressedBufferLength);
  memset(compressedBuffer, 0, compressedBufferLength);

  CompressedArray<s16> compressed(compressedBuffer, compressedBufferLength, 0,
    arrayHeight, arrayWidth, Flags::Buffer());

  for(s32 y=0; y<arrayHeight; y++) {
    for(s32 x=0; x<arrayWidth; x++) {
      original[y][x] = static_cast<s16>(10*y + x + 3);
    }
  }

  {
    const Result result = Compress<s16>(original, compressed, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  original.Print("original");

  {
    const s32 originalLength = original.get_size(0) * original.get_stride();
    const s32 compressedLength = compressed.get_compressedBufferUsedLength();
    printf("Original Length:%d Compressed Length:%d Compression percent size:%f\n", originalLength, compressedLength, 100.0f*f32(compressedLength)/f32(originalLength));
  }

  Array<s16> decompressed = Decompress<s16>(compressed, ms);

  decompressed.Print("decompressed");

  ASSERT_TRUE(AreElementwiseEqual<s16>(original, decompressed));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, CompressArray)

GTEST_TEST(CoreTech_Common, Heatshrink)
{
  const s32 dataLength = 1000;

  //printf("Compressor size: %d Decompressor size:%d\n", sizeof(heatshrink_encoder), sizeof(heatshrink_decoder));

  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> original(1, dataLength, ms);
  Array<u8> compressed(1, dataLength + dataLength/2 + 4, ms);
  Array<u8> decompressed(1, dataLength + dataLength/2 + 4, ms);

  compressed.SetZero();
  decompressed.SetZero();

  for(s32 i=0; i<dataLength; i++) {
    original[0][i] = static_cast<u8>((i<<1) + 5);
  }

  s32 compressedLength = -1;
  ASSERT_TRUE(Compress(original.Pointer(0,0), dataLength, compressed.Pointer(0,0), compressed.get_size(1), compressedLength, ms) == RESULT_OK);

  //original.Print("original");
  //compressed.Print("compressed", 0, 0, 0, compressedLength-1);

  printf("Original Length:%d Compressed Length:%d Compression percent size:%f\n", dataLength, compressedLength, 100.0f*f32(compressedLength)/f32(dataLength));

  s32 uncompressedLength = -1;
  ASSERT_TRUE(Decompress(compressed.Pointer(0,0), compressedLength, decompressed.Pointer(0,0), decompressed.get_size(1), uncompressedLength, ms) == RESULT_OK);
  decompressed.Resize(1, dataLength, ms);

  //decompressed.Print("decompressed");

  ASSERT_TRUE(AreElementwiseEqual<u8>(original, decompressed));
  ASSERT_TRUE(uncompressedLength == dataLength);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Heatshrink)

GTEST_TEST(CoreTech_Common, SerializedBuffer)
{
  const s32 segment1Length = 32;
  const s32 segment2Length = 64;
  const s32 segment3Length = 128;

  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, 5000);
  ASSERT_TRUE(ms.IsValid());

  u8 * segment1 = reinterpret_cast<u8*>( ms.Allocate(segment1Length) );
  u8 * segment2 = reinterpret_cast<u8*>( ms.Allocate(segment2Length) );
  u8 * segment3 = reinterpret_cast<u8*>( ms.Allocate(segment3Length) );

  ASSERT_TRUE(segment1 != NULL);
  ASSERT_TRUE(segment2 != NULL);
  ASSERT_TRUE(segment3 != NULL);

  for(s32 i=0; i<segment1Length; i++) {
    segment1[i] = i + 1;
  }

  for(s32 i=0; i<segment2Length; i++) {
    segment2[i] = 2*i + 1;
  }

  for(s32 i=0; i<segment3Length; i++) {
    segment3[i] = 3*i + 1;
  }

  ASSERT_TRUE(offchipBuffer != NULL);
  SerializedBuffer serialized(offchipBuffer+5000, 6000);
  ASSERT_TRUE(serialized.IsValid());

  void * segment1b = serialized.PushBack("segment1", segment1, segment1Length);
  void * segment2b = serialized.PushBack("segment2", segment2, segment2Length);
  void * segment3b = serialized.PushBack("segment3", segment3, segment3Length);

  ASSERT_TRUE(segment1b != NULL);
  ASSERT_TRUE(segment2b != NULL);
  ASSERT_TRUE(segment3b != NULL);

  SerializedBufferConstIterator iterator(serialized);

  {
    s32 segment1LengthB = -1;
    const char * typeName = NULL;
    const char * objectName = NULL;
    ASSERT_TRUE(iterator.HasNext());
    const void * segment1c = iterator.GetNext(&typeName, &objectName, segment1LengthB, true);
    ASSERT_TRUE(segment1LengthB == 48+64);
    ASSERT_TRUE(reinterpret_cast<size_t>(segment1b) == reinterpret_cast<size_t>(segment1c));
    ASSERT_TRUE(strcmp(typeName, "Basic Type Buffer") == 0);
    ASSERT_TRUE(strcmp(objectName, "segment1") == 0);
  }

  {
    s32 segment2LengthB = -1;
    const char * typeName = NULL;
    const char * objectName = NULL;
    ASSERT_TRUE(iterator.HasNext());
    const void * segment2c = iterator.GetNext(&typeName, &objectName, segment2LengthB, true);
    ASSERT_TRUE(segment2LengthB == 80+64);
    ASSERT_TRUE(reinterpret_cast<size_t>(segment2b) == reinterpret_cast<size_t>(segment2c));
    ASSERT_TRUE(strcmp(typeName, "Basic Type Buffer") == 0);
    ASSERT_TRUE(strcmp(objectName, "segment2") == 0);
  }

  {
    s32 segment3LengthB = -1;
    const char * typeName = NULL;
    const char * objectName = NULL;
    ASSERT_TRUE(iterator.HasNext());
    const void * segment3c = iterator.GetNext(&typeName, &objectName, segment3LengthB, true);
    ASSERT_TRUE(segment3LengthB == 144+64);
    ASSERT_TRUE(reinterpret_cast<size_t>(segment3b) == reinterpret_cast<size_t>(segment3c));
    ASSERT_TRUE(strcmp(typeName, "Basic Type Buffer") == 0);
    ASSERT_TRUE(strcmp(objectName, "segment3") == 0);
  }

  ASSERT_FALSE(iterator.HasNext());

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, SerializedBuffer)

GTEST_TEST(CoreTech_Common, CRC32Code)
{
  const s32 numDataBytes = 16;
  const u32 data[] = {0x1234BEF2, 0xA342EE00, 0x00000000, 0xFFFFFFFF};
  const u32 initialCRC = 0xFFFFFFFF;

  const u32 crc = ComputeCRC32(&data[0], numDataBytes, initialCRC);

  const u32 crc_groundTruth = 0xF2939FF3;

  printf("CRC code: 0x%x\n", crc);

  ASSERT_TRUE(crc == crc_groundTruth);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, CRC32Code)

GTEST_TEST(CoreTech_Common, MemoryStackIterator)
{
  //u32 buffer[100];
  //for(u32 i=0; i<100; i++) {
  //  buffer[i] = i+1;
  //}

  ////Anki::Cozmo::HAL::USBPutChar('\n');
  ////Anki::Cozmo::HAL::USBPutChar('\n');
  ////Anki::Cozmo::HAL::USBPutChar('\n');
  ////Anki::Cozmo::HAL::USBPutChar('\n');
  ////printf("\n\n\n\n");

  //Anki::Cozmo::HAL::USBSendBuffer(reinterpret_cast<u8*>(&buffer[0]), 100*sizeof(u32));

  //while(1) {}

  const s32 segment1Length = 32;
  const s32 segment2Length = 64;
  const s32 segment3Length = 128;

  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  void * segment1 = ms.Allocate(segment1Length);
  void * segment2 = ms.Allocate(segment2Length);
  void * segment3 = ms.Allocate(segment3Length);

  ASSERT_TRUE(segment1 != NULL);
  ASSERT_TRUE(segment2 != NULL);
  ASSERT_TRUE(segment3 != NULL);

  MemoryStackIterator msi(ms);

  s32 segment1bLength = -1;
  s32 segment2bLength = -1;
  s32 segment3bLength = -1;

  ASSERT_TRUE(msi.HasNext());
  const void * segment1b = msi.GetNext(segment1bLength);
  ASSERT_TRUE(segment1 == segment1b);
  ASSERT_TRUE(segment1Length == segment1bLength);

  ASSERT_TRUE(msi.HasNext());
  const void * segment2b = msi.GetNext(segment2bLength);
  ASSERT_TRUE(segment2 == segment2b);
  ASSERT_TRUE(segment2Length == segment2bLength);

  ASSERT_TRUE(msi.HasNext());
  const void * segment3b = msi.GetNext(segment3bLength);
  ASSERT_TRUE(segment3 == segment3b);
  ASSERT_TRUE(segment3Length == segment3bLength);

  ASSERT_FALSE(msi.HasNext());

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, MemoryStackIterator)

GTEST_TEST(CoreTech_Common, MatrixTranspose)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  const s32 in_data[12] = {
    1, 2, 3, 4,
    5, 6, 7, 8,
    9, 10, 11, 12};

  Array<s32> in(3,4,ms);
  Array<s16> out(4,3,ms);

  in.Set(in_data, 12);

  Matrix::Transpose(in, out);

  const s32 out_groundTruth_data[12] = {
    1, 5, 9,
    2, 6, 10,
    3, 7, 11,
    4, 8, 12};

  Array<s16> out_groundTruth(4,3,ms);

  out_groundTruth.SetCast<s32>(out_groundTruth_data, 12);

  ASSERT_TRUE(AreElementwiseEqual<s16>(out, out_groundTruth));

  //in.Print("in");
  //out.Print("out");

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, MatrixTranspose)

GTEST_TEST(CoreTech_Common, CholeskyDecomposition)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  const f32 A_data[9] = {
    2.7345f, 1.8859f, 2.0785f,
    1.8859f, 2.2340f, 2.0461f,
    2.0785f, 2.0461f, 2.7591f};

  const f32 B_data[6] = {
    1.0f, 3.0f, 5.0f,
    2.0f, 4.0f, 6.0f};

  Array<f32> A_L(3,3,ms);
  Array<f32> Bt_Xt(2,3,ms);

  ASSERT_TRUE(A_L.IsValid());
  ASSERT_TRUE(Bt_Xt.IsValid());

  A_L.Set(A_data, 9);
  Bt_Xt.Set(B_data, 9);

  A_L.Print("A");
  Bt_Xt.Print("Bt");

  bool numericalFailure;

  const Result result = Matrix::SolveLeastSquaresWithCholesky(A_L, Bt_Xt, true, numericalFailure);
  ASSERT_TRUE(result == RESULT_OK);
  ASSERT_TRUE(numericalFailure == false);

  A_L.Print("L");
  Bt_Xt.Print("Xt");

  const f32 L_groundTruth_data[9] = {
    1.6536f, 0.0f, 0.0f,
    1.1405f, 0.9661f, 0.0f,
    1.2569f, 0.6341f, 0.8815f};

  const f32 Xt_groundTruth_data[9] = {
    -2.4188f, 0.1750f, 3.5046f,
    -2.2968f, 0.4769f, 3.5512f};

  Array<f32> L_groundTruth(3,3,ms);
  Array<f32> Xt_groundTruth(2,3,ms);

  L_groundTruth.Set(L_groundTruth_data, 9);
  Xt_groundTruth.Set(Xt_groundTruth_data, 6);

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(A_L, L_groundTruth, .01, .001));
  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(Bt_Xt, Xt_groundTruth, .01, .001));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, CholeskyDecomposition)

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

  explicitPrintf(0, 0, "The following two lines should be the same:\n");
  explicitPrintf(0, 0, "%x %x %x %x\n", hex1, hex2, hex3, hex4);
  explicitPrintf(0, 0, "%x %x %x %x\n", *((u32*) float1), *((u32*) float2), *((u32*) float3), *((u32*)float4));

  //explicitPrintf(0, EXPLICIT_PRINTF_FLIP_CHARACTERS, "%f %f %f %f\n", *float1, *float2, *float3, *float4);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, ExplicitPrintf)

GTEST_TEST(CoreTech_Common, MatrixSortWithIndexes)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, MatrixSortWithIndexes)

GTEST_TEST(CoreTech_Common, MatrixSort)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, MatrixSort)

GTEST_TEST(CoreTech_Common, MatrixMultiplyTranspose)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, MatrixMultiplyTranspose)

GTEST_TEST(CoreTech_Common, Reshape)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Reshape)

GTEST_TEST(CoreTech_Common, ArrayPatterns)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
          ASSERT_TRUE(DBL_NEAR(pOut[x], 1.0));
        } else {
          ASSERT_TRUE(DBL_NEAR(pOut[x], 0.0));
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
} // GTEST_TEST(CoreTech_Common, ArrayPatterns)

GTEST_TEST(CoreTech_Common, Interp2_Projective_oneDimensional)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> reference(3,5,ms);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid((-0.9:0.9:6), (-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));

  Array<f32> homography = Eye<f32>(3,3,ms);
  homography[0][0] = 1.5f;
  homography[1][2] = 1.0f;
  homography[2][0] = 0.05f;
  homography[2][1] = 0.09f;
  // homography = [1.5, 0, 0.0; 0.0, 1.0, 1.0; 0.05, 0.09, 1.0];

  // points = [xGridVector(:), yGridVector(:), ones(48,1)]';
  // warpedPoints = homography*points;
  // warpedPoints = warpedPoints ./ repmat(warpedPoints(3,:), [3,1]);
  // warpedPoints = warpedPoints(1:2, :);

  // warpedXGridVector = reshape(warpedPoints(1,:), [6,8]);
  // warpedYGridVector = reshape(warpedPoints(2,:), [6,8]);

  // result = round(interp2(reference, 1+warpedXGridVector, 1+warpedYGridVector)); result(isnan(result)) = 0;
  // r = result'; r(:)'

  const Point<f32> centerOffset(0.0f, 0.0f);

  const s32 numElements = mesh.get_yGridVector().get_size() * mesh.get_xGridVector().get_size();
  Array<u8> result(1, numElements, ms);
  const Result lastResult = Interp2_Projective<u8,u8>(reference, mesh, homography, centerOffset, result);
  ASSERT_TRUE(lastResult == RESULT_OK);

  const u8 result_groundTruth[48] = {0, 1, 2, 4, 5, 0, 0, 0, 0, 11, 12, 13, 13, 0, 0, 0, 0, 19, 20, 20, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  //result.Print("result");

  for(s32 i=0; i<48; i++) {
    ASSERT_TRUE(result[0][i] == result_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Interp2_Projective_oneDimensional)

GTEST_TEST(CoreTech_Common, Interp2_Projective_twoDimensional)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> reference(3,5,ms);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid((-0.9:0.9:6), (-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));

  Array<f32> homography = Eye<f32>(3,3,ms);
  homography[0][0] = 1.5f;
  homography[1][2] = 1.0f;
  homography[2][0] = 0.05f;
  homography[2][1] = 0.09f;
  // homography = [1.5, 0, 0.0; 0.0, 1.0, 1.0; 0.05, 0.09, 1.0];

  // points = [xGridVector(:), yGridVector(:), ones(48,1)]';
  // warpedPoints = homography*points;
  // warpedPoints = warpedPoints ./ repmat(warpedPoints(3,:), [3,1]);
  // warpedPoints = warpedPoints(1:2, :);

  // warpedXGridVector = reshape(warpedPoints(1,:), [6,8]);
  // warpedYGridVector = reshape(warpedPoints(2,:), [6,8]);

  // result = round(interp2(reference, 1+warpedXGridVector, 1+warpedYGridVector)); result(isnan(result)) = 0

  const Point<f32> centerOffset(0.0f, 0.0f);

  Array<u8> result(mesh.get_yGridVector().get_size(), mesh.get_xGridVector().get_size(), ms);
  const Result lastResult = Interp2_Projective<u8,u8>(reference, mesh, homography, centerOffset, result);
  ASSERT_TRUE(lastResult == RESULT_OK);

  const u8 result_groundTruth[6][8] = {
    {0, 1, 2, 4, 5, 0, 0, 0},
    {0, 11, 12, 13, 13, 0, 0, 0},
    {0, 19, 20, 20, 21, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},};

  //result.Print("result");

  for(s32 y=0; y<6; y++) {
    for(s32 x=0; x<8; x++) {
      ASSERT_TRUE(result[y][x] == result_groundTruth[y][x]);
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Interp2_Projective_twoDimensional)

GTEST_TEST(CoreTech_Common, Interp2_Affine_twoDimensional)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> reference(3,5,ms);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid((-0.9:0.9:6), (-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));

  Array<f32> homography = Eye<f32>(3,3,ms);
  homography[0][0] = 1.5f;
  homography[1][2] = 1.0f;
  // homography = [1.5, 0, 0.0; 0.0, 1.0, 1.0; 0.0, 0.0, 1.0];

  // points = [xGridVector(:), yGridVector(:), ones(48,1)]';
  // warpedPoints = homography*points;
  // warpedPoints = warpedPoints ./ repmat(warpedPoints(3,:), [3,1]);
  // warpedPoints = warpedPoints(1:2, :);

  // warpedXGridVector = reshape(warpedPoints(1,:), [6,8]);
  // warpedYGridVector = reshape(warpedPoints(2,:), [6,8]);

  const Point<f32> centerOffset(0.0f, 0.0f);

  // result = round(interp2(reference, 1+warpedXGridVector, 1+warpedYGridVector)); result(isnan(result)) = 0
  Array<u8> result(mesh.get_yGridVector().get_size(), mesh.get_xGridVector().get_size(), ms);
  const Result lastResult = Interp2_Affine<u8,u8>(reference, mesh, homography, centerOffset, result);
  ASSERT_TRUE(lastResult == RESULT_OK);

  const u8 result_groundTruth[6][8] = {
    {0,  1,  2,  4, 0, 0, 0, 0},
    {0, 11, 12, 14, 0, 0, 0, 0},
    {0, 21, 22, 24, 0, 0, 0, 0},
    {0,  0,  0,  0, 0, 0, 0, 0},
    {0,  0,  0,  0, 0, 0, 0, 0},
    {0,  0,  0,  0, 0, 0, 0, 0}};

  //result.Print("result");

  for(s32 y=0; y<6; y++) {
    for(s32 x=0; x<8; x++) {
      ASSERT_TRUE(result[y][x] == result_groundTruth[y][x]);
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Interp2_Affine_twoDimensional)

GTEST_TEST(CoreTech_Common, Interp2_Affine_oneDimensional)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> reference(3,5,ms);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid((-0.9:0.9:6), (-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));

  Array<f32> homography = Eye<f32>(3,3,ms);
  homography[0][0] = 1.5f;
  homography[1][2] = 1.0f;
  // homography = [1.5, 0, 0.0; 0.0, 1.0, 1.0; 0.0, 0.0, 1.0];

  // points = [xGridVector(:), yGridVector(:), ones(48,1)]';
  // warpedPoints = homography*points;
  // warpedPoints = warpedPoints ./ repmat(warpedPoints(3,:), [3,1]);
  // warpedPoints = warpedPoints(1:2, :);

  // warpedXGridVector = reshape(warpedPoints(1,:), [6,8]);
  // warpedYGridVector = reshape(warpedPoints(2,:), [6,8]);

  const Point<f32> centerOffset(0.0f, 0.0f);

  // result = round(interp2(reference, 1+warpedXGridVector, 1+warpedYGridVector)); result(isnan(result)) = 0;
  // r = result'; r(:)'
  const s32 numElements = mesh.get_yGridVector().get_size() * mesh.get_xGridVector().get_size();
  Array<u8> result(1, numElements, ms);
  const Result lastResult = Interp2_Affine<u8,u8>(reference, mesh, homography, centerOffset, result);
  ASSERT_TRUE(lastResult == RESULT_OK);

  const u8 result_groundTruth[48] = {0, 1, 2, 4, 0, 0, 0, 0, 0, 11, 12, 14, 0, 0, 0, 0, 0, 21, 22, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  //result.Print("result");

  for(s32 i=0; i<48; i++) {
    ASSERT_TRUE(result[0][i] == result_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Interp2_Affine_oneDimensional)

GTEST_TEST(CoreTech_Common, Interp2_twoDimensional)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Interp2_twoDimensional)

GTEST_TEST(CoreTech_Common, Interp2_oneDimensional)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Interp2_oneDimensional)

GTEST_TEST(CoreTech_Common, Meshgrid_twoDimensional)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Meshgrid_twoDimensional)

GTEST_TEST(CoreTech_Common, Meshgrid_oneDimensional)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Meshgrid_oneDimensional)

GTEST_TEST(CoreTech_Common, Linspace)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Linspace)

GTEST_TEST(CoreTech_Common, Find_SetArray1)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Find_SetArray1)

GTEST_TEST(CoreTech_Common, Find_SetArray2)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Find_SetArray2)

GTEST_TEST(CoreTech_Common, Find_SetValue)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Find_SetValue)

GTEST_TEST(CoreTech_Common, ZeroSizedArray)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, ZeroSizedArray)

GTEST_TEST(CoreTech_Common, Find_Evaluate1D)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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

    ASSERT_TRUE(findB.Evaluate(indexes, ms) == RESULT_FAIL_INVALID_PARAMETERS);
  } // PUSH_MEMORY_STACK(ms)

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Find_Evaluate1D)

GTEST_TEST(CoreTech_Common, Find_Evaluate2D)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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

  ASSERT_TRUE(find.Evaluate(yIndexes, ms) == RESULT_FAIL_INVALID_PARAMETERS);
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
} // GTEST_TEST(CoreTech_Common, Find_Evaluate2D)

GTEST_TEST(CoreTech_Common, Find_NumMatchesAndBoundingRectangle)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, Find_NumMatchesAndBoundingRectangle)

GTEST_TEST(CoreTech_Common, MatrixElementwise)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, MatrixElementwise)

GTEST_TEST(CoreTech_Common, SliceArrayAssignment)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, SliceArrayAssignment)

GTEST_TEST(CoreTech_Common, SliceArrayCompileTest)
{
  // This is just a compile test, and should always pass unless there's a very serious error
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> array1(20,30,ms);

  // This is okay
  ArraySlice<u8> slice1 = array1(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  ASSERT_TRUE(slice1.get_array().IsValid());

  const Array<u8> array2(20,30,ms);

  // Will not compile
  //ArraySlice<u8> slice2 = array2(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  // This is okay
  ConstArraySlice<u8> slice1b = array1(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  ASSERT_TRUE(slice1b.get_array().IsValid());

  // This is okay
  ConstArraySlice<u8> slice2 = array2(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  ASSERT_TRUE(slice2.get_array().IsValid());

  //printf("%d %d %d\n", slice1.get_xSlice().get_start(), slice1.get_xSlice().get_end(), *slice1.get_array().Pointer(0,0));
  //printf("%d %d %d\n", slice1b.get_xSlice().get_start(), slice1b.get_xSlice().get_end(), *slice1b.get_array().Pointer(0,0));
  //printf("%d %d %d\n", slice2.get_xSlice().get_start(), slice2.get_xSlice().get_end(), *slice2.get_array().Pointer(0,0));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, SliceArrayCompileTest)

GTEST_TEST(CoreTech_Common, MatrixMinAndMaxAndSum)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, MatrixMinAndMaxAndSum)

GTEST_TEST(CoreTech_Common, ReallocateArray)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> array1(20,30,ms);
  Array<u8> array2(20,30,ms);
  Array<u8> array3(20,30,ms);

  ASSERT_TRUE(array1.IsValid());
  ASSERT_TRUE(array2.IsValid());
  ASSERT_TRUE(array3.IsValid());

  ASSERT_TRUE(array1.IsValid());
  ASSERT_TRUE(array1.Resize(20,15,ms) == RESULT_FAIL_UNINITIALIZED_MEMORY);
  ASSERT_TRUE(!array1.IsValid());

  ASSERT_TRUE(array3.IsValid());
  ASSERT_TRUE(array3.Resize(20,15,ms) == RESULT_OK);
  ASSERT_TRUE(array3.IsValid());

  ASSERT_TRUE(array2.IsValid());
  ASSERT_TRUE(array2.Resize(20,15,ms) == RESULT_FAIL_UNINITIALIZED_MEMORY);
  ASSERT_TRUE(!array2.IsValid());

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, ReallocateArray)

GTEST_TEST(CoreTech_Common, ReallocateMemoryStack)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, ReallocateMemoryStack)

GTEST_TEST(CoreTech_Common, LinearSequence)
{
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, OFFCHIP_BUFFER_SIZE);
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
} // GTEST_TEST(CoreTech_Common, LinearSequence)

GTEST_TEST(CoreTech_Common, MemoryStackId)
{
  //printf("%f %f %f %f %f\n", 43423442334324.010203, 15.500, 15.0, 0.05, 0.12004333);

  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 5000);
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

  MemoryStack ms1(offchipBuffer, 50);
  MemoryStack ms2(offchipBuffer+50, 50);
  MemoryStack ms2b(ms2);
  MemoryStack ms2c(ms2b);
  MemoryStack ms3(offchipBuffer+50*2, 50);
  MemoryStack ms4(offchipBuffer+50*3, 50);
  MemoryStack ms5(offchipBuffer+50*4, 50);

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
} // GTEST_TEST(CoreTech_Common, MemoryStackId)

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
} // GTEST_TEST(CoreTech_Common, ApproximateExp)

GTEST_TEST(CoreTech_Common, MatrixMultiply)
{
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 5000);
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, numBytes);
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
} // GTEST_TEST(CoreTech_Common, MatrixMultiply)

GTEST_TEST(CoreTech_Common, ComputeHomography)
{
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 5000);
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, numBytes);
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

  const Result result = Matrix::EstimateHomography(originalPointsList, transformedPointsList, homography, ms);

  ASSERT_TRUE(result == RESULT_OK);

  homography.Print("homography");

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(homography, homography_groundTruth, .01, .01));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, ComputeHomography)

GTEST_TEST(CoreTech_Common, MemoryStack)
{
  ASSERT_TRUE(MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 200);
  void * alignedBuffer = reinterpret_cast<void*>(RoundUp(reinterpret_cast<size_t>(offchipBuffer), MEMORY_ALIGNMENT));
  ASSERT_TRUE(offchipBuffer != NULL);
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
} // GTEST_TEST(CoreTech_Common, MemoryStack)

s32 CheckMemoryStackUsage(MemoryStack ms, s32 numBytes)
{
  ms.Allocate(numBytes);
  return ms.get_usedBytes();
} // s32 CheckMemoryStackUsage(MemoryStack ms, s32 numBytes)

s32 CheckConstCasting(const MemoryStack ms, s32 numBytes)
{
  // ms.Allocate(1); // Will not compile

  return CheckMemoryStackUsage(ms, numBytes);
} // s32 CheckConstCasting(const MemoryStack ms, s32 numBytes)

GTEST_TEST(CoreTech_Common, MemoryStack_call)
{
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 100);
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, numBytes);

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
} // GTEST_TEST(CoreTech_Common, MemoryStack_call)

GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1)
{
  ASSERT_TRUE(MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 104); // 12*9 = 104
  ASSERT_TRUE(offchipBuffer != NULL);

  void * alignedBuffer = reinterpret_cast<void*>(RoundUp<size_t>(reinterpret_cast<size_t>(offchipBuffer), MEMORY_ALIGNMENT));

  const size_t bufferShift = reinterpret_cast<size_t>(alignedBuffer) - reinterpret_cast<size_t>(offchipBuffer);
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
} // GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1)

GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest)
{
  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 1000);
  ASSERT_TRUE(offchipBuffer != NULL);
  MemoryStack ms(offchipBuffer, numBytes);

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
} // GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest)

GTEST_TEST(CoreTech_Common, ArraySpecifiedClass)
{
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 1000);
  ASSERT_TRUE(offchipBuffer != NULL);

  MemoryStack ms(offchipBuffer, numBytes);

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
} // GTEST_TEST(CoreTech_Common, ArraySpecifiedClass)

GTEST_TEST(CoreTech_Common, ArrayAlignment1)
{
  ASSERT_TRUE(offchipBuffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(offchipBuffer), MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<=16; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    Array<s16> simpleArray(10, 8, alignedBufferAndOffset, OFFCHIP_BUFFER_SIZE-offset-8);

    if(offset%MEMORY_ALIGNMENT == 0) {
      ASSERT_TRUE(simpleArray.IsValid());
      const size_t trueLocation = reinterpret_cast<size_t>(simpleArray.Pointer(0,0));
      const size_t expectedLocation = RoundUp(reinterpret_cast<size_t>(alignedBufferAndOffset), MEMORY_ALIGNMENT);;
      ASSERT_TRUE(trueLocation ==  expectedLocation);
    } else {
      ASSERT_FALSE(simpleArray.IsValid());
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, ArrayAlignment1)

GTEST_TEST(CoreTech_Common, MemoryStackAlignment)
{
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 1000);
  ASSERT_TRUE(offchipBuffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(offchipBuffer), MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    MemoryStack simpleMemoryStack(alignedBufferAndOffset, numBytes-offset-8);
    Array<s16> simpleArray(10, 6, simpleMemoryStack);

    const size_t matrixStart = reinterpret_cast<size_t>(simpleArray.Pointer(0,0));
    ASSERT_TRUE(matrixStart == RoundUp(matrixStart, MEMORY_ALIGNMENT));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, MemoryStackAlignment)

GTEST_TEST(CoreTech_Common, ArrayFillPattern)
{
  const s32 width = 6, height = 10;
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 1000);
  ASSERT_TRUE(offchipBuffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(offchipBuffer), MEMORY_ALIGNMENT) );

  MemoryStack ms(alignedBuffer, numBytes-MEMORY_ALIGNMENT);

  // Create a matrix, and manually set a few values
  Array<s16> simpleArray(height, width, ms, Flags::Buffer(true,true,false));
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
} // GTEST_TEST(CoreTech_Common, ArrayFillPattern)

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
} // GTEST_TEST(CoreTech_Common, Benchmarking)
#endif //#ifdef TEST_BENCHMARKING

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
} // GTEST_TEST(CoreTech_Common, SimpleMatlabTest1)
#endif //#if ANKICORETECH_EMBEDDED_USE_MATLAB

#if ANKICORETECH_EMBEDDED_USE_MATLAB
GTEST_TEST(CoreTech_Common, SimpleMatlabTest2)
{
  MemoryStack ms(calloc(1000000,1), 1000000);
  ASSERT_TRUE(ms.IsValid());

  matlab.EvalStringEcho("simpleArray = int16([1,2,3,4,5;6,7,8,9,10]);");
  Array<s16> simpleArray = matlab.GetArray<s16>("simpleArray", ms);
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

  free(ms.get_buffer());

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, SimpleMatlabTest2)
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
} // GTEST_TEST(CoreTech_Common, SimpleOpenCVTest)
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

#if !defined(ANKICORETECH_EMBEDDED_USE_GTEST)
s32 RUN_ALL_COMMON_TESTS(s32 &numPassedTests, s32 &numFailedTests)
{
  numPassedTests = 0;
  numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Common, RoundAndSaturate);
  CALL_GTEST_TEST(CoreTech_Common, RunLengthEncode);
  CALL_GTEST_TEST(CoreTech_Common, IsConvex);
  CALL_GTEST_TEST(CoreTech_Common, RoundFloat);
  CALL_GTEST_TEST(CoreTech_Common, CompressArray);
  CALL_GTEST_TEST(CoreTech_Common, Heatshrink);
  CALL_GTEST_TEST(CoreTech_Common, SerializedBuffer);
  CALL_GTEST_TEST(CoreTech_Common, CRC32Code);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStackIterator);
  CALL_GTEST_TEST(CoreTech_Common, MatrixTranspose);
  CALL_GTEST_TEST(CoreTech_Common, CholeskyDecomposition);
  CALL_GTEST_TEST(CoreTech_Common, ExplicitPrintf);
  CALL_GTEST_TEST(CoreTech_Common, MatrixSortWithIndexes);
  CALL_GTEST_TEST(CoreTech_Common, MatrixSort);
  CALL_GTEST_TEST(CoreTech_Common, MatrixMultiplyTranspose);
  CALL_GTEST_TEST(CoreTech_Common, Reshape);
  CALL_GTEST_TEST(CoreTech_Common, ArrayPatterns);
  CALL_GTEST_TEST(CoreTech_Common, Interp2_Affine_twoDimensional);
  CALL_GTEST_TEST(CoreTech_Common, Interp2_Affine_oneDimensional);
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
  CALL_GTEST_TEST(CoreTech_Common, ReallocateArray);
  CALL_GTEST_TEST(CoreTech_Common, ReallocateMemoryStack);
  CALL_GTEST_TEST(CoreTech_Common, LinearSequence);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStackId);
  CALL_GTEST_TEST(CoreTech_Common, ApproximateExp);
  CALL_GTEST_TEST(CoreTech_Common, MatrixMultiply);
  CALL_GTEST_TEST(CoreTech_Common, ComputeHomography);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack_call);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1);
  CALL_GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest);
  CALL_GTEST_TEST(CoreTech_Common, ArraySpecifiedClass);
  CALL_GTEST_TEST(CoreTech_Common, ArrayAlignment1);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStackAlignment);
  CALL_GTEST_TEST(CoreTech_Common, ArrayFillPattern);

#ifdef TEST_BENCHMARKING
  CALL_GTEST_TEST(CoreTech_Common, Benchmarking);
#endif

  //CALL_GTEST_TEST(CoreTech_Common, SimpleMatlabTest1);
  //CALL_GTEST_TEST(CoreTech_Common, SimpleMatlabTest2);
  //CALL_GTEST_TEST(CoreTech_Common, SimpleOpenCVTest);

  //  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);

  return numFailedTests;
} // int RUN_ALL_COMMON_TESTS()
#endif // #if !defined(ANKICORETECH_EMBEDDED_USE_GTEST)
