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

// #define RUN_PC_ONLY_TESTS
// #define RUN_A7_ASSEMBLY_TEST

#include "anki/common/robot/config.h"
#include "anki/common/vectorTypes.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/benchmarking.h"
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
#include "anki/common/robot/hostIntrinsics_m4.h"
#include "anki/common/robot/draw.h"

#include "embeddedTests.h"

using namespace Anki;
using namespace Anki::Embedded;

GTEST_TEST(CoreTech_Common, SerializeStrings)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 arrayHeight = 3;
  const s32 arrayWidth = 2;

  Array<const char*> strings(arrayHeight,arrayWidth,scratchOffchip);
  strings[0][0] = "Hello"; strings[0][1] = "World";
  strings[1][0] = "What's"; strings[1][1] = "up";
  strings[2][0] = "Doc"; strings[2][1] = "??? (Also including a quite long string here, in case there is some overflow somewhere. Possible? Likely? Hard to say. Stream of consciousness...)";

  SerializedBuffer serialized(offchipBuffer+5000, 6000);
  ASSERT_TRUE(serialized.IsValid());

  serialized.PushBack("My String Array", strings);

  SerializedBufferReconstructingIterator iterator(serialized);

  ASSERT_TRUE(iterator.HasNext());

  s32 dataLength;
  const char * typeName = NULL;
  const char * objectName = NULL;
  bool isReportedSegmentLengthCorrect;
  void * dataSegment = reinterpret_cast<u8*>(iterator.GetNext(&typeName, &objectName, dataLength, isReportedSegmentLengthCorrect));

  ASSERT_TRUE(dataSegment);
  ASSERT_TRUE(strcmp(typeName, "Array") == 0);
  ASSERT_TRUE(strcmp(objectName, "My String Array") == 0);

  char innerObjectName[128];
  Array<char*> stringsOut = SerializedBuffer::DeserializeRawArray<char*>(&innerObjectName[0], &dataSegment, dataLength, scratchOffchip);

  //strings.Print("strings");
  //stringsOut.Print("stringsOut");

  ASSERT_TRUE(AreEqualSize(strings, stringsOut));

  for(s32 y=0; y<arrayHeight; y++) {
    char const * const * restrict pStrings = strings.Pointer(y,0);
    char const * const * restrict pStringsOut = stringsOut.Pointer(y,0);
    for(s32 x=0; x<arrayWidth; x++) {
      // Are the string the same?
      ASSERT_TRUE(strcmp(pStrings[x], pStringsOut[x]) == 0);

      // Are the pointer locations different?
      ASSERT_TRUE(reinterpret_cast<size_t>(pStrings[x]) != reinterpret_cast<size_t>(pStringsOut[x]));
    }
  }

  ASSERT_FALSE(iterator.HasNext());

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, SerializeStrings)

GTEST_TEST(CoreTech_Common, DrawQuadrilateral)
{
  const Quadrilateral<f32> quad(Point<f32>(1, 5), Point<f32>(6, 6), Point<f32>(7, 11), Point<f32>(0, 11));

  const s32 arrayHeight = 15;
  const s32 arrayWidth = 15;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> image(arrayHeight, arrayWidth, scratchCcm);

  const Result result = DrawFilledConvexQuadrilateral<u8>(image, quad, 255);

  ASSERT_TRUE(result == RESULT_OK);

  //image.Print("image");
  //image.Show("image", true, false, false);

  const u8 image_groundTruthData[arrayHeight*arrayWidth] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0,
    255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0,
    255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  Array<u8> image_groundTruth(arrayHeight, arrayWidth, scratchCcm);
  image_groundTruth.Set(image_groundTruthData, arrayHeight*arrayWidth);

  ASSERT_TRUE(AreElementwiseEqual<u8>(image, image_groundTruth));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, DrawQuadrilateral)

#define PRINT_INTRINSICS_GROUND_TRUTH_V(intrinsic, numValues)\
{\
  printf("const u32 " #intrinsic "_groundTruth[" #numValues "] = ");\
  printf("{"); \
  for(s32 i=0; i<numValues; i++) {\
  printf("0x%x, ", intrinsic(values[i]));\
  }\
  printf("};\n");\
}

#define PRINT_INTRINSICS_GROUND_TRUTH_VF(intrinsic, numValues, fixedParameter)\
{\
  printf("const u32 "  #intrinsic "_groundTruth[" #numValues "] = ");\
  printf("{"); \
  for(s32 i=0; i<numValues; i++) {\
  printf("0x%x, ", intrinsic(values[i], fixedParameter));\
  }\
  printf("};\n");\
}

#define PRINT_INTRINSICS_GROUND_TRUTH_VV(intrinsic, numValues)\
{\
  printf("const u32 " #intrinsic "_groundTruth[" #numValues "][" #numValues "] = ");\
  printf("{");\
  for(s32 i=0; i<numValues; i++) {\
  printf("{");\
  for(s32 j=0; j<numValues; j++) {\
  printf("0x%x, ", intrinsic(values[i], values[j]));\
  }\
  printf("}, ");\
  }\
  printf("};\n");\
  printf("const u32 " #intrinsic "_select_groundTruth[" #numValues "][" #numValues "] = ");\
  printf("{");\
  for(s32 i=0; i<numValues; i++) {\
  printf("{");\
  for(s32 j=0; j<numValues; j++) {\
  intrinsic(values[i], values[j]);\
  printf("0x%x, ", __SEL(0x11111111, 0x22222222));\
  }\
  printf("}, ");\
  }\
  printf("};\n");\
}

#define PRINT_INTRINSICS_GROUND_TRUTH_VVF(intrinsic, numValues, fixedParameter)\
{\
  printf("const u32 " #intrinsic "_groundTruth[" #numValues "][" #numValues "] = ");\
  printf("{");\
  for(s32 i=0; i<numValues; i++) {\
  printf("{");\
  for(s32 j=0; j<numValues; j++) {\
  printf("0x%x, ", intrinsic(values[i], values[j], fixedParameter));\
  }\
  printf("}, ");\
  }\
  printf("};\n");\
  printf("const u32 " #intrinsic "_select_groundTruth[" #numValues "][" #numValues "] = ");\
  printf("{");\
  for(s32 i=0; i<numValues; i++) {\
  printf("{");\
  for(s32 j=0; j<numValues; j++) {\
  intrinsic(values[i], values[j], fixedParameter);\
  printf("0x%x, ", __SEL(0x11111111, 0x22222222));\
  }\
  printf("}, ");\
  }\
  printf("};\n");\
}

#define TEST_INTRINSICS_V(intrinsic, numValues)\
{\
  for(s32 i=0; i<numValues; i++) {\
  if(intrinsic(values[i]) != intrinsic ## _groundTruth[i]) {\
  printf(#intrinsic " %d for 0x%x failed. (0x%x != 0x%x)\n", i, values[i], intrinsic(values[i]), intrinsic ## _groundTruth[i]);\
  }\
  ASSERT_TRUE(intrinsic(values[i]) == intrinsic ## _groundTruth[i]);\
  }\
}

#define TEST_INTRINSICS_VF(intrinsic, numValues, fixedParameter)\
{\
  for(s32 i=0; i<numValues; i++) {\
  if(intrinsic(values[i],fixedParameter) != intrinsic ## _groundTruth[i]) {\
  printf(#intrinsic " %d for 0x%x failed. (0x%x != 0x%x)\n", i, values[i], intrinsic(values[i],fixedParameter), intrinsic ## _groundTruth[i]);\
  }\
  ASSERT_TRUE(intrinsic(values[i], fixedParameter) == intrinsic ## _groundTruth[i]);\
  }\
}

#define TEST_INTRINSICS_VV(intrinsic, numValues)\
{\
  for(s32 i=0; i<numValues; i++) {\
  for(s32 j=0; j<numValues; j++) {\
  if(intrinsic(values[i], values[j]) != intrinsic ## _groundTruth[i][j]) {\
  printf(#intrinsic " (%d,%d) for (0x%x,0x%x) failed. (0x%x != 0x%x)\n", i, j, values[i], values[j], intrinsic(values[i], values[j]), intrinsic ## _groundTruth[i][j]);\
  }\
  ASSERT_TRUE(intrinsic(values[i], values[j]) == intrinsic ## _groundTruth[i][j]);\
  const u32 selected = __SEL(0x11111111, 0x22222222);\
  if(selected != intrinsic ## _select_groundTruth[i][j]) {\
  printf(#intrinsic " select (%d,%d) for (0x%x,0x%x) failed. (0x%x != 0x%x)\n", i, j, values[i], values[j], selected, intrinsic ## _select_groundTruth[i][j]);\
  }\
  ASSERT_TRUE(selected == intrinsic ## _select_groundTruth[i][j]);\
  }\
  }\
}

#define TEST_INTRINSICS_VVF(intrinsic, numValues, fixedParameter)\
{\
  for(s32 i=0; i<numValues; i++) {\
  for(s32 j=0; j<numValues; j++) {\
  if(intrinsic(values[i], values[j], fixedParameter) != intrinsic ## _groundTruth[i][j]) {\
  printf(#intrinsic " (%d,%d) for (0x%x,0x%x) failed. (0x%x != 0x%x)\n", i, j, values[i], values[j], intrinsic(values[i], values[j], fixedParameter), intrinsic ## _groundTruth[i][j]);\
  }\
  ASSERT_TRUE(intrinsic(values[i], values[j], fixedParameter) == intrinsic ## _groundTruth[i][j]);\
  const u32 selected = __SEL(0x11111111, 0x22222222);\
  if(selected != intrinsic ## _select_groundTruth[i][j]) {\
  printf(#intrinsic " select (%d,%d) for (0x%x,0x%x) failed. (0x%x != 0x%x)\n", i, j, values[i], values[j], selected, intrinsic ## _select_groundTruth[i][j]);\
  }\
  ASSERT_TRUE(selected == intrinsic ## _select_groundTruth[i][j]);\
  }\
  }\
}

GTEST_TEST(CoreTech_Common, HostIntrinsics_m4)
{
  const s32 numValues = 9;
  const u32 values[numValues] = {0xffffffff, 0x00000000, 0x01010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000};
  const bool printGroundTruth = false;

  if(printGroundTruth) {
    PRINT_INTRINSICS_GROUND_TRUTH_V(__REV, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_V(__REV16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_V(__REVSH, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_V(__RBIT, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VF(__SSAT, numValues, 5);
    PRINT_INTRINSICS_GROUND_TRUTH_VF(__USAT, numValues, 5);
    PRINT_INTRINSICS_GROUND_TRUTH_V(__CLZ, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__SADD8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__QADD8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__SHADD8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UADD8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UQADD8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UHADD8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__SSUB8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__QSUB8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__SHSUB8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__USUB8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UQSUB8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UHSUB8, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__SADD16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__QADD16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__SHADD16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UADD16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UQADD16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UHADD16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__SSUB16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__QSUB16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__USUB16, numValues);
    PRINT_INTRINSICS_GROUND_TRUTH_VV(__UQSUB16, numValues);
    //PRINT_INTRINSICS_GROUND_TRUTH_VV(__USAD8, numValues); // TODO: verify implementation before using
    PRINT_INTRINSICS_GROUND_TRUTH_VVF(__SMLAD, numValues, 5);
  } // if(printGroundTruth)

  const u32 __REV_groundTruth[numValues] = {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0xff7fff7f, 0x800080, 0xffffff7f, 0x80, };
  const u32 __REV16_groundTruth[numValues] = {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0xff7fff7f, 0x800080, 0xff7fffff, 0x800000, };
  const u32 __REVSH_groundTruth[numValues] = {0xffffffff, 0x0, 0x101, 0x7f7f, 0xffff8080, 0xffffff7f, 0x80, 0xffffffff, 0x0, };
  const u32 __RBIT_groundTruth[numValues] = {0xffffffff, 0x0, 0x80808080, 0xfefefefe, 0x1010101, 0xfffefffe, 0x10001, 0xfffffffe, 0x1, };
  const u32 __SSAT_groundTruth[numValues] = {0xffffffff, 0x0, 0xf, 0xf, 0xfffffff0, 0xf, 0xfffffff0, 0xf, 0xfffffff0, };
  const u32 __USAT_groundTruth[numValues] = {0x0, 0x0, 0x1f, 0x1f, 0x0, 0x1f, 0x0, 0x1f, 0x0, };
  const u32 __CLZ_groundTruth[numValues] = {0x0, 0x20, 0x7, 0x1, 0x0, 0x1, 0x0, 0x1, 0x0, };
  const u32 __SADD8_groundTruth[numValues][numValues] = {{0xfefefefe, 0xffffffff, 0x0, 0x7e7e7e7e, 0x7f7f7f7f, 0x7efe7efe, 0x7fff7fff, 0x7efefefe, 0x7fffffff, }, {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000, }, {0x0, 0x1010101, 0x2020202, 0x80808080, 0x81818181, 0x80008000, 0x81018101, 0x80000000, 0x81010101, }, {0x7e7e7e7e, 0x7f7f7f7f, 0x80808080, 0xfefefefe, 0xffffffff, 0xfe7efe7e, 0xff7fff7f, 0xfe7e7e7e, 0xff7f7f7f, }, {0x7f7f7f7f, 0x80808080, 0x81818181, 0xffffffff, 0x0, 0xff7fff7f, 0x800080, 0xff7f7f7f, 0x808080, }, {0x7efe7efe, 0x7fff7fff, 0x80008000, 0xfe7efe7e, 0xff7fff7f, 0xfefefefe, 0xffffffff, 0xfefe7efe, 0xffff7fff, }, {0x7fff7fff, 0x80008000, 0x81018101, 0xff7fff7f, 0x800080, 0xffffffff, 0x0, 0xffff7fff, 0x8000, }, {0x7efefefe, 0x7fffffff, 0x80000000, 0xfe7e7e7e, 0xff7f7f7f, 0xfefe7efe, 0xffff7fff, 0xfefefefe, 0xffffffff, }, {0x7fffffff, 0x80000000, 0x81010101, 0xff7f7f7f, 0x808080, 0xffff7fff, 0x8000, 0xffffffff, 0x0, }, };
  const u32 __SADD8_select_groundTruth[numValues][numValues] = {{0x22222222, 0x22222222, 0x11111111, 0x11111111, 0x22222222, 0x11221122, 0x22222222, 0x11222222, 0x22222222, }, {0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11221122, 0x22112211, 0x11222222, 0x22111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22112211, 0x11111111, 0x22111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22112211, 0x11111111, 0x22111111, }, {0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, }, {0x11221122, 0x11221122, 0x11111111, 0x11111111, 0x22222222, 0x11221122, 0x22222222, 0x11221122, 0x22221122, }, {0x22222222, 0x22112211, 0x22112211, 0x22112211, 0x22222222, 0x22222222, 0x22112211, 0x22222222, 0x22112211, }, {0x11222222, 0x11222222, 0x11111111, 0x11111111, 0x22222222, 0x11221122, 0x22222222, 0x11222222, 0x22222222, }, {0x22222222, 0x22111111, 0x22111111, 0x22111111, 0x22222222, 0x22221122, 0x22112211, 0x22222222, 0x22111111, }, };
  const u32 __QADD8_groundTruth[numValues][numValues] = {{0xfefefefe, 0xffffffff, 0x0, 0x7e7e7e7e, 0x80808080, 0x7efe7efe, 0x80ff80ff, 0x7efefefe, 0x80ffffff, }, {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000, }, {0x0, 0x1010101, 0x2020202, 0x7f7f7f7f, 0x81818181, 0x7f007f00, 0x81018101, 0x7f000000, 0x81010101, }, {0x7e7e7e7e, 0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f, 0xffffffff, 0x7f7e7f7e, 0xff7fff7f, 0x7f7e7e7e, 0xff7f7f7f, }, {0x80808080, 0x80808080, 0x81818181, 0xffffffff, 0x80808080, 0xff80ff80, 0x80808080, 0xff808080, 0x80808080, }, {0x7efe7efe, 0x7fff7fff, 0x7f007f00, 0x7f7e7f7e, 0xff80ff80, 0x7ffe7ffe, 0xffffffff, 0x7ffe7efe, 0xffff7fff, }, {0x80ff80ff, 0x80008000, 0x81018101, 0xff7fff7f, 0x80808080, 0xffffffff, 0x80008000, 0xffff80ff, 0x80008000, }, {0x7efefefe, 0x7fffffff, 0x7f000000, 0x7f7e7e7e, 0xff808080, 0x7ffe7efe, 0xffff80ff, 0x7ffefefe, 0xffffffff, }, {0x80ffffff, 0x80000000, 0x81010101, 0xff7f7f7f, 0x80808080, 0xffff7fff, 0x80008000, 0xffffffff, 0x80000000, }, };
  const u32 __QADD8_select_groundTruth[numValues][numValues] = {{0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, };
  const u32 __SHADD8_groundTruth[numValues][numValues] = {{0xffffffff, 0xffffffff, 0x0, 0x3f3f3f3f, 0xbfbfbfbf, 0x3fff3fff, 0xbfffbfff, 0x3fffffff, 0xbfffffff, }, {0xffffffff, 0x0, 0x0, 0x3f3f3f3f, 0xc0c0c0c0, 0x3fff3fff, 0xc000c000, 0x3fffffff, 0xc0000000, }, {0x0, 0x0, 0x1010101, 0x40404040, 0xc0c0c0c0, 0x40004000, 0xc000c000, 0x40000000, 0xc0000000, }, {0x3f3f3f3f, 0x3f3f3f3f, 0x40404040, 0x7f7f7f7f, 0xffffffff, 0x7f3f7f3f, 0xff3fff3f, 0x7f3f3f3f, 0xff3f3f3f, }, {0xbfbfbfbf, 0xc0c0c0c0, 0xc0c0c0c0, 0xffffffff, 0x80808080, 0xffbfffbf, 0x80c080c0, 0xffbfbfbf, 0x80c0c0c0, }, {0x3fff3fff, 0x3fff3fff, 0x40004000, 0x7f3f7f3f, 0xffbfffbf, 0x7fff7fff, 0xffffffff, 0x7fff3fff, 0xffff3fff, }, {0xbfffbfff, 0xc000c000, 0xc000c000, 0xff3fff3f, 0x80c080c0, 0xffffffff, 0x80008000, 0xffffbfff, 0x8000c000, }, {0x3fffffff, 0x3fffffff, 0x40000000, 0x7f3f3f3f, 0xffbfbfbf, 0x7fff3fff, 0xffffbfff, 0x7fffffff, 0xffffffff, }, {0xbfffffff, 0xc0000000, 0xc0000000, 0xff3f3f3f, 0x80c0c0c0, 0xffff3fff, 0x8000c000, 0xffffffff, 0x80000000, }, };
  const u32 __SHADD8_select_groundTruth[numValues][numValues] = {{0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, {0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22111111, }, };
  const u32 __UADD8_groundTruth[numValues][numValues] = {{0xfefefefe, 0xffffffff, 0x0, 0x7e7e7e7e, 0x7f7f7f7f, 0x7efe7efe, 0x7fff7fff, 0x7efefefe, 0x7fffffff, }, {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000, }, {0x0, 0x1010101, 0x2020202, 0x80808080, 0x81818181, 0x80008000, 0x81018101, 0x80000000, 0x81010101, }, {0x7e7e7e7e, 0x7f7f7f7f, 0x80808080, 0xfefefefe, 0xffffffff, 0xfe7efe7e, 0xff7fff7f, 0xfe7e7e7e, 0xff7f7f7f, }, {0x7f7f7f7f, 0x80808080, 0x81818181, 0xffffffff, 0x0, 0xff7fff7f, 0x800080, 0xff7f7f7f, 0x808080, }, {0x7efe7efe, 0x7fff7fff, 0x80008000, 0xfe7efe7e, 0xff7fff7f, 0xfefefefe, 0xffffffff, 0xfefe7efe, 0xffff7fff, }, {0x7fff7fff, 0x80008000, 0x81018101, 0xff7fff7f, 0x800080, 0xffffffff, 0x0, 0xffff7fff, 0x8000, }, {0x7efefefe, 0x7fffffff, 0x80000000, 0xfe7e7e7e, 0xff7f7f7f, 0xfefe7efe, 0xffff7fff, 0xfefefefe, 0xffffffff, }, {0x7fffffff, 0x80000000, 0x81010101, 0xff7f7f7f, 0x808080, 0xffff7fff, 0x8000, 0xffffffff, 0x0, }, };
  const u32 __UADD8_select_groundTruth[numValues][numValues] = {{0x11111111, 0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11221122, 0x11111111, 0x11222222, }, {0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, }, {0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22112211, 0x22222222, 0x22111111, 0x22222222, }, {0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22112211, 0x22222222, 0x22111111, 0x22222222, }, {0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22112211, 0x11221122, 0x22111111, 0x11222222, }, {0x11111111, 0x22222222, 0x22112211, 0x22112211, 0x22112211, 0x22112211, 0x22222222, 0x22111111, 0x22222222, }, {0x11221122, 0x22222222, 0x22222222, 0x22222222, 0x11221122, 0x22222222, 0x11221122, 0x22221122, 0x11222222, }, {0x11111111, 0x22222222, 0x22111111, 0x22111111, 0x22111111, 0x22111111, 0x22221122, 0x22111111, 0x22222222, }, {0x11222222, 0x22222222, 0x22222222, 0x22222222, 0x11222222, 0x22222222, 0x11222222, 0x22222222, 0x11222222, }, };
  const u32 __UQADD8_groundTruth[numValues][numValues] = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, }, {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000, }, {0xffffffff, 0x1010101, 0x2020202, 0x80808080, 0x81818181, 0x80ff80ff, 0x81018101, 0x80ffffff, 0x81010101, }, {0xffffffff, 0x7f7f7f7f, 0x80808080, 0xfefefefe, 0xffffffff, 0xfefffeff, 0xff7fff7f, 0xfeffffff, 0xff7f7f7f, }, {0xffffffff, 0x80808080, 0x81818181, 0xffffffff, 0xffffffff, 0xffffffff, 0xff80ff80, 0xffffffff, 0xff808080, }, {0xffffffff, 0x7fff7fff, 0x80ff80ff, 0xfefffeff, 0xffffffff, 0xfefffeff, 0xffffffff, 0xfeffffff, 0xffff7fff, }, {0xffffffff, 0x80008000, 0x81018101, 0xff7fff7f, 0xff80ff80, 0xffffffff, 0xff00ff00, 0xffffffff, 0xff008000, }, {0xffffffff, 0x7fffffff, 0x80ffffff, 0xfeffffff, 0xffffffff, 0xfeffffff, 0xffffffff, 0xfeffffff, 0xffffffff, }, {0xffffffff, 0x80000000, 0x81010101, 0xff7f7f7f, 0xff808080, 0xffff7fff, 0xff008000, 0xffffffff, 0xff000000, }, };
  const u32 __UQADD8_select_groundTruth[numValues][numValues] = {{0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, };
  const u32 __UHADD8_groundTruth[numValues][numValues] = {{0xffffffff, 0x7f7f7f7f, 0x80808080, 0xbfbfbfbf, 0xbfbfbfbf, 0xbfffbfff, 0xbf7fbf7f, 0xbfffffff, 0xbf7f7f7f, }, {0x7f7f7f7f, 0x0, 0x0, 0x3f3f3f3f, 0x40404040, 0x3f7f3f7f, 0x40004000, 0x3f7f7f7f, 0x40000000, }, {0x80808080, 0x0, 0x1010101, 0x40404040, 0x40404040, 0x40804080, 0x40004000, 0x40808080, 0x40000000, }, {0xbfbfbfbf, 0x3f3f3f3f, 0x40404040, 0x7f7f7f7f, 0x7f7f7f7f, 0x7fbf7fbf, 0x7f3f7f3f, 0x7fbfbfbf, 0x7f3f3f3f, }, {0xbfbfbfbf, 0x40404040, 0x40404040, 0x7f7f7f7f, 0x80808080, 0x7fbf7fbf, 0x80408040, 0x7fbfbfbf, 0x80404040, }, {0xbfffbfff, 0x3f7f3f7f, 0x40804080, 0x7fbf7fbf, 0x7fbf7fbf, 0x7fff7fff, 0x7f7f7f7f, 0x7fffbfff, 0x7f7f3f7f, }, {0xbf7fbf7f, 0x40004000, 0x40004000, 0x7f3f7f3f, 0x80408040, 0x7f7f7f7f, 0x80008000, 0x7f7fbf7f, 0x80004000, }, {0xbfffffff, 0x3f7f7f7f, 0x40808080, 0x7fbfbfbf, 0x7fbfbfbf, 0x7fffbfff, 0x7f7fbf7f, 0x7fffffff, 0x7f7f7f7f, }, {0xbf7f7f7f, 0x40000000, 0x40000000, 0x7f3f3f3f, 0x80404040, 0x7f7f3f7f, 0x80004000, 0x7f7f7f7f, 0x80000000, }, };
  const u32 __UHADD8_select_groundTruth[numValues][numValues] = {{0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, {0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11222222, }, };
  const u32 __SSUB8_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xfefefefe, 0x80808080, 0x7f7f7f7f, 0x80008000, 0x7fff7fff, 0x80000000, 0x7fffffff, }, {0x1010101, 0x0, 0xffffffff, 0x81818181, 0x80808080, 0x81018101, 0x80008000, 0x81010101, 0x80000000, }, {0x2020202, 0x1010101, 0x0, 0x82828282, 0x81818181, 0x82028202, 0x81018101, 0x82020202, 0x81010101, }, {0x80808080, 0x7f7f7f7f, 0x7e7e7e7e, 0x0, 0xffffffff, 0x800080, 0xff7fff7f, 0x808080, 0xff7f7f7f, }, {0x81818181, 0x80808080, 0x7f7f7f7f, 0x1010101, 0x0, 0x1810181, 0x800080, 0x1818181, 0x808080, }, {0x80008000, 0x7fff7fff, 0x7efe7efe, 0x800080, 0xff7fff7f, 0x0, 0xffffffff, 0x8000, 0xffff7fff, }, {0x81018101, 0x80008000, 0x7fff7fff, 0x1810181, 0x800080, 0x1010101, 0x0, 0x1018101, 0x8000, }, {0x80000000, 0x7fffffff, 0x7efefefe, 0x808080, 0xff7f7f7f, 0x8000, 0xffff7fff, 0x0, 0xffffffff, }, {0x81010101, 0x80000000, 0x7fffffff, 0x1818181, 0x808080, 0x1018101, 0x8000, 0x1010101, 0x0, }, };
  const u32 __SSUB8_select_groundTruth[numValues][numValues] = {{0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22112211, 0x11221122, 0x22111111, 0x11222222, }, {0x11111111, 0x11111111, 0x22222222, 0x22222222, 0x11111111, 0x22112211, 0x11111111, 0x22111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22112211, 0x11111111, 0x22111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22222222, 0x11221122, 0x22222222, 0x11222222, }, {0x11111111, 0x11221122, 0x11221122, 0x11221122, 0x11111111, 0x11111111, 0x11221122, 0x11111111, 0x11221122, }, {0x22112211, 0x22112211, 0x22222222, 0x22222222, 0x11111111, 0x22112211, 0x11111111, 0x22112211, 0x11112211, }, {0x11111111, 0x11222222, 0x11222222, 0x11222222, 0x11111111, 0x11112211, 0x11221122, 0x11111111, 0x11222222, }, {0x22111111, 0x22111111, 0x22222222, 0x22222222, 0x11111111, 0x22112211, 0x11111111, 0x22111111, 0x11111111, }, };
  const u32 __QSUB8_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xfefefefe, 0x80808080, 0x7f7f7f7f, 0x80008000, 0x7fff7fff, 0x80000000, 0x7fffffff, }, {0x1010101, 0x0, 0xffffffff, 0x81818181, 0x7f7f7f7f, 0x81018101, 0x7f007f00, 0x81010101, 0x7f000000, }, {0x2020202, 0x1010101, 0x0, 0x82828282, 0x7f7f7f7f, 0x82028202, 0x7f017f01, 0x82020202, 0x7f010101, }, {0x7f7f7f7f, 0x7f7f7f7f, 0x7e7e7e7e, 0x0, 0x7f7f7f7f, 0x7f007f, 0x7f7f7f7f, 0x7f7f7f, 0x7f7f7f7f, }, {0x81818181, 0x80808080, 0x80808080, 0x80808080, 0x0, 0x80818081, 0x800080, 0x80818181, 0x808080, }, {0x7f007f00, 0x7fff7fff, 0x7efe7efe, 0x800080, 0x7f7f7f7f, 0x0, 0x7fff7fff, 0x7f00, 0x7fff7fff, }, {0x81018101, 0x80008000, 0x80ff80ff, 0x80818081, 0x7f007f, 0x80018001, 0x0, 0x80018101, 0x8000, }, {0x7f000000, 0x7fffffff, 0x7efefefe, 0x808080, 0x7f7f7f7f, 0x8000, 0x7fff7fff, 0x0, 0x7fffffff, }, {0x81010101, 0x80000000, 0x80ffffff, 0x80818181, 0x7f7f7f, 0x80018101, 0x7f00, 0x80010101, 0x0, }, };
  const u32 __QSUB8_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, };
  const u32 __SHSUB8_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xffffffff, 0xc0c0c0c0, 0x3f3f3f3f, 0xc000c000, 0x3fff3fff, 0xc0000000, 0x3fffffff, }, {0x0, 0x0, 0xffffffff, 0xc0c0c0c0, 0x40404040, 0xc000c000, 0x40004000, 0xc0000000, 0x40000000, }, {0x1010101, 0x0, 0x0, 0xc1c1c1c1, 0x40404040, 0xc101c101, 0x40004000, 0xc1010101, 0x40000000, }, {0x40404040, 0x3f3f3f3f, 0x3f3f3f3f, 0x0, 0x7f7f7f7f, 0x400040, 0x7f3f7f3f, 0x404040, 0x7f3f3f3f, }, {0xc0c0c0c0, 0xc0c0c0c0, 0xbfbfbfbf, 0x80808080, 0x0, 0x80c080c0, 0xc000c0, 0x80c0c0c0, 0xc0c0c0, }, {0x40004000, 0x3fff3fff, 0x3fff3fff, 0xc000c0, 0x7f3f7f3f, 0x0, 0x7fff7fff, 0x4000, 0x7fff3fff, }, {0xc000c000, 0xc000c000, 0xbfffbfff, 0x80c080c0, 0x400040, 0x80008000, 0x0, 0x8000c000, 0xc000, }, {0x40000000, 0x3fffffff, 0x3fffffff, 0xc0c0c0, 0x7f3f3f3f, 0xc000, 0x7fff3fff, 0x0, 0x7fffffff, }, {0xc0000000, 0xc0000000, 0xbfffffff, 0x80c0c0c0, 0x404040, 0x8000c000, 0x4000, 0x80000000, 0x0, }, };
  const u32 __SHSUB8_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, };
  const u32 __USUB8_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xfefefefe, 0x80808080, 0x7f7f7f7f, 0x80008000, 0x7fff7fff, 0x80000000, 0x7fffffff, }, {0x1010101, 0x0, 0xffffffff, 0x81818181, 0x80808080, 0x81018101, 0x80008000, 0x81010101, 0x80000000, }, {0x2020202, 0x1010101, 0x0, 0x82828282, 0x81818181, 0x82028202, 0x81018101, 0x82020202, 0x81010101, }, {0x80808080, 0x7f7f7f7f, 0x7e7e7e7e, 0x0, 0xffffffff, 0x800080, 0xff7fff7f, 0x808080, 0xff7f7f7f, }, {0x81818181, 0x80808080, 0x7f7f7f7f, 0x1010101, 0x0, 0x1810181, 0x800080, 0x1818181, 0x808080, }, {0x80008000, 0x7fff7fff, 0x7efe7efe, 0x800080, 0xff7fff7f, 0x0, 0xffffffff, 0x8000, 0xffff7fff, }, {0x81018101, 0x80008000, 0x7fff7fff, 0x1810181, 0x800080, 0x1010101, 0x0, 0x1018101, 0x8000, }, {0x80000000, 0x7fffffff, 0x7efefefe, 0x808080, 0xff7f7f7f, 0x8000, 0xffff7fff, 0x0, 0xffffffff, }, {0x81010101, 0x80000000, 0x7fffffff, 0x1818181, 0x808080, 0x1018101, 0x8000, 0x1010101, 0x0, }, };
  const u32 __USUB8_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x22222222, 0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22112211, 0x22222222, 0x22111111, }, {0x22222222, 0x11111111, 0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22112211, 0x22222222, 0x22111111, }, {0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11221122, 0x22112211, 0x11222222, 0x22111111, }, {0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11221122, 0x11111111, 0x11222222, 0x11111111, }, {0x22112211, 0x11111111, 0x11111111, 0x11111111, 0x22112211, 0x11111111, 0x22112211, 0x11112211, 0x22111111, }, {0x22222222, 0x11111111, 0x11221122, 0x11221122, 0x11221122, 0x11221122, 0x11111111, 0x11222222, 0x11111111, }, {0x22111111, 0x11111111, 0x11111111, 0x11111111, 0x22111111, 0x11111111, 0x22111111, 0x11111111, 0x22111111, }, {0x22222222, 0x11111111, 0x11222222, 0x11222222, 0x11222222, 0x11222222, 0x11112211, 0x11222222, 0x11111111, }, };
  const u32 __UQSUB8_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xfefefefe, 0x80808080, 0x7f7f7f7f, 0x80008000, 0x7fff7fff, 0x80000000, 0x7fffffff, }, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, }, {0x0, 0x1010101, 0x0, 0x0, 0x0, 0x0, 0x10001, 0x0, 0x10101, }, {0x0, 0x7f7f7f7f, 0x7e7e7e7e, 0x0, 0x0, 0x0, 0x7f007f, 0x0, 0x7f7f7f, }, {0x0, 0x80808080, 0x7f7f7f7f, 0x1010101, 0x0, 0x1000100, 0x800080, 0x1000000, 0x808080, }, {0x0, 0x7fff7fff, 0x7efe7efe, 0x800080, 0x7f007f, 0x0, 0xff00ff, 0x0, 0xff7fff, }, {0x0, 0x80008000, 0x7f007f00, 0x1000100, 0x0, 0x1000100, 0x0, 0x1000000, 0x8000, }, {0x0, 0x7fffffff, 0x7efefefe, 0x808080, 0x7f7f7f, 0x8000, 0xff7fff, 0x0, 0xffffff, }, {0x0, 0x80000000, 0x7f000000, 0x1000000, 0x0, 0x1000000, 0x0, 0x1000000, 0x0, }, };
  const u32 __UQSUB8_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, };
  const u32 __UHSUB8_groundTruth[numValues][numValues] = {{0x0, 0x7f7f7f7f, 0x7f7f7f7f, 0x40404040, 0x3f3f3f3f, 0x40004000, 0x3f7f3f7f, 0x40000000, 0x3f7f7f7f, }, {0x80808080, 0x0, 0xffffffff, 0xc0c0c0c0, 0xc0c0c0c0, 0xc080c080, 0xc000c000, 0xc0808080, 0xc0000000, }, {0x81818181, 0x0, 0x0, 0xc1c1c1c1, 0xc0c0c0c0, 0xc181c181, 0xc000c000, 0xc1818181, 0xc0000000, }, {0xc0c0c0c0, 0x3f3f3f3f, 0x3f3f3f3f, 0x0, 0xffffffff, 0xc000c0, 0xff3fff3f, 0xc0c0c0, 0xff3f3f3f, }, {0xc0c0c0c0, 0x40404040, 0x3f3f3f3f, 0x0, 0x0, 0xc000c0, 0x400040, 0xc0c0c0, 0x404040, }, {0xc000c000, 0x3f7f3f7f, 0x3f7f3f7f, 0x400040, 0xff3fff3f, 0x0, 0xff7fff7f, 0xc000, 0xff7f3f7f, }, {0xc080c080, 0x40004000, 0x3fff3fff, 0xc000c0, 0xc000c0, 0x800080, 0x0, 0x80c080, 0x4000, }, {0xc0000000, 0x3f7f7f7f, 0x3f7f7f7f, 0x404040, 0xff3f3f3f, 0x4000, 0xff7f3f7f, 0x0, 0xff7f7f7f, }, {0xc0808080, 0x40000000, 0x3fffffff, 0xc0c0c0, 0xc0c0c0, 0x80c080, 0xc000, 0x808080, 0x0, }, };
  const u32 __UHSUB8_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, };
  const u32 __SADD16_groundTruth[numValues][numValues] = {{0xfffefffe, 0xffffffff, 0x1000100, 0x7f7e7f7e, 0x807f807f, 0x7ffe7ffe, 0x7fff7fff, 0x7ffefffe, 0x7fffffff, }, {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000, }, {0x1000100, 0x1010101, 0x2020202, 0x80808080, 0x81818181, 0x81008100, 0x81018101, 0x81000100, 0x81010101, }, {0x7f7e7f7e, 0x7f7f7f7f, 0x80808080, 0xfefefefe, 0xffffffff, 0xff7eff7e, 0xff7fff7f, 0xff7e7f7e, 0xff7f7f7f, }, {0x807f807f, 0x80808080, 0x81818181, 0xffffffff, 0x1000100, 0x7f007f, 0x800080, 0x7f807f, 0x808080, }, {0x7ffe7ffe, 0x7fff7fff, 0x81008100, 0xff7eff7e, 0x7f007f, 0xfffefffe, 0xffffffff, 0xfffe7ffe, 0xffff7fff, }, {0x7fff7fff, 0x80008000, 0x81018101, 0xff7fff7f, 0x800080, 0xffffffff, 0x0, 0xffff7fff, 0x8000, }, {0x7ffefffe, 0x7fffffff, 0x81000100, 0xff7e7f7e, 0x7f807f, 0xfffe7ffe, 0xffff7fff, 0xfffefffe, 0xffffffff, }, {0x7fffffff, 0x80000000, 0x81010101, 0xff7f7f7f, 0x808080, 0xffff7fff, 0x8000, 0xffffffff, 0x0, }, };
  const u32 __SADD16_select_groundTruth[numValues][numValues] = {{0x22222222, 0x22222222, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22222222, 0x11112222, 0x22222222, }, {0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22222222, 0x11112222, 0x22221111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22222222, 0x11111111, 0x22221111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22222222, 0x11111111, 0x22221111, }, {0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22222222, 0x11112222, 0x22222222, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22221111, }, {0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, }, {0x11112222, 0x11112222, 0x11111111, 0x11111111, 0x11112222, 0x11111111, 0x22222222, 0x11112222, 0x22222222, }, {0x22222222, 0x22221111, 0x22221111, 0x22221111, 0x22222222, 0x22221111, 0x22222222, 0x22222222, 0x22221111, }, };
  const u32 __QADD16_groundTruth[numValues][numValues] = {{0xfffefffe, 0xffffffff, 0x1000100, 0x7f7e7f7e, 0x807f807f, 0x7ffe7ffe, 0x80008000, 0x7ffefffe, 0x8000ffff, }, {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000, }, {0x1000100, 0x1010101, 0x2020202, 0x7fff7fff, 0x81818181, 0x7fff7fff, 0x81018101, 0x7fff0100, 0x81010101, }, {0x7f7e7f7e, 0x7f7f7f7f, 0x7fff7fff, 0x7fff7fff, 0xffffffff, 0x7fff7fff, 0xff7fff7f, 0x7fff7f7e, 0xff7f7f7f, }, {0x807f807f, 0x80808080, 0x81818181, 0xffffffff, 0x80008000, 0x7f007f, 0x80008000, 0x7f807f, 0x80008080, }, {0x7ffe7ffe, 0x7fff7fff, 0x7fff7fff, 0x7fff7fff, 0x7f007f, 0x7fff7fff, 0xffffffff, 0x7fff7ffe, 0xffff7fff, }, {0x80008000, 0x80008000, 0x81018101, 0xff7fff7f, 0x80008000, 0xffffffff, 0x80008000, 0xffff8000, 0x80008000, }, {0x7ffefffe, 0x7fffffff, 0x7fff0100, 0x7fff7f7e, 0x7f807f, 0x7fff7ffe, 0xffff8000, 0x7ffffffe, 0xffffffff, }, {0x8000ffff, 0x80000000, 0x81010101, 0xff7f7f7f, 0x80008080, 0xffff7fff, 0x80008000, 0xffffffff, 0x80000000, }, };
  const u32 __QADD16_select_groundTruth[numValues][numValues] = {{0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, };
  const u32 __SHADD16_groundTruth[numValues][numValues] = {{0xffffffff, 0xffffffff, 0x800080, 0x3fbf3fbf, 0xc03fc03f, 0x3fff3fff, 0xbfffbfff, 0x3fffffff, 0xbfffffff, }, {0xffffffff, 0x0, 0x800080, 0x3fbf3fbf, 0xc040c040, 0x3fff3fff, 0xc000c000, 0x3fffffff, 0xc0000000, }, {0x800080, 0x800080, 0x1010101, 0x40404040, 0xc0c0c0c0, 0x40804080, 0xc080c080, 0x40800080, 0xc0800080, }, {0x3fbf3fbf, 0x3fbf3fbf, 0x40404040, 0x7f7f7f7f, 0xffffffff, 0x7fbf7fbf, 0xffbfffbf, 0x7fbf3fbf, 0xffbf3fbf, }, {0xc03fc03f, 0xc040c040, 0xc0c0c0c0, 0xffffffff, 0x80808080, 0x3f003f, 0x80408040, 0x3fc03f, 0x8040c040, }, {0x3fff3fff, 0x3fff3fff, 0x40804080, 0x7fbf7fbf, 0x3f003f, 0x7fff7fff, 0xffffffff, 0x7fff3fff, 0xffff3fff, }, {0xbfffbfff, 0xc000c000, 0xc080c080, 0xffbfffbf, 0x80408040, 0xffffffff, 0x80008000, 0xffffbfff, 0x8000c000, }, {0x3fffffff, 0x3fffffff, 0x40800080, 0x7fbf3fbf, 0x3fc03f, 0x7fff3fff, 0xffffbfff, 0x7fffffff, 0xffffffff, }, {0xbfffffff, 0xc0000000, 0xc0800080, 0xffbf3fbf, 0x8040c040, 0xffff3fff, 0x8000c000, 0xffffffff, 0x80000000, }, };
  const u32 __SHADD16_select_groundTruth[numValues][numValues] = {{0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, {0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, 0x22221111, }, };
  const u32 __UADD16_groundTruth[numValues][numValues] = {{0xfffefffe, 0xffffffff, 0x1000100, 0x7f7e7f7e, 0x807f807f, 0x7ffe7ffe, 0x7fff7fff, 0x7ffefffe, 0x7fffffff, }, {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000, }, {0x1000100, 0x1010101, 0x2020202, 0x80808080, 0x81818181, 0x81008100, 0x81018101, 0x81000100, 0x81010101, }, {0x7f7e7f7e, 0x7f7f7f7f, 0x80808080, 0xfefefefe, 0xffffffff, 0xff7eff7e, 0xff7fff7f, 0xff7e7f7e, 0xff7f7f7f, }, {0x807f807f, 0x80808080, 0x81818181, 0xffffffff, 0x1000100, 0x7f007f, 0x800080, 0x7f807f, 0x808080, }, {0x7ffe7ffe, 0x7fff7fff, 0x81008100, 0xff7eff7e, 0x7f007f, 0xfffefffe, 0xffffffff, 0xfffe7ffe, 0xffff7fff, }, {0x7fff7fff, 0x80008000, 0x81018101, 0xff7fff7f, 0x800080, 0xffffffff, 0x0, 0xffff7fff, 0x8000, }, {0x7ffefffe, 0x7fffffff, 0x81000100, 0xff7e7f7e, 0x7f807f, 0xfffe7ffe, 0xffff7fff, 0xfffefffe, 0xffffffff, }, {0x7fffffff, 0x80000000, 0x81010101, 0xff7f7f7f, 0x808080, 0xffff7fff, 0x8000, 0xffffffff, 0x0, }, };
  const u32 __UADD16_select_groundTruth[numValues][numValues] = {{0x11111111, 0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11112222, }, {0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, }, {0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22221111, 0x22222222, }, {0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22221111, 0x22222222, }, {0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11112222, }, {0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22222222, 0x22222222, 0x22221111, 0x22222222, }, {0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22222222, 0x11111111, 0x22221111, 0x11112222, }, {0x11111111, 0x22222222, 0x22221111, 0x22221111, 0x11111111, 0x22221111, 0x22221111, 0x22221111, 0x22222222, }, {0x11112222, 0x22222222, 0x22222222, 0x22222222, 0x11112222, 0x22222222, 0x11112222, 0x22222222, 0x11112222, }, };
  const u32 __UQADD16_groundTruth[numValues][numValues] = {{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, }, {0xffffffff, 0x0, 0x1010101, 0x7f7f7f7f, 0x80808080, 0x7fff7fff, 0x80008000, 0x7fffffff, 0x80000000, }, {0xffffffff, 0x1010101, 0x2020202, 0x80808080, 0x81818181, 0x81008100, 0x81018101, 0x8100ffff, 0x81010101, }, {0xffffffff, 0x7f7f7f7f, 0x80808080, 0xfefefefe, 0xffffffff, 0xff7eff7e, 0xff7fff7f, 0xff7effff, 0xff7f7f7f, }, {0xffffffff, 0x80808080, 0x81818181, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffff8080, }, {0xffffffff, 0x7fff7fff, 0x81008100, 0xff7eff7e, 0xffffffff, 0xfffefffe, 0xffffffff, 0xfffeffff, 0xffff7fff, }, {0xffffffff, 0x80008000, 0x81018101, 0xff7fff7f, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffff8000, }, {0xffffffff, 0x7fffffff, 0x8100ffff, 0xff7effff, 0xffffffff, 0xfffeffff, 0xffffffff, 0xfffeffff, 0xffffffff, }, {0xffffffff, 0x80000000, 0x81010101, 0xff7f7f7f, 0xffff8080, 0xffff7fff, 0xffff8000, 0xffffffff, 0xffff0000, }, };
  const u32 __UQADD16_select_groundTruth[numValues][numValues] = {{0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, };
  const u32 __UHADD16_groundTruth[numValues][numValues] = {{0xffffffff, 0x7fff7fff, 0x80808080, 0xbfbfbfbf, 0xc03fc03f, 0xbfffbfff, 0xbfffbfff, 0xbfffffff, 0xbfff7fff, }, {0x7fff7fff, 0x0, 0x800080, 0x3fbf3fbf, 0x40404040, 0x3fff3fff, 0x40004000, 0x3fff7fff, 0x40000000, }, {0x80808080, 0x800080, 0x1010101, 0x40404040, 0x40c040c0, 0x40804080, 0x40804080, 0x40808080, 0x40800080, }, {0xbfbfbfbf, 0x3fbf3fbf, 0x40404040, 0x7f7f7f7f, 0x7fff7fff, 0x7fbf7fbf, 0x7fbf7fbf, 0x7fbfbfbf, 0x7fbf3fbf, }, {0xc03fc03f, 0x40404040, 0x40c040c0, 0x7fff7fff, 0x80808080, 0x803f803f, 0x80408040, 0x803fc03f, 0x80404040, }, {0xbfffbfff, 0x3fff3fff, 0x40804080, 0x7fbf7fbf, 0x803f803f, 0x7fff7fff, 0x7fff7fff, 0x7fffbfff, 0x7fff3fff, }, {0xbfffbfff, 0x40004000, 0x40804080, 0x7fbf7fbf, 0x80408040, 0x7fff7fff, 0x80008000, 0x7fffbfff, 0x80004000, }, {0xbfffffff, 0x3fff7fff, 0x40808080, 0x7fbfbfbf, 0x803fc03f, 0x7fffbfff, 0x7fffbfff, 0x7fffffff, 0x7fff7fff, }, {0xbfff7fff, 0x40000000, 0x40800080, 0x7fbf3fbf, 0x80404040, 0x7fff3fff, 0x80004000, 0x7fff7fff, 0x80000000, }, };
  const u32 __UHADD16_select_groundTruth[numValues][numValues] = {{0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, {0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, 0x11112222, }, };
  const u32 __SSUB16_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xfefefefe, 0x80808080, 0x7f7f7f7f, 0x80008000, 0x7fff7fff, 0x80000000, 0x7fffffff, }, {0x10001, 0x0, 0xfefffeff, 0x80818081, 0x7f807f80, 0x80018001, 0x80008000, 0x80010001, 0x80000000, }, {0x1020102, 0x1010101, 0x0, 0x81828182, 0x80818081, 0x81028102, 0x81018101, 0x81020102, 0x81010101, }, {0x7f807f80, 0x7f7f7f7f, 0x7e7e7e7e, 0x0, 0xfefffeff, 0xff80ff80, 0xff7fff7f, 0xff807f80, 0xff7f7f7f, }, {0x80818081, 0x80808080, 0x7f7f7f7f, 0x1010101, 0x0, 0x810081, 0x800080, 0x818081, 0x808080, }, {0x80008000, 0x7fff7fff, 0x7efe7efe, 0x800080, 0xff7fff7f, 0x0, 0xffffffff, 0x8000, 0xffff7fff, }, {0x80018001, 0x80008000, 0x7eff7eff, 0x810081, 0xff80ff80, 0x10001, 0x0, 0x18001, 0x8000, }, {0x80000000, 0x7fffffff, 0x7efefefe, 0x808080, 0xff7f7f7f, 0x8000, 0xffff7fff, 0x0, 0xffffffff, }, {0x80010001, 0x80000000, 0x7efffeff, 0x818081, 0xff807f80, 0x18001, 0x8000, 0x10001, 0x0, }, };
  const u32 __SSUB16_select_groundTruth[numValues][numValues] = {{0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22222222, 0x11111111, 0x22221111, 0x11112222, }, {0x11111111, 0x11111111, 0x22222222, 0x22222222, 0x11111111, 0x22222222, 0x11111111, 0x22221111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22222222, 0x11111111, 0x22221111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22221111, 0x11111111, }, {0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22222222, 0x11111111, 0x22222222, 0x11112222, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x11111111, 0x22222222, 0x11112222, }, {0x11111111, 0x11112222, 0x11112222, 0x11112222, 0x11111111, 0x11112222, 0x11111111, 0x11111111, 0x11112222, }, {0x22221111, 0x22221111, 0x22222222, 0x22222222, 0x22221111, 0x22222222, 0x11111111, 0x22221111, 0x11111111, }, };
  const u32 __QSUB16_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xfefefefe, 0x80808080, 0x7f7f7f7f, 0x80008000, 0x7fff7fff, 0x80000000, 0x7fffffff, }, {0x10001, 0x0, 0xfefffeff, 0x80818081, 0x7f807f80, 0x80018001, 0x7fff7fff, 0x80010001, 0x7fff0000, }, {0x1020102, 0x1010101, 0x0, 0x81828182, 0x7fff7fff, 0x81028102, 0x7fff7fff, 0x81020102, 0x7fff0101, }, {0x7f807f80, 0x7f7f7f7f, 0x7e7e7e7e, 0x0, 0x7fff7fff, 0xff80ff80, 0x7fff7fff, 0xff807f80, 0x7fff7f7f, }, {0x80818081, 0x80808080, 0x80008000, 0x80008000, 0x0, 0x80008000, 0x800080, 0x80008081, 0x808080, }, {0x7fff7fff, 0x7fff7fff, 0x7efe7efe, 0x800080, 0x7fff7fff, 0x0, 0x7fff7fff, 0x7fff, 0x7fff7fff, }, {0x80018001, 0x80008000, 0x80008000, 0x80008000, 0xff80ff80, 0x80008000, 0x0, 0x80008001, 0x8000, }, {0x7fff0000, 0x7fffffff, 0x7efefefe, 0x808080, 0x7fff7f7f, 0x8000, 0x7fff7fff, 0x0, 0x7fffffff, }, {0x80010001, 0x80000000, 0x8000feff, 0x80008081, 0xff807f80, 0x80008001, 0x7fff, 0x80000001, 0x0, }, };
  const u32 __QSUB16_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, };
  const u32 __USUB16_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xfefefefe, 0x80808080, 0x7f7f7f7f, 0x80008000, 0x7fff7fff, 0x80000000, 0x7fffffff, }, {0x10001, 0x0, 0xfefffeff, 0x80818081, 0x7f807f80, 0x80018001, 0x80008000, 0x80010001, 0x80000000, }, {0x1020102, 0x1010101, 0x0, 0x81828182, 0x80818081, 0x81028102, 0x81018101, 0x81020102, 0x81010101, }, {0x7f807f80, 0x7f7f7f7f, 0x7e7e7e7e, 0x0, 0xfefffeff, 0xff80ff80, 0xff7fff7f, 0xff807f80, 0xff7f7f7f, }, {0x80818081, 0x80808080, 0x7f7f7f7f, 0x1010101, 0x0, 0x810081, 0x800080, 0x818081, 0x808080, }, {0x80008000, 0x7fff7fff, 0x7efe7efe, 0x800080, 0xff7fff7f, 0x0, 0xffffffff, 0x8000, 0xffff7fff, }, {0x80018001, 0x80008000, 0x7eff7eff, 0x810081, 0xff80ff80, 0x10001, 0x0, 0x18001, 0x8000, }, {0x80000000, 0x7fffffff, 0x7efefefe, 0x808080, 0xff7f7f7f, 0x8000, 0xffff7fff, 0x0, 0xffffffff, }, {0x80010001, 0x80000000, 0x7efffeff, 0x818081, 0xff807f80, 0x18001, 0x8000, 0x10001, 0x0, }, };
  const u32 __USUB16_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x22222222, 0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22221111, }, {0x22222222, 0x11111111, 0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22221111, }, {0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22221111, }, {0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11112222, 0x11111111, }, {0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x22222222, 0x11112222, 0x22221111, }, {0x22222222, 0x11111111, 0x11111111, 0x11111111, 0x22222222, 0x11111111, 0x11111111, 0x11112222, 0x11111111, }, {0x22221111, 0x11111111, 0x11111111, 0x11111111, 0x22221111, 0x11111111, 0x22221111, 0x11111111, 0x22221111, }, {0x22222222, 0x11111111, 0x11112222, 0x11112222, 0x22222222, 0x11112222, 0x11112222, 0x11112222, 0x11111111, }, };
  const u32 __UQSUB16_groundTruth[numValues][numValues] = {{0x0, 0xffffffff, 0xfefefefe, 0x80808080, 0x7f7f7f7f, 0x80008000, 0x7fff7fff, 0x80000000, 0x7fffffff, }, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, }, {0x0, 0x1010101, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x101, }, {0x0, 0x7f7f7f7f, 0x7e7e7e7e, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7f7f, }, {0x0, 0x80808080, 0x7f7f7f7f, 0x1010101, 0x0, 0x810081, 0x800080, 0x810000, 0x808080, }, {0x0, 0x7fff7fff, 0x7efe7efe, 0x800080, 0x0, 0x0, 0x0, 0x0, 0x7fff, }, {0x0, 0x80008000, 0x7eff7eff, 0x810081, 0x0, 0x10001, 0x0, 0x10000, 0x8000, }, {0x0, 0x7fffffff, 0x7efefefe, 0x808080, 0x7f7f, 0x8000, 0x7fff, 0x0, 0xffff, }, {0x0, 0x80000000, 0x7eff0000, 0x810000, 0x0, 0x10000, 0x0, 0x10000, 0x0, }, };
  const u32 __UQSUB16_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, };
  const u32 __SMLAD_groundTruth[numValues][numValues] = {{0x7, 0x5, 0xfffffe03, 0xffff0107, 0xff05, 0xffff0007, 0x10005, 0xffff8007, 0x8005, }, {0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, }, {0xfffffe03, 0x5, 0x20407, 0xfffd03, 0xff000105, 0x100fe03, 0xfeff0005, 0x807e03, 0xff7f8005, }, {0xffff0107, 0x5, 0xfffd03, 0x7efe8207, 0x81007f05, 0x7f7e0107, 0x80810005, 0x3fbe8107, 0xc0408005, }, {0xff05, 0x5, 0xff000105, 0x81007f05, 0x7f008005, 0x8080ff05, 0x7f800005, 0xc040ff05, 0x3fc00005, }, {0xffff0007, 0x5, 0x100fe03, 0x7f7e0107, 0x8080ff05, 0x7ffe0007, 0x80010005, 0x3ffe8007, 0xc0008005, }, {0x10005, 0x5, 0xfeff0005, 0x80810005, 0x7f800005, 0x80010005, 0x80000005, 0xc0010005, 0x40000005, }, {0xffff8007, 0x5, 0x807e03, 0x3fbe8107, 0xc040ff05, 0x3ffe8007, 0xc0010005, 0x3fff0007, 0xc0008005, }, {0x8005, 0x5, 0xff7f8005, 0xc0408005, 0x3fc00005, 0xc0008005, 0x40000005, 0xc0008005, 0x40000005, }, };
  const u32 __SMLAD_select_groundTruth[numValues][numValues] = {{0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, }, };

  TEST_INTRINSICS_V(__REV, numValues);
  TEST_INTRINSICS_V(__REV16, numValues);
  TEST_INTRINSICS_V(__REVSH, numValues);
  TEST_INTRINSICS_V(__RBIT, numValues);
  TEST_INTRINSICS_VF(__SSAT, numValues, 5);
  TEST_INTRINSICS_VF(__USAT, numValues, 5);
  TEST_INTRINSICS_V(__CLZ, numValues);
  TEST_INTRINSICS_VV(__SADD8, numValues);
  TEST_INTRINSICS_VV(__QADD8, numValues);
  TEST_INTRINSICS_VV(__SHADD8, numValues);
  TEST_INTRINSICS_VV(__UADD8, numValues);
  TEST_INTRINSICS_VV(__UQADD8, numValues);
  TEST_INTRINSICS_VV(__UHADD8, numValues);
  TEST_INTRINSICS_VV(__SSUB8, numValues);
  TEST_INTRINSICS_VV(__QSUB8, numValues);
  TEST_INTRINSICS_VV(__SHSUB8, numValues);
  TEST_INTRINSICS_VV(__USUB8, numValues);
  TEST_INTRINSICS_VV(__UQSUB8, numValues);
  TEST_INTRINSICS_VV(__UHSUB8, numValues);
  TEST_INTRINSICS_VV(__SADD16, numValues);
  TEST_INTRINSICS_VV(__QADD16, numValues);
  TEST_INTRINSICS_VV(__SHADD16, numValues);
  TEST_INTRINSICS_VV(__UADD16, numValues);
  TEST_INTRINSICS_VV(__UQADD16, numValues);
  TEST_INTRINSICS_VV(__UHADD16, numValues);
  TEST_INTRINSICS_VV(__SSUB16, numValues);
  TEST_INTRINSICS_VV(__QSUB16, numValues);
  TEST_INTRINSICS_VV(__USUB16, numValues);
  TEST_INTRINSICS_VV(__UQSUB16, numValues);
  //TEST_INTRINSICS_VV(__USAD8, numValues); // TODO: verify implementation before using
  TEST_INTRINSICS_VVF(__SMLAD, numValues, 5);

  //
  // A few extras
  //

  {
    const u8 count = __CLZ(0);
    ASSERT_TRUE(count == 32);

    for(s32 i=0; i<32; i++) {
      const u32 number = 1 << i;
      const u8 count = __CLZ(number);
      ASSERT_TRUE(count == (31-i));
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, HostIntrinsics_m4)

GTEST_TEST(CoreTech_Common, VectorTypes)
{
  {
    u8x2 a_u8x2;
    a_u8x2.v0 = 0x1; a_u8x2.v1 = 0x2;
    ASSERT_TRUE(u16(a_u8x2) == 0x0201);
  }

  {
    s8x2 a_s8x2;
    a_s8x2.v0 = -0x1; a_s8x2.v1 = -0x2;
    ASSERT_TRUE(u16(a_s8x2) == 0xfeff);
  }

  {
    u8x4 a_u8x4;
    a_u8x4.v0 = 0x3; a_u8x4.v1 = 0x4; a_u8x4.v2 = 0x5; a_u8x4.v3 = 0x6;
    ASSERT_TRUE(u32(a_u8x4) == 0x06050403);
  }

  {
    s8x4 a_s8x4;
    a_s8x4.v0 = -0x3; a_s8x4.v1 = -0x4; a_s8x4.v2 = -0x5; a_s8x4.v3 = -0x6;
    ASSERT_TRUE(u32(a_s8x4) == 0xfafbfcfd);
  }

  {
    u8x8 a_u8x8;
    a_u8x8.v0 = 0x7; a_u8x8.v1 = 0x8; a_u8x8.v2 = 0x9; a_u8x8.v3 = 0xa; a_u8x8.v4 = 0xb; a_u8x8.v5 = 0xc; a_u8x8.v6 = 0xd; a_u8x8.v7 = 0xe;
    ASSERT_TRUE(u64(a_u8x8) == 0x0e0d0c0b0a090807);
  }

  {
    s8x8 a_s8x8;
    a_s8x8.v0 = -0x7; a_s8x8.v1 = -0x8; a_s8x8.v2 = -0x9; a_s8x8.v3 = -0xa; a_s8x8.v4 = -0xb; a_s8x8.v5 = -0xc; a_s8x8.v6 = -0xd; a_s8x8.v7 = -0xe;
    ASSERT_TRUE(u64(a_s8x8) == 0xf2f3f4f5f6f7f8f9);
  }

  {
    u8x16 a_u8x16;
    a_u8x16.v0 = 0xf; a_u8x16.v1 = 0x10; a_u8x16.v2 = 0x11; a_u8x16.v3 = 0x12; a_u8x16.v4 = 0x13; a_u8x16.v5 = 0x14; a_u8x16.v6 = 0x15; a_u8x16.v7 = 0x16; a_u8x16.v8 = 0x17; a_u8x16.v9 = 0x18; a_u8x16.va = 0x19; a_u8x16.vb = 0x1a; a_u8x16.vc = 0x1b; a_u8x16.vd = 0x1c; a_u8x16.ve = 0x1d; a_u8x16.vf = 0x1e;
    ASSERT_TRUE(a_u8x16.bits[0] == 0x161514131211100f);
    ASSERT_TRUE(a_u8x16.bits[1] == 0x1e1d1c1b1a191817);
  }

  {
    s8x16 a_s8x16;
    a_s8x16.v0 = -0xf; a_s8x16.v1 = -0x10; a_s8x16.v2 = -0x11; a_s8x16.v3 = -0x12; a_s8x16.v4 = -0x13; a_s8x16.v5 = -0x14; a_s8x16.v6 = -0x15; a_s8x16.v7 = -0x16; a_s8x16.v8 = -0x17; a_s8x16.v9 = -0x18; a_s8x16.va = -0x19; a_s8x16.vb = -0x1a; a_s8x16.vc = -0x1b; a_s8x16.vd = -0x1c; a_s8x16.ve = -0x1d; a_s8x16.vf = -0x1e;
    ASSERT_TRUE(a_s8x16.bits[0] == 0xeaebecedeeeff0f1);
    ASSERT_TRUE(a_s8x16.bits[1] == 0xe2e3e4e5e6e7e8e9);
  }

  {
    u16x2 a_u16x2;
    a_u16x2.v0 = 0x1f; a_u16x2.v1 = 0x20;
    ASSERT_TRUE(u32(a_u16x2) == 0x0020001f);
  }

  {
    s16x2 a_s16x2;
    a_s16x2.v0 = -0x1f; a_s16x2.v1 = -0x20;
    ASSERT_TRUE(u32(a_s16x2) == 0xffe0ffe1);
  }

  {
    u16x4 a_u16x4;
    a_u16x4.v0 = 0x21; a_u16x4.v1 = 0x22; a_u16x4.v2 = 0x23; a_u16x4.v3 = 0x24;
    ASSERT_TRUE(u64(a_u16x4) == 0x0024002300220021);
  }

  {
    s16x4 a_s16x4;
    a_s16x4.v0 = -0x21; a_s16x4.v1 = -0x22; a_s16x4.v2 = -0x23; a_s16x4.v3 = -0x24;
    ASSERT_TRUE(u64(a_s16x4) == 0xffdcffddffdeffdf);
  }

  {
    u16x8 a_u16x8;
    a_u16x8.v0 = 0x25; a_u16x8.v1 = 0x26; a_u16x8.v2 = 0x27; a_u16x8.v3 = 0x28; a_u16x8.v4 = 0x29; a_u16x8.v5 = 0x2a; a_u16x8.v6 = 0x2b; a_u16x8.v7 = 0x2c;
    ASSERT_TRUE(a_u16x8.bits[0] == 0x0028002700260025);
    ASSERT_TRUE(a_u16x8.bits[1] == 0x002c002b002a0029);
  }

  {
    s16x8 a_s16x8;
    a_s16x8.v0 = -0x25; a_s16x8.v1 = -0x26; a_s16x8.v2 = -0x27; a_s16x8.v3 = -0x28; a_s16x8.v4 = -0x29; a_s16x8.v5 = -0x2a; a_s16x8.v6 = -0x2b; a_s16x8.v7 = -0x2c;
    ASSERT_TRUE(a_s16x8.bits[0] == 0xffd8ffd9ffdaffdb);
    ASSERT_TRUE(a_s16x8.bits[1] == 0xffd4ffd5ffd6ffd7);
  }

  {
    u32x2 a_u32x2;
    a_u32x2.v0 = 0x2d; a_u32x2.v1 = 0x2e;
    ASSERT_TRUE(u64(a_u32x2) == 0x0000002e0000002d);
  }

  {
    s32x2 a_s32x2;
    a_s32x2.v0 = -0x2d; a_s32x2.v1 = -0x2e;
    ASSERT_TRUE(u64(a_s32x2) == 0xffffffd2ffffffd3);
  }

  {
    u32x4 a_u32x4;
    a_u32x4.v0 = 0x2f; a_u32x4.v1 = 0x30; a_u32x4.v2 = 0x31; a_u32x4.v3 = 0x32;
    ASSERT_TRUE(a_u32x4.bits[0] == 0x000000300000002f);
    ASSERT_TRUE(a_u32x4.bits[1] == 0x0000003200000031);
  }

  {
    s32x4 a_s32x4;
    a_s32x4.v0 = -0x2f; a_s32x4.v1 = -0x30; a_s32x4.v2 = -0x31; a_s32x4.v3 = -0x32;
    ASSERT_TRUE(a_s32x4.bits[0] == 0xffffffd0ffffffd1);
    ASSERT_TRUE(a_s32x4.bits[1] == 0xffffffceffffffcf);
  }

  {
    u64x2 a_u64x2;
    a_u64x2.v0 = 0x33; a_u64x2.v1 = 0x34;
    ASSERT_TRUE(a_u64x2.bits[0] == 0x0000000000000033);
    ASSERT_TRUE(a_u64x2.bits[1] == 0x0000000000000034);
  }

  {
    s64x2 a_s64x2;
    a_s64x2.v0 = -0x33; a_s64x2.v1 = -0x34;
    ASSERT_TRUE(a_s64x2.bits[0] == 0xffffffffffffffcd);
    ASSERT_TRUE(a_s64x2.bits[1] == 0xffffffffffffffcc);
  }

  {
    f32x2 a_f32x2;
    a_f32x2.v0 = 0x35; a_f32x2.v1 = 0x36;
    ASSERT_TRUE(u64(a_f32x2) == 0x4258000042540000);
  }

  {
    f32x4 a_f32x4;
    a_f32x4.v0 = 0x37; a_f32x4.v1 = 0x38; a_f32x4.v2 = 0x39; a_f32x4.v3 = 0x3a;
    ASSERT_TRUE(a_f32x4.bits[0] == 0x42600000425c0000);
    ASSERT_TRUE(a_f32x4.bits[1] == 0x4268000042640000);
  }

  {
    f64x2 a_f64x2;
    a_f64x2.v0 = 0x3b; a_f64x2.v1 = 0x3c;
    ASSERT_TRUE(a_f64x2.bits[0] == 0x404d800000000000);
    ASSERT_TRUE(a_f64x2.bits[1] == 0x404e000000000000);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, VectorTypes)

GTEST_TEST(CoreTech_Common, RoundUpAndDown)
{
  ASSERT_TRUE(RoundUp<u32>(0,1) == 0);
  ASSERT_TRUE(RoundUp<u32>(1,1) == 1);
  ASSERT_TRUE(RoundUp<u32>(2,1) == 2);
  ASSERT_TRUE(RoundUp<u32>(3,1) == 3);

  ASSERT_TRUE(RoundUp<u32>(0,2) == 0);
  ASSERT_TRUE(RoundUp<u32>(1,2) == 2);
  ASSERT_TRUE(RoundUp<u32>(2,2) == 2);
  ASSERT_TRUE(RoundUp<u32>(3,2) == 4);

  ASSERT_TRUE(RoundUp<u32>(0 ,16) == 0);
  ASSERT_TRUE(RoundUp<u32>(1 ,16) == 16);
  ASSERT_TRUE(RoundUp<u32>(2 ,16) == 16);
  ASSERT_TRUE(RoundUp<u32>(3 ,16) == 16);
  ASSERT_TRUE(RoundUp<u32>(14,16) == 16);
  ASSERT_TRUE(RoundUp<u32>(15,16) == 16);
  ASSERT_TRUE(RoundUp<u32>(16,16) == 16);
  ASSERT_TRUE(RoundUp<u32>(17,16) == 32);

  ASSERT_TRUE(RoundUp<s32>(-3,1) == -3);
  ASSERT_TRUE(RoundUp<s32>(-2,1) == -2);
  ASSERT_TRUE(RoundUp<s32>(-1,1) == -1);
  ASSERT_TRUE(RoundUp<s32>(0 ,1) == 0);
  ASSERT_TRUE(RoundUp<s32>(1 ,1) == 1);
  ASSERT_TRUE(RoundUp<s32>(2 ,1) == 2);
  ASSERT_TRUE(RoundUp<s32>(3 ,1) == 3);

  ASSERT_TRUE(RoundUp<s32>(-3,2) == -2);
  ASSERT_TRUE(RoundUp<s32>(-2,2) == -2);
  ASSERT_TRUE(RoundUp<s32>(-1,2) == 0);
  ASSERT_TRUE(RoundUp<s32>(0 ,2) == 0);
  ASSERT_TRUE(RoundUp<s32>(1 ,2) == 2);
  ASSERT_TRUE(RoundUp<s32>(2 ,2) == 2);
  ASSERT_TRUE(RoundUp<s32>(3 ,2) == 4);

  ASSERT_TRUE(RoundUp<s32>(-17,16) == -16);
  ASSERT_TRUE(RoundUp<s32>(-16,16) == -16);
  ASSERT_TRUE(RoundUp<s32>(-15,16) == 0);
  ASSERT_TRUE(RoundUp<s32>(-14,16) == 0);
  ASSERT_TRUE(RoundUp<s32>(-3 ,16) == 0);
  ASSERT_TRUE(RoundUp<s32>(-2 ,16) == 0);
  ASSERT_TRUE(RoundUp<s32>(-1 ,16) == 0);
  ASSERT_TRUE(RoundUp<s32>(0  ,16) == 0);
  ASSERT_TRUE(RoundUp<s32>(1  ,16) == 16);
  ASSERT_TRUE(RoundUp<s32>(2  ,16) == 16);
  ASSERT_TRUE(RoundUp<s32>(3  ,16) == 16);
  ASSERT_TRUE(RoundUp<s32>(14 ,16) == 16);
  ASSERT_TRUE(RoundUp<s32>(15 ,16) == 16);
  ASSERT_TRUE(RoundUp<s32>(16 ,16) == 16);
  ASSERT_TRUE(RoundUp<s32>(17 ,16) == 32);

  ASSERT_TRUE(RoundDown<u32>(0,1) == 0);
  ASSERT_TRUE(RoundDown<u32>(1,1) == 1);
  ASSERT_TRUE(RoundDown<u32>(2,1) == 2);
  ASSERT_TRUE(RoundDown<u32>(3,1) == 3);

  ASSERT_TRUE(RoundDown<u32>(0,2) == 0);
  ASSERT_TRUE(RoundDown<u32>(1,2) == 0);
  ASSERT_TRUE(RoundDown<u32>(2,2) == 2);
  ASSERT_TRUE(RoundDown<u32>(3,2) == 2);

  ASSERT_TRUE(RoundDown<u32>(0 ,16) == 0);
  ASSERT_TRUE(RoundDown<u32>(1 ,16) == 0);
  ASSERT_TRUE(RoundDown<u32>(2 ,16) == 0);
  ASSERT_TRUE(RoundDown<u32>(3 ,16) == 0);
  ASSERT_TRUE(RoundDown<u32>(14,16) == 0);
  ASSERT_TRUE(RoundDown<u32>(15,16) == 0);
  ASSERT_TRUE(RoundDown<u32>(16,16) == 16);
  ASSERT_TRUE(RoundDown<u32>(17,16) == 16);

  ASSERT_TRUE(RoundDown<s32>(-3,1) == -3);
  ASSERT_TRUE(RoundDown<s32>(-2,1) == -2);
  ASSERT_TRUE(RoundDown<s32>(-1,1) == -1);
  ASSERT_TRUE(RoundDown<s32>(0 ,1) == 0);
  ASSERT_TRUE(RoundDown<s32>(1 ,1) == 1);
  ASSERT_TRUE(RoundDown<s32>(2 ,1) == 2);
  ASSERT_TRUE(RoundDown<s32>(3 ,1) == 3);

  ASSERT_TRUE(RoundDown<s32>(-3,2) == -4);
  ASSERT_TRUE(RoundDown<s32>(-2,2) == -2);
  ASSERT_TRUE(RoundDown<s32>(-1,2) == -2);
  ASSERT_TRUE(RoundDown<s32>(0 ,2) == 0);
  ASSERT_TRUE(RoundDown<s32>(1 ,2) == 0);
  ASSERT_TRUE(RoundDown<s32>(2 ,2) == 2);
  ASSERT_TRUE(RoundDown<s32>(3 ,2) == 2);

  ASSERT_TRUE(RoundDown<s32>(-17,16) == -32);
  ASSERT_TRUE(RoundDown<s32>(-16,16) == -16);
  ASSERT_TRUE(RoundDown<s32>(-15,16) == -16);
  ASSERT_TRUE(RoundDown<s32>(-14,16) == -16);
  ASSERT_TRUE(RoundDown<s32>(-3 ,16) == -16);
  ASSERT_TRUE(RoundDown<s32>(-2 ,16) == -16);
  ASSERT_TRUE(RoundDown<s32>(-1 ,16) == -16);
  ASSERT_TRUE(RoundDown<s32>(0  ,16) == 0);
  ASSERT_TRUE(RoundDown<s32>(1  ,16) == 0);
  ASSERT_TRUE(RoundDown<s32>(2  ,16) == 0);
  ASSERT_TRUE(RoundDown<s32>(3  ,16) == 0);
  ASSERT_TRUE(RoundDown<s32>(14 ,16) == 0);
  ASSERT_TRUE(RoundDown<s32>(15 ,16) == 0);
  ASSERT_TRUE(RoundDown<s32>(16 ,16) == 16);
  ASSERT_TRUE(RoundDown<s32>(17 ,16) == 16);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, RoundUpAndDown)

GTEST_TEST(CoreTech_Common, RoundAndSaturate)
{
  //
  // Normal Round<>
  //

  // f32
  /**/                                  ASSERT_TRUE(Round<u32>(-0.4f) == 0); ASSERT_TRUE(Round<u32>(0.0f) == 0); ASSERT_TRUE(Round<u32>(1.1f) == 1); ASSERT_TRUE(Round<u32>(1.6f) == 2);
  /**/                                  ASSERT_TRUE(Round<u64>(-0.4f) == 0); ASSERT_TRUE(Round<u64>(0.0f) == 0); ASSERT_TRUE(Round<u64>(1.1f) == 1); ASSERT_TRUE(Round<u64>(1.6f) == 2);

  ASSERT_TRUE(Round<s32>(-1.6f) == -2); ASSERT_TRUE(Round<s32>(-0.4f) == 0); ASSERT_TRUE(Round<s32>(0.0f) == 0); ASSERT_TRUE(Round<s32>(1.1f) == 1); ASSERT_TRUE(Round<s32>(1.6f) == 2);
  ASSERT_TRUE(Round<s64>(-1.6f) == -2); ASSERT_TRUE(Round<s64>(-0.4f) == 0); ASSERT_TRUE(Round<s64>(0.0f) == 0); ASSERT_TRUE(Round<s64>(1.1f) == 1); ASSERT_TRUE(Round<s64>(1.6f) == 2);

  ASSERT_TRUE(Round<f32>(-1.6f) == -2); ASSERT_TRUE(Round<f32>(-0.4f) == 0); ASSERT_TRUE(Round<f32>(0.0f) == 0); ASSERT_TRUE(Round<f32>(1.1f) == 1); ASSERT_TRUE(Round<f32>(1.6f) == 2);
  ASSERT_TRUE(Round<f64>(-1.6f) == -2); ASSERT_TRUE(Round<f64>(-0.4f) == 0); ASSERT_TRUE(Round<f64>(0.0f) == 0); ASSERT_TRUE(Round<f64>(1.1f) == 1); ASSERT_TRUE(Round<f64>(1.6f) == 2);

  // f64
  /**/                                  ASSERT_TRUE(Round<u32>(-0.4)  == 0); ASSERT_TRUE(Round<u32>(0.0)  == 0); ASSERT_TRUE(Round<u32>(1.1)  == 1); ASSERT_TRUE(Round<u32>(1.6)  == 2);
  /**/                                  ASSERT_TRUE(Round<u64>(-0.4)  == 0); ASSERT_TRUE(Round<u64>(0.0)  == 0); ASSERT_TRUE(Round<u64>(1.1)  == 1); ASSERT_TRUE(Round<u64>(1.6)  == 2);

  ASSERT_TRUE(Round<s32>(-1.6)  == -2); ASSERT_TRUE(Round<s32>(-0.4)  == 0); ASSERT_TRUE(Round<s32>(0.0)  == 0); ASSERT_TRUE(Round<s32>(1.1)  == 1); ASSERT_TRUE(Round<s32>(1.6)  == 2);
  ASSERT_TRUE(Round<s64>(-1.6)  == -2); ASSERT_TRUE(Round<s64>(-0.4)  == 0); ASSERT_TRUE(Round<s64>(0.0)  == 0); ASSERT_TRUE(Round<s64>(1.1)  == 1); ASSERT_TRUE(Round<s64>(1.6)  == 2);

  ASSERT_TRUE(Round<f32>(-1.6)  == -2); ASSERT_TRUE(Round<f32>(-0.4)  == 0); ASSERT_TRUE(Round<f32>(0.0)  == 0); ASSERT_TRUE(Round<f32>(1.1)  == 1); ASSERT_TRUE(Round<f32>(1.6)  == 2);
  ASSERT_TRUE(Round<f64>(-1.6)  == -2); ASSERT_TRUE(Round<f64>(-0.4)  == 0); ASSERT_TRUE(Round<f64>(0.0)  == 0); ASSERT_TRUE(Round<f64>(1.1)  == 1); ASSERT_TRUE(Round<f64>(1.6)  == 2);

  //
  // RoundIfInteger<>
  //

  // f32
  /**/                                           ASSERT_TRUE(RoundIfInteger<u8> (-0.4f) == 0); ASSERT_TRUE(RoundIfInteger<u8> (0.0f) == 0); ASSERT_TRUE(RoundIfInteger<u8> (1.1f) == 1); ASSERT_TRUE(RoundIfInteger<u8> (1.6f) == 2);
  /**/                                           ASSERT_TRUE(RoundIfInteger<u16>(-0.4f) == 0); ASSERT_TRUE(RoundIfInteger<u16>(0.0f) == 0); ASSERT_TRUE(RoundIfInteger<u16>(1.1f) == 1); ASSERT_TRUE(RoundIfInteger<u16>(1.6f) == 2);
  /**/                                           ASSERT_TRUE(RoundIfInteger<u32>(-0.4f) == 0); ASSERT_TRUE(RoundIfInteger<u32>(0.0f) == 0); ASSERT_TRUE(RoundIfInteger<u32>(1.1f) == 1); ASSERT_TRUE(RoundIfInteger<u32>(1.6f) == 2);
  /**/                                           ASSERT_TRUE(RoundIfInteger<u64>(-0.4f) == 0); ASSERT_TRUE(RoundIfInteger<u64>(0.0f) == 0); ASSERT_TRUE(RoundIfInteger<u64>(1.1f) == 1); ASSERT_TRUE(RoundIfInteger<u64>(1.6f) == 2);

  ASSERT_TRUE(RoundIfInteger<s8> (-1.6f) == -2); ASSERT_TRUE(RoundIfInteger<s8> (-0.4f) == 0); ASSERT_TRUE(RoundIfInteger<s8> (0.0f) == 0); ASSERT_TRUE(RoundIfInteger<s8> (1.1f) == 1); ASSERT_TRUE(RoundIfInteger<s8> (1.6f) == 2);
  ASSERT_TRUE(RoundIfInteger<s16>(-1.6f) == -2); ASSERT_TRUE(RoundIfInteger<s16>(-0.4f) == 0); ASSERT_TRUE(RoundIfInteger<s16>(0.0f) == 0); ASSERT_TRUE(RoundIfInteger<s16>(1.1f) == 1); ASSERT_TRUE(RoundIfInteger<s16>(1.6f) == 2);
  ASSERT_TRUE(RoundIfInteger<s32>(-1.6f) == -2); ASSERT_TRUE(RoundIfInteger<s32>(-0.4f) == 0); ASSERT_TRUE(RoundIfInteger<s32>(0.0f) == 0); ASSERT_TRUE(RoundIfInteger<s32>(1.1f) == 1); ASSERT_TRUE(RoundIfInteger<s32>(1.6f) == 2);
  ASSERT_TRUE(RoundIfInteger<s64>(-1.6f) == -2); ASSERT_TRUE(RoundIfInteger<s64>(-0.4f) == 0); ASSERT_TRUE(RoundIfInteger<s64>(0.0f) == 0); ASSERT_TRUE(RoundIfInteger<s64>(1.1f) == 1); ASSERT_TRUE(RoundIfInteger<s64>(1.6f) == 2);

  ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(-1.6f), -1.6f)); ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(-0.4f), -0.4f)); ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(0.0f), 0)); ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(1.1f), 1.1f)); ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(1.6f), 1.6f));
  ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(-1.6f), -1.6));  ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(-0.4f), -0.4));  ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(0.0f), 0)); ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(1.1f), 1.1));  ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(1.6f), 1.6));

  // f64
  /**/                                           ASSERT_TRUE(RoundIfInteger<u8> (-0.4)  == 0); ASSERT_TRUE(RoundIfInteger<u8> (0.0)  == 0); ASSERT_TRUE(RoundIfInteger<u8> (1.1) == 1); ASSERT_TRUE(RoundIfInteger<u8> (1.6) == 2);
  /**/                                           ASSERT_TRUE(RoundIfInteger<u16>(-0.4)  == 0); ASSERT_TRUE(RoundIfInteger<u16>(0.0)  == 0); ASSERT_TRUE(RoundIfInteger<u16>(1.1) == 1); ASSERT_TRUE(RoundIfInteger<u16>(1.6) == 2);
  /**/                                           ASSERT_TRUE(RoundIfInteger<u32>(-0.4)  == 0); ASSERT_TRUE(RoundIfInteger<u32>(0.0)  == 0); ASSERT_TRUE(RoundIfInteger<u32>(1.1) == 1); ASSERT_TRUE(RoundIfInteger<u32>(1.6) == 2);
  /**/                                           ASSERT_TRUE(RoundIfInteger<u64>(-0.4)  == 0); ASSERT_TRUE(RoundIfInteger<u64>(0.0)  == 0); ASSERT_TRUE(RoundIfInteger<u64>(1.1) == 1); ASSERT_TRUE(RoundIfInteger<u64>(1.6) == 2);

  ASSERT_TRUE(RoundIfInteger<s8> (-1.6)  == -2); ASSERT_TRUE(RoundIfInteger<s8> (-0.4)  == 0); ASSERT_TRUE(RoundIfInteger<s8> (0.0)  == 0); ASSERT_TRUE(RoundIfInteger<s8> (1.1) == 1); ASSERT_TRUE(RoundIfInteger<s8> (1.6) == 2);
  ASSERT_TRUE(RoundIfInteger<s16>(-1.6)  == -2); ASSERT_TRUE(RoundIfInteger<s16>(-0.4)  == 0); ASSERT_TRUE(RoundIfInteger<s16>(0.0)  == 0); ASSERT_TRUE(RoundIfInteger<s16>(1.1) == 1); ASSERT_TRUE(RoundIfInteger<s16>(1.6) == 2);
  ASSERT_TRUE(RoundIfInteger<s32>(-1.6)  == -2); ASSERT_TRUE(RoundIfInteger<s32>(-0.4)  == 0); ASSERT_TRUE(RoundIfInteger<s32>(0.0)  == 0); ASSERT_TRUE(RoundIfInteger<s32>(1.1) == 1); ASSERT_TRUE(RoundIfInteger<s32>(1.6) == 2);
  ASSERT_TRUE(RoundIfInteger<s64>(-1.6)  == -2); ASSERT_TRUE(RoundIfInteger<s64>(-0.4)  == 0); ASSERT_TRUE(RoundIfInteger<s64>(0.0)  == 0); ASSERT_TRUE(RoundIfInteger<s64>(1.1) == 1); ASSERT_TRUE(RoundIfInteger<s64>(1.6) == 2);

  ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(-1.6), -1.6f)); ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(-0.4), -0.4f)); ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(0.0), 0)); ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(1.1), 1.1f)); ASSERT_TRUE(FLT_NEAR(RoundIfInteger<f32>(1.6), 1.6f));
  ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(-1.6), -1.6));  ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(-0.4), -0.4));  ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(0.0), 0)); ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(1.1), 1.1));  ASSERT_TRUE(DBL_NEAR(RoundIfInteger<f64>(1.6), 1.6));

  //
  // saturate_cast<>
  //

  /**/                                                                           ASSERT_TRUE(saturate_cast<u8> (static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<u8> (0xFF))                  == 0xFF);
  ASSERT_TRUE(saturate_cast<u8> (static_cast<s8> (-0x80))                 == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<s8> (0x7F))                  == 0x7F);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u8> (static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<u16>(0xFFFF))                == 0xFF);
  ASSERT_TRUE(saturate_cast<u8> (static_cast<s16>(-0x8000))               == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<s16>(0x7FFF))                == 0xFF);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u8> (static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<u32>(0xFFFFFFFF))            == 0xFF);
  ASSERT_TRUE(saturate_cast<u8> (static_cast<s32>(-0x80000000LL))         == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<s32>(0x7FFFFFFF))            == 0xFF);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u8> (static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) == 0xFF);
  ASSERT_TRUE(saturate_cast<u8> (static_cast<s64>(-0x8000000000000000LL)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<s64>(0x7FFFFFFFFFFFFFFFLL))  == 0xFF);
  ASSERT_TRUE(saturate_cast<u8> (static_cast<f32>(-10e30))                == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<f32>(10e30))                 == 0xFF);
  ASSERT_TRUE(saturate_cast<u8> (static_cast<f64>(-10e50))                == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<u8> (static_cast<f64>(10e50))                 == 0xFF);

  /**/                                                                               ASSERT_TRUE(saturate_cast<s8> (static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<u8> (0xFF))                  == 0x7F);
  ASSERT_TRUE(saturate_cast<s8> (static_cast<s8> (-0x80))                 == -0x80); ASSERT_TRUE(saturate_cast<s8> (static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<s8> (0x7F))                  == 0x7F);
  /**/                                                                               ASSERT_TRUE(saturate_cast<s8> (static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<u16>(0xFFFF))                == 0x7F);
  ASSERT_TRUE(saturate_cast<s8> (static_cast<s16>(-0x8000))               == -0x80); ASSERT_TRUE(saturate_cast<s8> (static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<s16>(0x7FFF))                == 0x7F);
  /**/                                                                               ASSERT_TRUE(saturate_cast<s8> (static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<u32>(0xFFFFFFFF))            == 0x7F);
  ASSERT_TRUE(saturate_cast<s8> (static_cast<s32>(-0x80000000LL))         == -0x80); ASSERT_TRUE(saturate_cast<s8> (static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<s32>(0x7FFFFFFF))            == 0x7F);
  /**/                                                                               ASSERT_TRUE(saturate_cast<s8> (static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) == 0x7F);
  ASSERT_TRUE(saturate_cast<s8> (static_cast<s64>(-0x8000000000000000LL)) == -0x80); ASSERT_TRUE(saturate_cast<s8> (static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<s64>(0x7FFFFFFFFFFFFFFFLL))  == 0x7F);
  ASSERT_TRUE(saturate_cast<s8> (static_cast<f32>(-10e30))                == -0x80); ASSERT_TRUE(saturate_cast<s8> (static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<f32>(10e30))                 == 0x7F);
  ASSERT_TRUE(saturate_cast<s8> (static_cast<f64>(-10e50))                == -0x80); ASSERT_TRUE(saturate_cast<s8> (static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<s8> (static_cast<f64>(10e50))                 == 0x7F);

  /**/                                                                           ASSERT_TRUE(saturate_cast<u16>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<u8> (0xFF))                  == 0xFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<s8> (-0x80))                 == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s8> (0x7F))                  == 0x7F);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u16>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<u16>(0xFFFF))                == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<s16>(-0x8000))               == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s16>(0x7FFF))                == 0x7FFF);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u16>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<u32>(0xFFFFFFFF))            == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<s32>(-0x80000000LL))         == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s32>(0x7FFFFFFF))            == 0xFFFF);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u16>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<s64>(-0x8000000000000000LL)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<s64>(0x7FFFFFFFFFFFFFFFLL))  == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<f32>(-10e30))                == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<f32>(10e30))                 == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u16>(static_cast<f64>(-10e50))                == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<u16>(static_cast<f64>(10e50))                 == 0xFFFF);

  /**/                                                                                 ASSERT_TRUE(saturate_cast<s16>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<u8> (0xFF))                  == 0xFF);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<s8> (-0x80))                 == -0x80);   ASSERT_TRUE(saturate_cast<s16>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<s8> (0x7F))                  == 0x7F);
  /**/                                                                                 ASSERT_TRUE(saturate_cast<s16>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<u16>(0xFFFF))                == 0x7FFF);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<s16>(-0x8000))               == -0x8000); ASSERT_TRUE(saturate_cast<s16>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<s16>(0x7FFF))                == 0x7FFF);
  /**/                                                                                 ASSERT_TRUE(saturate_cast<s16>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<u32>(0xFFFFFFFF))            == 0x7FFF);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<s32>(-0x80000000LL))         == -0x8000); ASSERT_TRUE(saturate_cast<s16>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<s32>(0x7FFFFFFF))            == 0x7FFF);
  /**/                                                                                 ASSERT_TRUE(saturate_cast<s16>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) == 0x7FFF);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<s64>(-0x8000000000000000LL)) == -0x8000); ASSERT_TRUE(saturate_cast<s16>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<s64>(0x7FFFFFFFFFFFFFFFLL))  == 0x7FFF);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<f32>(-10e30))                == -0x8000); ASSERT_TRUE(saturate_cast<s16>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<f32>(10e30))                 == 0x7FFF);
  ASSERT_TRUE(saturate_cast<s16>(static_cast<f64>(-10e50))                == -0x8000); ASSERT_TRUE(saturate_cast<s16>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<s16>(static_cast<f64>(10e50))                 == 0x7FFF);

  /**/                                                                           ASSERT_TRUE(saturate_cast<u32>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<u8> (0xFF))                  == 0xFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<s8> (-0x80))                 == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s8> (0x7F))                  == 0x7F);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u32>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<u16>(0xFFFF))                == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<s16>(-0x8000))               == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s16>(0x7FFF))                == 0x7FFF);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u32>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<u32>(0xFFFFFFFF))            == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<s32>(-0x80000000LL))         == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s32>(0x7FFFFFFF))            == 0x7FFFFFFF);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u32>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<s64>(-0x8000000000000000LL)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<s64>(0x7FFFFFFFFFFFFFFFLL))  == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<f32>(-10e30))                == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<f32>(10e30))                 == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<f64>(-10e50))                == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<u32>(static_cast<f64>(10e50))                 == 0xFFFFFFFF);

  /**/                                                                                       ASSERT_TRUE(saturate_cast<s32>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<u8> (0xFF))                  == 0xFF);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<s8> (-0x80))                 == -0x80);         ASSERT_TRUE(saturate_cast<s32>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<s8> (0x7F))                  == 0x7F);
  /**/                                                                                       ASSERT_TRUE(saturate_cast<s32>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<u16>(0xFFFF))                == 0xFFFF);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<s16>(-0x8000))               == -0x8000);       ASSERT_TRUE(saturate_cast<s32>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<s16>(0x7FFF))                == 0x7FFF);
  /**/                                                                                       ASSERT_TRUE(saturate_cast<s32>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<u32>(0xFFFFFFFF))            == 0x7FFFFFFF);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<s32>(-0x80000000LL))         == -0x80000000LL); ASSERT_TRUE(saturate_cast<s32>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<s32>(0x7FFFFFFF))            == 0x7FFFFFFF);
  /**/                                                                                       ASSERT_TRUE(saturate_cast<s32>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) == 0x7FFFFFFF);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<s64>(-0x8000000000000000LL)) == -0x80000000LL); ASSERT_TRUE(saturate_cast<s32>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<s64>(0x7FFFFFFFFFFFFFFFLL))  == 0x7FFFFFFF);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<f32>(-10e30))                == -0x80000000LL); ASSERT_TRUE(saturate_cast<s32>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<f32>(10e30))                 == 0x7FFFFFFF);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<f64>(-10e50))                == -0x80000000LL); ASSERT_TRUE(saturate_cast<s32>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<s32>(static_cast<f64>(10e50))                 == 0x7FFFFFFF);

  /**/                                                                           ASSERT_TRUE(saturate_cast<u64>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<u8> (0xFF))                  == 0xFF);
  ASSERT_TRUE(saturate_cast<u64>(static_cast<s8> (-0x80))                 == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<s8> (0x7F))                  == 0x7F);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u64>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<u16>(0xFFFF))                == 0xFFFF);
  ASSERT_TRUE(saturate_cast<u64>(static_cast<s16>(-0x8000))               == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<s16>(0x7FFF))                == 0x7FFF);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u64>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<u32>(0xFFFFFFFF))            == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<u64>(static_cast<s32>(-0x80000000LL))         == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<s32>(0x7FFFFFFF))            == 0x7FFFFFFF);
  /**/                                                                           ASSERT_TRUE(saturate_cast<u64>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) == 0xFFFFFFFFFFFFFFFFULL);
  ASSERT_TRUE(saturate_cast<u64>(static_cast<s64>(-0x8000000000000000LL)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<s64>(0x7FFFFFFFFFFFFFFFLL))  == 0x7FFFFFFFFFFFFFFFLL);
  ASSERT_TRUE(saturate_cast<u64>(static_cast<f32>(-10e30))                == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<f32>(10e30))                 == 0xFFFFFFFFFFFFFFFFULL);
  ASSERT_TRUE(saturate_cast<u64>(static_cast<f64>(-10e50))                == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<u64>(static_cast<f64>(10e50))                 == 0xFFFFFFFFFFFFFFFFULL);

  /**/                                                                                               ASSERT_TRUE(saturate_cast<s64>(static_cast<u8> (0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<u8> (0xFF))                  == 0xFF);
  ASSERT_TRUE(saturate_cast<s64>(static_cast<s8> (-0x80))                 == -0x80);                 ASSERT_TRUE(saturate_cast<s64>(static_cast<s8> (0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<s8> (0x7F))                  == 0x7F);
  /**/                                                                                               ASSERT_TRUE(saturate_cast<s64>(static_cast<u16>(0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<u16>(0xFFFF))                == 0xFFFF);
  ASSERT_TRUE(saturate_cast<s64>(static_cast<s16>(-0x8000))               == -0x8000);               ASSERT_TRUE(saturate_cast<s64>(static_cast<s16>(0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<s16>(0x7FFF))                == 0x7FFF);
  /**/                                                                                               ASSERT_TRUE(saturate_cast<s64>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<u32>(0xFFFFFFFF))            == 0xFFFFFFFF);
  ASSERT_TRUE(saturate_cast<s64>(static_cast<s32>(-0x80000000LL))         == -0x80000000LL);         ASSERT_TRUE(saturate_cast<s64>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<s32>(0x7FFFFFFF))            == 0x7FFFFFFF);
  /**/                                                                                               ASSERT_TRUE(saturate_cast<s64>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) == 0x7FFFFFFFFFFFFFFFLL);
  ASSERT_TRUE(saturate_cast<s64>(static_cast<s64>(-0x8000000000000000LL)) == -0x8000000000000000LL); ASSERT_TRUE(saturate_cast<s64>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<s64>(0x7FFFFFFFFFFFFFFFLL))  == 0x7FFFFFFFFFFFFFFFLL);
  ASSERT_TRUE(saturate_cast<s64>(static_cast<f32>(-10e30))                == -0x8000000000000000LL); ASSERT_TRUE(saturate_cast<s64>(static_cast<f32>(0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<f32>(10e30))                 == 0x7FFFFFFFFFFFFFFFLL);
  ASSERT_TRUE(saturate_cast<s64>(static_cast<f64>(-10e50))                == -0x8000000000000000LL); ASSERT_TRUE(saturate_cast<s64>(static_cast<f64>(0)) == 0); ASSERT_TRUE(saturate_cast<s64>(static_cast<f64>(10e50))                 == 0x7FFFFFFFFFFFFFFFLL);

  /**/                                                                                  ASSERT_TRUE(saturate_cast<f32>(static_cast<u32>(0)) == 0); ASSERT_TRUE(saturate_cast<f32>(static_cast<u32>(0xFFFFFFFF)) >= 4.0e9f);
  ASSERT_TRUE(saturate_cast<f32>(static_cast<s32>(-0x80000000LL))         <= -2.0e9f);  ASSERT_TRUE(saturate_cast<f32>(static_cast<s32>(0)) == 0); ASSERT_TRUE(saturate_cast<f32>(static_cast<s32>(0x7FFFFFFF)) >= 2.0e9f);
  /**/                                                                                  ASSERT_TRUE(saturate_cast<f32>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<f32>(static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) >= 1.0e19f);
  ASSERT_TRUE(saturate_cast<f32>(static_cast<s64>(-0x8000000000000000LL)) <= -9.0e18f); ASSERT_TRUE(saturate_cast<f32>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<f32>(static_cast<s64>(0x7FFFFFFFFFFFFFFFULL)) >= 9.0e18f);

  /**/                                                                                  ASSERT_TRUE(saturate_cast<f32>(static_cast<u64>(0)) == 0); ASSERT_TRUE(saturate_cast<f32>(static_cast<u64>(0xFFFFFFFFFFFFFFFFULL)) >= 1.0e19f);
  ASSERT_TRUE(saturate_cast<f32>(static_cast<s64>(-0x8000000000000000LL)) <= -9.0e18f); ASSERT_TRUE(saturate_cast<f32>(static_cast<s64>(0)) == 0); ASSERT_TRUE(saturate_cast<f32>(static_cast<s64>(0x7FFFFFFFFFFFFFFFULL)) >= 9.0e18f);

  ASSERT_TRUE(saturate_cast<f32>(static_cast<f64>(FLT_MAX))         == FLT_MAX);
  ASSERT_TRUE(saturate_cast<f32>(static_cast<f64>(3.402823467e+38)) == FLT_MAX);
  ASSERT_TRUE(saturate_cast<f32>(static_cast<f64>(3.402823466e+39)) == FLT_MAX);
  ASSERT_TRUE(saturate_cast<f32>(static_cast<f64>(DBL_MAX))         == FLT_MAX);

  ASSERT_TRUE(saturate_cast<f32>(-static_cast<f64>(FLT_MAX))         == -FLT_MAX);
  ASSERT_TRUE(saturate_cast<f32>(-static_cast<f64>(3.402823467e+38)) == -FLT_MAX);
  ASSERT_TRUE(saturate_cast<f32>(-static_cast<f64>(3.402823466e+39)) == -FLT_MAX);
  ASSERT_TRUE(saturate_cast<f32>(-static_cast<f64>(DBL_MAX))         == -FLT_MAX);

  //
  // More tests for floating point saturate_cast<>, for cases with insufficient precision
  //

  // u32
  ASSERT_TRUE(saturate_cast<u32>(static_cast<f32>(0xFFFFFF7E)) == 0xFFFFFF00);
  ASSERT_TRUE(saturate_cast<u32>(static_cast<f32>(0xFFFFFF80)) == 0xFFFFFFFF);

  // s32
  ASSERT_TRUE(saturate_cast<s32>(static_cast<f32>(0x7FFFFFBE)) == 0x7FFFFF80);
  ASSERT_TRUE(saturate_cast<s32>(static_cast<f32>(0x7FFFFFC0)) == 0x7FFFFFFF);

  // u64
  ASSERT_TRUE(saturate_cast<u64>(static_cast<f32>(0XFFFFFF7FFFFFFBFFULL)) == 0xFFFFFF0000000000ULL);
  ASSERT_TRUE(saturate_cast<u64>(static_cast<f32>(0XFFFFFF7FFFFFFC00ULL)) == 0xFFFFFF0000000000ULL);

  const u64 a = saturate_cast<u64>(static_cast<f32>(0XFFFFFF7FFFFFFFFFULL));
#if defined(__EDG__) || defined(__APPLE_CC__) || defined(__GNUC__)
  ASSERT_TRUE(a == 0xFFFFFF0000000000ULL);
#else
  ASSERT_TRUE(a == 0xFFFFFFFFFFFFFFFFULL);
#endif

  {
    u64 i = 0XFFFFFF7FFFFFFC01ULL;
    while(true) {
      ASSERT_TRUE(saturate_cast<u64>(static_cast<f32>(i)) >= 0xFFFFFF0000000000ULL);
      if(i == 0XFFFFFF7FFFFFFFFFULL) {
        break;
      }
      i++;
    }
  }

  {
    u64 i = 0XFFFFFF7FFFFFFFFFULL;
    while(true) {
      ASSERT_TRUE(saturate_cast<u64>(static_cast<f32>(i)) >= 0xFFFFFF0000000000ULL);
      if(i == 0XFFFFFFFFFFFFFFFFULL) {
        break;
      }
      i += 0X1000000000ULL;
    }
  }

  ASSERT_TRUE(saturate_cast<u64>(static_cast<f64>(0xFFFFFFFFFFFFFBFFULL)) == 0xFFFFFFFFFFFFF800ULL);

  const u64 b = saturate_cast<u64>(static_cast<f32>(0xFFFFFFFFFFFFFC00ULL));
  ASSERT_TRUE(b == 0xFFFFFFFFFFFFFFFFULL);

  ASSERT_TRUE(saturate_cast<u64>(static_cast<f64>(0xFFFFFFFFFFFFFFFFULL)) == 0xFFFFFFFFFFFFFFFFULL);

  {
    u64 i = 0xFFFFFFFFFFFFFC01ULL;
    while(true) {
      ASSERT_TRUE(saturate_cast<u64>(static_cast<f64>(i)) == 0xFFFFFFFFFFFFFFFFULL);
      if(i == 0XFFFFFFFFFFFFFFFFULL) {
        break;
      }
      i++;
    }
  }

  // s64
  ASSERT_TRUE(saturate_cast<s64>(static_cast<f32>(0x7FFFFFBFFFFFFDFFLL)) == 0x7FFFFF8000000000LL);
  ASSERT_TRUE(saturate_cast<s64>(static_cast<f32>(0x7FFFFFBFFFFFFE00LL)) == 0x7FFFFF8000000000LL);

  const s64 c = saturate_cast<s64>(static_cast<f32>(0x7FFFFFBFFFFFFE01LL));
#if defined(__EDG__) || defined(__APPLE_CC__) || defined(__GNUC__)
  ASSERT_TRUE(c == 0x7FFFFF8000000000LL);
#else
  ASSERT_TRUE(c == 0x7FFFFFFFFFFFFFFFLL);
#endif

  ASSERT_TRUE(saturate_cast<s64>(static_cast<f32>(0x7FFFFFFFFFFFFFFFLL)) == 0x7FFFFFFFFFFFFFFFLL);
  ASSERT_TRUE(saturate_cast<s64>(static_cast<f64>(0x7FFFFFFFFFFFFDFFLL)) == 0x7FFFFFFFFFFFFC00LL);

  const s64 d = saturate_cast<s64>(static_cast<f64>(0x7FFFFFFFFFFFFE00LL));
#if defined(__EDG__) || defined(__APPLE_CC__) || defined(__GNUC__)
  ASSERT_TRUE(d == 0x7FFFFFFFFFFFFFFFLL);
#else
  ASSERT_TRUE(d == 0x7FFFFFFFFFFFFC00LL);
#endif

  ASSERT_TRUE(saturate_cast<s64>(static_cast<f64>(0x7FFFFFFFFFFFFFFFLL)) == 0x7FFFFFFFFFFFFFFFLL);

  {
    s64 i = 0x7FFFFFFFFFFFFE01LL;
    while(true) {
      ASSERT_TRUE(saturate_cast<s64>(static_cast<f64>(i)) == 0x7FFFFFFFFFFFFFFFLL);
      if(i == 0X7FFFFFFFFFFFFFFFULL) {
        break;
      }
      i++;
    }
  }

  /*
  {
  const f64 v = 10e30;
  for(s64 i=0x7ffffffffffffdffLL; i<0x7fffffffffffffffLL; i+=0x1LL) {
  CoreTechPrint("0x%llx = 0x%llx\n", i, Round<s64>( MIN((f64)i, MAX((f64)0, (f64)v)) ));
  }
  }*/

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, RoundAndSaturate)

GTEST_TEST(CoreTech_Common, Rotate90)
{
  const s32 arrayHeight = 4;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s16> in(arrayHeight, arrayHeight, scratchOnchip);
  Array<s16> out(arrayHeight, arrayHeight, scratchOnchip);
  Array<s16> out90_groundTruth(arrayHeight, arrayHeight, scratchOnchip);
  Array<s16> out180_groundTruth(arrayHeight, arrayHeight, scratchOnchip);
  Array<s16> out270_groundTruth(arrayHeight, arrayHeight, scratchOnchip);

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

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> original(arrayHeight, arrayWidth, scratchOnchip);
  u8 * compressed = reinterpret_cast<u8*>( scratchOnchip.Allocate(maxCompressedLength) );

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
    scratchOnchip);

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

    //CoreTechPrint("%f = %d\n", f32s[i], s32s[i]);
    ASSERT_TRUE(s32s[i] == s32s_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, SerializedBuffer)
{
  const s32 segment1Length = 32;
  const s32 segment2Length = 64;
  const s32 segment3Length = 128;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  u8 * segment1 = reinterpret_cast<u8*>( scratchOnchip.Allocate(segment1Length) );
  u8 * segment2 = reinterpret_cast<u8*>( scratchOnchip.Allocate(segment2Length) );
  u8 * segment3 = reinterpret_cast<u8*>( scratchOnchip.Allocate(segment3Length) );

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

  CoreTechPrint("CRC code: 0x%x\n", crc);

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
  ////CoreTechPrint("\n\n\n\n");

  //Anki::Cozmo::HAL::USBSendBuffer(reinterpret_cast<u8*>(&buffer[0]), 100*sizeof(u32));

  //while(1) {}

  const s32 segment1Length = 32;
  const s32 segment2Length = 64;
  const s32 segment3Length = 128;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  void * segment1 = scratchOnchip.Allocate(segment1Length);
  void * segment2 = scratchOnchip.Allocate(segment2Length);
  void * segment3 = scratchOnchip.Allocate(segment3Length);

  ASSERT_TRUE(segment1 != NULL);
  ASSERT_TRUE(segment2 != NULL);
  ASSERT_TRUE(segment3 != NULL);

  MemoryStackIterator msi(scratchOnchip);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 in_data[12] = {
    1, 2, 3, 4,
    5, 6, 7, 8,
    9, 10, 11, 12};

  Array<s32> in(3,4,scratchOnchip);
  Array<s16> out(4,3,scratchOnchip);

  in.Set(in_data, 12);

  Matrix::Transpose(in, out);

  const s32 out_groundTruth_data[12] = {
    1, 5, 9,
    2, 6, 10,
    3, 7, 11,
    4, 8, 12};

  Array<s16> out_groundTruth(4,3,scratchOnchip);

  out_groundTruth.SetCast<s32>(out_groundTruth_data, 12);

  ASSERT_TRUE(AreElementwiseEqual<s16>(out, out_groundTruth));

  //in.Print("in");
  //out.Print("out");

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, MatrixTranspose)

GTEST_TEST(CoreTech_Common, CholeskyDecomposition)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const f32 A_data[9] = {
    2.7345f, 1.8859f, 2.0785f,
    1.8859f, 2.2340f, 2.0461f,
    2.0785f, 2.0461f, 2.7591f};

  const f32 B_data[6] = {
    1.0f, 3.0f, 5.0f,
    2.0f, 4.0f, 6.0f};

  Array<f32> A_L(3,3,scratchOnchip);
  Array<f32> Bt_Xt(2,3,scratchOnchip);

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

  Array<f32> L_groundTruth(3,3,scratchOnchip);
  Array<f32> Xt_groundTruth(2,3,scratchOnchip);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 arr_data[15] = {
    81, 10, 16,
    91, 28, 97,
    13, 55, 96,
    91, 96, 49,
    63, 96, 80};

  Array<s32> arr(5,3,scratchOnchip);
  Array<s32> arrIndexes(5,3,scratchOnchip);

  Array<s32> sortedArr_groundTruth(5,3,scratchOnchip);
  Array<s32> sortedArrIndexes_groundTruth(5,3,scratchOnchip);

  ASSERT_TRUE(arr.IsValid());
  ASSERT_TRUE(sortedArr_groundTruth.IsValid());
  ASSERT_TRUE(sortedArrIndexes_groundTruth.IsValid());

  // sortWhichDimension==0 sortAscending==false
  {
    const s32 sortedArr_groundTruthData[15] = {
      91, 96, 97,
      91, 96, 96,
      81, 55, 80,
      63, 28, 49,
      13, 10, 16};

    // These indexes are different, because quicksort doesn't maintain order of identical elements
    const s32 sortedArrIndexes_insertionGroundTruthData[15] = {
      1, 3, 1,
      3, 4, 2,
      0, 2, 4,
      4, 1, 3,
      2, 0, 0};

    const s32 sortedArrIndexes_quickGroundTruthData[15] = {
      3, 4, 1,
      1, 3, 2,
      0, 2, 4,
      4, 1, 3,
      2, 0, 0};

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);
    ASSERT_TRUE(sortedArr_groundTruth[0][0] == 91);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::InsertionSort(arr, arrIndexes, 0, false) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_insertionGroundTruthData, 15) == 15);
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));

    // Quick sort
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::QuickSort(arr, arrIndexes, 0, false, 0, 0xFFFF, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_quickGroundTruthData, 15) == 15);
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));
  }

  // sortWhichDimension==0 sortAscending==true
  {
    const s32 sortedArr_groundTruthData[15] = {
      13, 10, 16,
      63, 28, 49,
      81, 55, 80,
      91, 96, 96,
      91, 96, 97};

    const s32 sortedArrIndexes_groundTruthData[15] = {
      2, 0, 0,
      4, 1, 3,
      0, 2, 4,
      1, 3, 2,
      3, 4, 1};

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_groundTruthData, 15) == 15);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::InsertionSort(arr, arrIndexes, 0, true) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));

    // Quick sort
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::QuickSort(arr, arrIndexes, 0, true, 0, 0xFFFF, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==false
  {
    const s32 sortedArr_groundTruthData[15] = {
      81, 16, 10,
      97, 91, 28,
      96, 55, 13,
      96, 91, 49,
      96, 80, 63};

    const s32 sortedArrIndexes_groundTruthData[15] = {
      0, 2, 1,
      2, 0, 1,
      2, 1, 0,
      1, 0, 2,
      1, 2, 0};

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_groundTruthData, 15) == 15);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::InsertionSort(arr, arrIndexes, 1, false) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));

    // Quick sort
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::QuickSort(arr, arrIndexes, 1, false, 0, 0xFFFF, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==true
  {
    const s32 sortedArr_groundTruthData[15] = {
      10, 16, 81,
      28, 91, 97,
      13, 55, 96,
      49, 91, 96,
      63, 80, 96};

    const s32 sortedArrIndexes_groundTruthData[15] = {
      1, 2, 0,
      1, 0, 2,
      0, 1, 2,
      2, 0, 1,
      0, 2, 1};

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData, 15) == 15);
    ASSERT_TRUE(sortedArrIndexes_groundTruth.Set(sortedArrIndexes_groundTruthData, 15) == 15);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::InsertionSort(arr, arrIndexes, 1, true) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));

    // Quick sort
    ASSERT_TRUE(arr.Set(arr_data, 15) == 15);
    ASSERT_TRUE(Matrix::QuickSort(arr, arrIndexes, 1, true, 0, 0xFFFF, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual(arrIndexes, sortedArrIndexes_groundTruth));
  }

  // Benchmark tests
  const s32 bigArrayHeight = 20;
  const s32 bigArrayWidth = 1000;
  Array<s32> bigArray(bigArrayHeight,bigArrayWidth , scratchOnchip);
  Array<s32> bigArrayIndexes(bigArrayHeight,bigArrayWidth , scratchOnchip);

  for(s32 insertionSortSize=1; insertionSortSize<1000; insertionSortSize+=3) {
    for(s32 y=0; y<bigArrayHeight; y++) {
      s32 * restrict pBigArray = bigArray.Pointer(y,0);
      for(s32 x=0; x<bigArrayWidth; x++) {
        pBigArray[x] = static_cast<u8>(x + 5*y);
      }
    }

    const u32 t0 = GetTimeU32();
    Matrix::QuickSort(bigArray, bigArrayIndexes, 1, true, 0, 0xFFFF, insertionSortSize);
    const u32 t1 = GetTimeU32();

    CoreTechPrint("insertionSortSize %d took %ums\n", insertionSortSize, t1-t0);

    for(s32 y=0; y<bigArrayHeight; y++) {
      s32 * restrict pBigArray = bigArray.Pointer(y,0);
      for(s32 x=1; x<bigArrayWidth; x++) {
        ASSERT_TRUE(pBigArray[x] >= pBigArray[x-1]);
      }
    }

    if(insertionSortSize > 100) {
      insertionSortSize += 99;
    } else if(insertionSortSize > 30) {
      insertionSortSize += 5;
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, MatrixSortWithIndexes)

GTEST_TEST(CoreTech_Common, MatrixSort)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 arr_data[200] = {
    50, 40, 30, 50, 93, 40, 45, 57, 51, 99,
    48, 12, 5, 64, 54, 66, 20, 32, 65, 55,
    87, 27, 19, 30, 72, 92, 89, 45, 94, 51,
    35, 26, 71, 14, 57, 80, 75, 71, 72, 33,
    44, 33, 71, 47, 3, 48, 87, 88, 40, 43,
    95, 15, 87, 36, 44, 75, 28, 71, 82, 49,
    4, 34, 58, 78, 64, 41, 67, 2, 13, 7,
    96, 12, 7, 77, 52, 96, 66, 67, 6, 88,
    19, 88, 91, 66, 37, 98, 12, 43, 8, 6,
    66, 9, 79, 13, 93, 86, 40, 43, 16, 43,
    58, 92, 28, 2, 82, 38, 27, 12, 32, 82,
    67, 40, 54, 55, 84, 45, 71, 81, 30, 39,
    36, 5, 97, 30, 37, 24, 28, 32, 1, 61,
    61, 34, 71, 93, 59, 78, 89, 24, 53, 81,
    80, 73, 83, 97, 86, 87, 82, 34, 9, 88,
    2, 79, 43, 28, 92, 90, 39, 37, 15, 92,
    8, 54, 47, 79, 66, 55, 49, 54, 62, 19,
    97, 68, 56, 89, 20, 59, 69, 56, 85, 26,
    64, 88, 27, 59, 65, 15, 83, 39, 96, 89,
    23, 5, 74, 88, 7, 89, 60, 39, 57, 59};

  Array<s32> arr(20,10,scratchOnchip);
  Array<s32> sortedArr_groundTruth(20,10,scratchOnchip);

  ASSERT_TRUE(arr.IsValid());
  ASSERT_TRUE(sortedArr_groundTruth.IsValid());

  // sortWhichDimension==0 sortAscending==false
  {
    // toArray(sort(arr, 1, 'descend'))
    const s32 sortedArr_groundTruthData[200] = {
      97, 92, 97, 97, 93, 98, 89, 88, 96, 99,
      96, 88, 91, 93, 93, 96, 89, 81, 94, 92,
      95, 88, 87, 89, 92, 92, 87, 71, 85, 89,
      87, 79, 83, 88, 86, 90, 83, 71, 82, 88,
      80, 73, 79, 79, 84, 89, 82, 67, 72, 88,
      67, 68, 74, 78, 82, 87, 75, 57, 65, 82,
      66, 54, 71, 77, 72, 86, 71, 56, 62, 81,
      64, 40, 71, 66, 66, 80, 69, 54, 57, 61,
      61, 40, 71, 64, 65, 78, 67, 45, 53, 59,
      58, 34, 58, 59, 64, 75, 66, 43, 51, 55,
      50, 34, 56, 55, 59, 66, 60, 43, 40, 51,
      48, 33, 54, 50, 57, 59, 49, 39, 32, 49,
      44, 27, 47, 47, 54, 55, 45, 39, 30, 43,
      36, 26, 43, 36, 52, 48, 40, 37, 16, 43,
      35, 15, 30, 30, 44, 45, 39, 34, 15, 39,
      23, 12, 28, 30, 37, 41, 28, 32, 13, 33,
      19, 12, 27, 28, 37, 40, 28, 32, 9, 26,
      8, 9, 19, 14, 20, 38, 27, 24, 8, 19,
      4, 5, 7, 13, 7, 24, 20, 12, 6, 7,
      2, 5, 5, 2, 3, 15, 12, 2, 1, 6,};

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 0, false) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 0, false, 0, 0xFFFF, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 0, false, 0, 0xFFFF, 5) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==0 sortAscending==false
  {
    // toArray([sort(arr(1:4,:), 1, 'descend'); arr(5:end,:)])
    const s32 sortedArr_groundTruthData[200] = {
      87, 40, 71, 64, 93, 92, 89, 71, 94, 99,
      50, 27, 30, 50, 72, 80, 75, 57, 72, 55,
      48, 26, 19, 30, 57, 66, 45, 45, 65, 51,
      35, 12, 5, 14, 54, 40, 20, 32, 51, 33,
      44, 33, 71, 47, 3, 48, 87, 88, 40, 43,
      95, 15, 87, 36, 44, 75, 28, 71, 82, 49,
      4, 34, 58, 78, 64, 41, 67, 2, 13, 7,
      96, 12, 7, 77, 52, 96, 66, 67, 6, 88,
      19, 88, 91, 66, 37, 98, 12, 43, 8, 6,
      66, 9, 79, 13, 93, 86, 40, 43, 16, 43,
      58, 92, 28, 2, 82, 38, 27, 12, 32, 82,
      67, 40, 54, 55, 84, 45, 71, 81, 30, 39,
      36, 5, 97, 30, 37, 24, 28, 32, 1, 61,
      61, 34, 71, 93, 59, 78, 89, 24, 53, 81,
      80, 73, 83, 97, 86, 87, 82, 34, 9, 88,
      2, 79, 43, 28, 92, 90, 39, 37, 15, 92,
      8, 54, 47, 79, 66, 55, 49, 54, 62, 19,
      97, 68, 56, 89, 20, 59, 69, 56, 85, 26,
      64, 88, 27, 59, 65, 15, 83, 39, 96, 89,
      23, 5, 74, 88, 7, 89, 60, 39, 57, 59, };

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 0, false, 0, 3) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 0, false, 0, 3, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==0 sortAscending==true
  {
    // toArray(sort(arr, 1, 'ascend'))
    const s32 sortedArr_groundTruthData[200] = {
      2, 5, 5, 2, 3, 15, 12, 2, 1, 6,
      4, 5, 7, 13, 7, 24, 20, 12, 6, 7,
      8, 9, 19, 14, 20, 38, 27, 24, 8, 19,
      19, 12, 27, 28, 37, 40, 28, 32, 9, 26,
      23, 12, 28, 30, 37, 41, 28, 32, 13, 33,
      35, 15, 30, 30, 44, 45, 39, 34, 15, 39,
      36, 26, 43, 36, 52, 48, 40, 37, 16, 43,
      44, 27, 47, 47, 54, 55, 45, 39, 30, 43,
      48, 33, 54, 50, 57, 59, 49, 39, 32, 49,
      50, 34, 56, 55, 59, 66, 60, 43, 40, 51,
      58, 34, 58, 59, 64, 75, 66, 43, 51, 55,
      61, 40, 71, 64, 65, 78, 67, 45, 53, 59,
      64, 40, 71, 66, 66, 80, 69, 54, 57, 61,
      66, 54, 71, 77, 72, 86, 71, 56, 62, 81,
      67, 68, 74, 78, 82, 87, 75, 57, 65, 82,
      80, 73, 79, 79, 84, 89, 82, 67, 72, 88,
      87, 79, 83, 88, 86, 90, 83, 71, 82, 88,
      95, 88, 87, 89, 92, 92, 87, 71, 85, 89,
      96, 88, 91, 93, 93, 96, 89, 81, 94, 92,
      97, 92, 97, 97, 93, 98, 89, 88, 96, 99, };

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 0, true) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 0, true, 0, 0xFFFF, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 0, true, 0, 0xFFFF, 7) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==0 sortAscending==true
  {
    // toArray([arr(1,:); sort(arr(2:5,:), 1, 'ascend'); arr(6:end,:)])
    const s32 sortedArr_groundTruthData[200] = {
      50, 40, 30, 50, 93, 40, 45, 57, 51, 99,
      35, 12, 5, 14, 3, 48, 20, 32, 40, 33,
      44, 26, 19, 30, 54, 66, 75, 45, 65, 43,
      48, 27, 71, 47, 57, 80, 87, 71, 72, 51,
      87, 33, 71, 64, 72, 92, 89, 88, 94, 55,
      95, 15, 87, 36, 44, 75, 28, 71, 82, 49,
      4, 34, 58, 78, 64, 41, 67, 2, 13, 7,
      96, 12, 7, 77, 52, 96, 66, 67, 6, 88,
      19, 88, 91, 66, 37, 98, 12, 43, 8, 6,
      66, 9, 79, 13, 93, 86, 40, 43, 16, 43,
      58, 92, 28, 2, 82, 38, 27, 12, 32, 82,
      67, 40, 54, 55, 84, 45, 71, 81, 30, 39,
      36, 5, 97, 30, 37, 24, 28, 32, 1, 61,
      61, 34, 71, 93, 59, 78, 89, 24, 53, 81,
      80, 73, 83, 97, 86, 87, 82, 34, 9, 88,
      2, 79, 43, 28, 92, 90, 39, 37, 15, 92,
      8, 54, 47, 79, 66, 55, 49, 54, 62, 19,
      97, 68, 56, 89, 20, 59, 69, 56, 85, 26,
      64, 88, 27, 59, 65, 15, 83, 39, 96, 89,
      23, 5, 74, 88, 7, 89, 60, 39, 57, 59, };

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 0, true, 1, 4) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 0, true, 1, 4, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 0, true, 1, 4, 2) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==false
  {
    // toArray(sort(arr, 2, 'descend'))
    const s32 sortedArr_groundTruthData[200] = {
      99, 93, 57, 51, 50, 50, 45, 40, 40, 30,
      66, 65, 64, 55, 54, 48, 32, 20, 12, 5,
      94, 92, 89, 87, 72, 51, 45, 30, 27, 19,
      80, 75, 72, 71, 71, 57, 35, 33, 26, 14,
      88, 87, 71, 48, 47, 44, 43, 40, 33, 3,
      95, 87, 82, 75, 71, 49, 44, 36, 28, 15,
      78, 67, 64, 58, 41, 34, 13, 7, 4, 2,
      96, 96, 88, 77, 67, 66, 52, 12, 7, 6,
      98, 91, 88, 66, 43, 37, 19, 12, 8, 6,
      93, 86, 79, 66, 43, 43, 40, 16, 13, 9,
      92, 82, 82, 58, 38, 32, 28, 27, 12, 2,
      84, 81, 71, 67, 55, 54, 45, 40, 39, 30,
      97, 61, 37, 36, 32, 30, 28, 24, 5, 1,
      93, 89, 81, 78, 71, 61, 59, 53, 34, 24,
      97, 88, 87, 86, 83, 82, 80, 73, 34, 9,
      92, 92, 90, 79, 43, 39, 37, 28, 15, 2,
      79, 66, 62, 55, 54, 54, 49, 47, 19, 8,
      97, 89, 85, 69, 68, 59, 56, 56, 26, 20,
      96, 89, 88, 83, 65, 64, 59, 39, 27, 15,
      89, 88, 74, 60, 59, 57, 39, 23, 7, 5, };

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 1, false) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 1, false, 0, 0xFFFF, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 1, false, 0, 0xFFFF, 4) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==false
  {
    // toArray([sort(arr(:,1:2), 2, 'descend'), arr(:,3:end)])
    const s32 sortedArr_groundTruthData[200] = {
      50, 40, 30, 50, 93, 40, 45, 57, 51, 99,
      48, 12, 5, 64, 54, 66, 20, 32, 65, 55,
      87, 27, 19, 30, 72, 92, 89, 45, 94, 51,
      35, 26, 71, 14, 57, 80, 75, 71, 72, 33,
      44, 33, 71, 47, 3, 48, 87, 88, 40, 43,
      95, 15, 87, 36, 44, 75, 28, 71, 82, 49,
      34, 4, 58, 78, 64, 41, 67, 2, 13, 7,
      96, 12, 7, 77, 52, 96, 66, 67, 6, 88,
      88, 19, 91, 66, 37, 98, 12, 43, 8, 6,
      66, 9, 79, 13, 93, 86, 40, 43, 16, 43,
      92, 58, 28, 2, 82, 38, 27, 12, 32, 82,
      67, 40, 54, 55, 84, 45, 71, 81, 30, 39,
      36, 5, 97, 30, 37, 24, 28, 32, 1, 61,
      61, 34, 71, 93, 59, 78, 89, 24, 53, 81,
      80, 73, 83, 97, 86, 87, 82, 34, 9, 88,
      79, 2, 43, 28, 92, 90, 39, 37, 15, 92,
      54, 8, 47, 79, 66, 55, 49, 54, 62, 19,
      97, 68, 56, 89, 20, 59, 69, 56, 85, 26,
      88, 64, 27, 59, 65, 15, 83, 39, 96, 89,
      23, 5, 74, 88, 7, 89, 60, 39, 57, 59,};

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 1, false, 0, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 1, false, 0, 1, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==true
  {
    // toArray(sort(arr, 2, 'ascend'))
    const s32 sortedArr_groundTruthData[200] = {
      30, 40, 40, 45, 50, 50, 51, 57, 93, 99,
      5, 12, 20, 32, 48, 54, 55, 64, 65, 66,
      19, 27, 30, 45, 51, 72, 87, 89, 92, 94,
      14, 26, 33, 35, 57, 71, 71, 72, 75, 80,
      3, 33, 40, 43, 44, 47, 48, 71, 87, 88,
      15, 28, 36, 44, 49, 71, 75, 82, 87, 95,
      2, 4, 7, 13, 34, 41, 58, 64, 67, 78,
      6, 7, 12, 52, 66, 67, 77, 88, 96, 96,
      6, 8, 12, 19, 37, 43, 66, 88, 91, 98,
      9, 13, 16, 40, 43, 43, 66, 79, 86, 93,
      2, 12, 27, 28, 32, 38, 58, 82, 82, 92,
      30, 39, 40, 45, 54, 55, 67, 71, 81, 84,
      1, 5, 24, 28, 30, 32, 36, 37, 61, 97,
      24, 34, 53, 59, 61, 71, 78, 81, 89, 93,
      9, 34, 73, 80, 82, 83, 86, 87, 88, 97,
      2, 15, 28, 37, 39, 43, 79, 90, 92, 92,
      8, 19, 47, 49, 54, 54, 55, 62, 66, 79,
      20, 26, 56, 56, 59, 68, 69, 85, 89, 97,
      15, 27, 39, 59, 64, 65, 83, 88, 89, 96,
      5, 7, 23, 39, 57, 59, 60, 74, 88, 89,};

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 1, true) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 1, true, 0, 0xFFFF, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 1, true, 0, 0xFFFF, 19) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==true
  {
    // toArray([arr(:,1), sort(arr(:,2:3), 2, 'ascend'), arr(:,4:end)])
    const s32 sortedArr_groundTruthData[200] = {
      50, 30, 40, 50, 93, 40, 45, 57, 51, 99,
      48, 5, 12, 64, 54, 66, 20, 32, 65, 55,
      87, 19, 27, 30, 72, 92, 89, 45, 94, 51,
      35, 26, 71, 14, 57, 80, 75, 71, 72, 33,
      44, 33, 71, 47, 3, 48, 87, 88, 40, 43,
      95, 15, 87, 36, 44, 75, 28, 71, 82, 49,
      4, 34, 58, 78, 64, 41, 67, 2, 13, 7,
      96, 7, 12, 77, 52, 96, 66, 67, 6, 88,
      19, 88, 91, 66, 37, 98, 12, 43, 8, 6,
      66, 9, 79, 13, 93, 86, 40, 43, 16, 43,
      58, 28, 92, 2, 82, 38, 27, 12, 32, 82,
      67, 40, 54, 55, 84, 45, 71, 81, 30, 39,
      36, 5, 97, 30, 37, 24, 28, 32, 1, 61,
      61, 34, 71, 93, 59, 78, 89, 24, 53, 81,
      80, 73, 83, 97, 86, 87, 82, 34, 9, 88,
      2, 43, 79, 28, 92, 90, 39, 37, 15, 92,
      8, 47, 54, 79, 66, 55, 49, 54, 62, 19,
      97, 56, 68, 89, 20, 59, 69, 56, 85, 26,
      64, 27, 88, 59, 65, 15, 83, 39, 96, 89,
      23, 5, 74, 88, 7, 89, 60, 39, 57, 59, };

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 1, true, 1, 2) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 1, true, 1, 2, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  // sortWhichDimension==1 sortAscending==true
  {
    // toArray([arr(:,1), sort(arr(:,2:2), 2, 'ascend'), arr(:,3:end)])
    const s32 sortedArr_groundTruthData[200] = {
      50, 40, 30, 50, 93, 40, 45, 57, 51, 99,
      48, 12, 5, 64, 54, 66, 20, 32, 65, 55,
      87, 27, 19, 30, 72, 92, 89, 45, 94, 51,
      35, 26, 71, 14, 57, 80, 75, 71, 72, 33,
      44, 33, 71, 47, 3, 48, 87, 88, 40, 43,
      95, 15, 87, 36, 44, 75, 28, 71, 82, 49,
      4, 34, 58, 78, 64, 41, 67, 2, 13, 7,
      96, 12, 7, 77, 52, 96, 66, 67, 6, 88,
      19, 88, 91, 66, 37, 98, 12, 43, 8, 6,
      66, 9, 79, 13, 93, 86, 40, 43, 16, 43,
      58, 92, 28, 2, 82, 38, 27, 12, 32, 82,
      67, 40, 54, 55, 84, 45, 71, 81, 30, 39,
      36, 5, 97, 30, 37, 24, 28, 32, 1, 61,
      61, 34, 71, 93, 59, 78, 89, 24, 53, 81,
      80, 73, 83, 97, 86, 87, 82, 34, 9, 88,
      2, 79, 43, 28, 92, 90, 39, 37, 15, 92,
      8, 54, 47, 79, 66, 55, 49, 54, 62, 19,
      97, 68, 56, 89, 20, 59, 69, 56, 85, 26,
      64, 88, 27, 59, 65, 15, 83, 39, 96, 89,
      23, 5, 74, 88, 7, 89, 60, 39, 57, 59, };

    ASSERT_TRUE(sortedArr_groundTruth.Set(sortedArr_groundTruthData,200) == 200);

    // Insertion sort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::InsertionSort(arr, 1, true, 1, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));

    // Quicksort
    ASSERT_TRUE(arr.Set(arr_data,200) == 200);
    ASSERT_TRUE(Matrix::QuickSort(arr, 1, true, 1, 1, 1) == RESULT_OK);
    ASSERT_TRUE(AreElementwiseEqual(arr, sortedArr_groundTruth));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, MatrixSort)

GTEST_TEST(CoreTech_Common, MatrixMultiplyTranspose)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s32> in1(2,3,scratchOnchip);
  Array<s32> in2(2,3,scratchOnchip);

  in1[0][0] = 1; in1[0][1] = 2; in1[0][2] = 3;
  in1[1][0] = 4; in1[1][1] = 5; in1[1][2] = 6;

  in2[0][0] = 10; in2[0][1] = 11; in2[0][2] = 12;
  in2[1][0] = 13; in2[1][1] = 14; in2[1][2] = 15;

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> out(2,2,scratchOnchip);
    const Result result = Matrix::MultiplyTranspose<s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print("out");

    ASSERT_TRUE(out[0][0] == 68);
    ASSERT_TRUE(out[0][1] == 86);
    ASSERT_TRUE(out[1][0] == 167);
    ASSERT_TRUE(out[1][1] == 212);
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> out(2,2,scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s32> in(2,2,scratchOnchip);
  in[0][0] = 1; in[0][1] = 2;
  in[1][0] = 3; in[1][1] = 4;

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> out = Matrix::Reshape<s32,s32>(true, in, 4, 1, scratchOnchip);

    ASSERT_TRUE(out[0][0] == 1);
    ASSERT_TRUE(out[1][0] == 3);
    ASSERT_TRUE(out[2][0] == 2);
    ASSERT_TRUE(out[3][0] == 4);
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> out = Matrix::Reshape<s32,s32>(false, in, 4, 1, scratchOnchip);

    ASSERT_TRUE(out[0][0] == 1);
    ASSERT_TRUE(out[1][0] == 2);
    ASSERT_TRUE(out[2][0] == 3);
    ASSERT_TRUE(out[3][0] == 4);
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> out = Matrix::Reshape<s32,s32>(true, in, 1, 4, scratchOnchip);

    ASSERT_TRUE(out[0][0] == 1);
    ASSERT_TRUE(out[0][1] == 3);
    ASSERT_TRUE(out[0][2] == 2);
    ASSERT_TRUE(out[0][3] == 4);
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> out = Matrix::Reshape<s32,s32>(false, in, 1, 4, scratchOnchip);

    ASSERT_TRUE(out[0][0] == 1);
    ASSERT_TRUE(out[0][1] == 2);
    ASSERT_TRUE(out[0][2] == 3);
    ASSERT_TRUE(out[0][3] == 4);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Reshape)

GTEST_TEST(CoreTech_Common, ArrayPatterns)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 arrayHeight = 3;
  const s32 arrayWidth = 2;

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s16> out = Zeros<s16>(arrayHeight, arrayWidth, scratchOnchip);

    for(s32 y=0; y<arrayHeight; y++) {
      const s16 * const pOut = out.Pointer(y, 0);

      for(s32 x=0; x<arrayWidth; x++) {
        ASSERT_TRUE(pOut[x] == 0);
      }
    }
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<u8> out = Ones<u8>(arrayHeight, arrayWidth, scratchOnchip);

    for(s32 y=0; y<arrayHeight; y++) {
      const u8 * const pOut = out.Pointer(y, 0);

      for(s32 x=0; x<arrayWidth; x++) {
        ASSERT_TRUE(pOut[x] == 1);
      }
    }
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);
    Array<f64> out = Eye<f64>(arrayHeight, arrayWidth, scratchOnchip);

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
    PUSH_MEMORY_STACK(scratchOnchip);

    // [logspace(-3,1,5), 3.14159]
    const f32 inData[6] = {0.001f, 0.01f, 0.1f, 1.0f, 10.0f, 3.14159f};

    Array<f32> expIn(arrayHeight, arrayWidth, scratchOnchip);
    expIn.Set(inData, 6);

    Array<f32> out = Exp<f32>(expIn, scratchOnchip);

    const f32 out_groundTruthData[6] = {1.00100050016671f, 1.01005016708417f, 1.10517091807565f, 2.71828182845905f, 22026.4657948067f, 23.1406312269550f};

    Array<f32> out_groundTruth = Array<f32>(3,2,scratchOnchip);
    out_groundTruth.Set(out_groundTruthData, 6);

    //out.Print("out");
    //out_groundTruth.Print("out_groundTruth");

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold(out, out_groundTruth, .05, .001));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, ArrayPatterns)

GTEST_TEST(CoreTech_Common, Interp2_Projective_oneDimensional)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> reference(3,5,scratchOnchip);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid((-0.9:0.9:6), (-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));

  Array<f32> homography = Eye<f32>(3,3,scratchOnchip);
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
  Array<u8> result(1, numElements, scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> reference(3,5,scratchOnchip);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid((-0.9:0.9:6), (-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));

  Array<f32> homography = Eye<f32>(3,3,scratchOnchip);
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

  Array<u8> result(mesh.get_yGridVector().get_size(), mesh.get_xGridVector().get_size(), scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> reference(3,5,scratchOnchip);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid((-0.9:0.9:6), (-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));

  Array<f32> homography = Eye<f32>(3,3,scratchOnchip);
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
  Array<u8> result(mesh.get_yGridVector().get_size(), mesh.get_xGridVector().get_size(), scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> reference(3,5,scratchOnchip);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid((-0.9:0.9:6), (-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));

  Array<f32> homography = Eye<f32>(3,3,scratchOnchip);
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
  Array<u8> result(1, numElements, scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> reference(3,5,scratchOnchip);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid(1+(-0.9:0.9:6), 1+(-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));
  Array<f32> xGridMatrix = mesh.EvaluateX2(scratchOnchip);
  Array<f32> yGridMatrix = mesh.EvaluateY2(scratchOnchip);

  // result = round(interp2(reference, xGridVector, yGridVector)); result(isnan(result)) = 0
  Array<u8> result(xGridMatrix.get_size(0), xGridMatrix.get_size(1), scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> reference(3,5,scratchOnchip);

  // reference = [1:5; 11:15; 21:25];
  reference(0,0,0,-1).Set(LinearSequence<u8>(1,5));
  reference(1,1,0,-1).Set(LinearSequence<u8>(11,15));
  reference(2,2,0,-1).Set(LinearSequence<u8>(21,25));

  // [xGridVector, yGridVector] = meshgrid(1+(-0.9:0.9:6), 1+(-1:1:4));
  Meshgrid<f32> mesh(LinearSequence<f32>(-0.9f,0.9f,6.0f), LinearSequence<f32>(-1.0f,1.0f,4.0f));
  Array<f32> xGridVector = mesh.EvaluateX1(true, scratchOnchip);
  Array<f32> yGridVector = mesh.EvaluateY1(true, scratchOnchip);

  // result = round(interp2(reference, xGridVector(:), yGridVector(:)))
  Array<u8> result(1, xGridVector.get_size(1), scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  // [x,y] = meshgrid(1:5,1:3)
  Meshgrid<s32> mesh(LinearSequence<s32>(1,5), LinearSequence<s32>(1,3));

  Array<s32> out(20,20,scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  // [x,y] = meshgrid(1:5,1:3)
  Meshgrid<s32> mesh(LinearSequence<s32>(1,5), LinearSequence<s32>(1,3));

  Array<s32> out(20,20,scratchOnchip);
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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const f32 differenceThreshold = 1.0e-10f;

  const s32 numTests = 16;
  const s32 tests[numTests][3] = {
    {0,9,0},
    {0,9,1},
    {0,9,10},
    {0,9,10000000},
    {-3,5,0},
    {-3,5,1},
    {-3,5,2},
    {-3,5,3},
    {5,-3,0},
    {5,-3,1},
    {5,-3,2},
    {5,-3,3},
    {-3,-3,0},
    {-3,-3,10},
    {3,3,0},
    {3,3,10}};

  const f32 test_increments[numTests] = {
    1,
    1,
    1,
    9.000000900000090e-07f,
    1,
    1,
    8,
    4,
    1,
    1,
    -8,
    -4,
    0,
    0,
    0,
    0};

  // f32
  for(s32 iTest=0; iTest<numTests; iTest++) {
    LinearSequence<f32> sequence = Linspace<f32>(static_cast<f32>(tests[iTest][0]), static_cast<f32>(tests[iTest][1]), tests[iTest][2]);
    ASSERT_TRUE(sequence.get_size() == tests[iTest][2]);
    ASSERT_TRUE(ABS(sequence.get_increment() - test_increments[iTest]) < differenceThreshold);
  }

  // f64
  for(s32 iTest=0; iTest<numTests; iTest++) {
    LinearSequence<f64> sequence = Linspace<f64>(static_cast<f64>(tests[iTest][0]), static_cast<f64>(tests[iTest][1]), tests[iTest][2]);
    ASSERT_TRUE(sequence.get_size() == tests[iTest][2]);
    ASSERT_TRUE(ABS(sequence.get_increment() - test_increments[iTest]) < differenceThreshold);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Linspace)

GTEST_TEST(CoreTech_Common, Find_SetArray1)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s32> in1(1,6,scratchOnchip); //in1: 2000 2000 2000 3 4 5

  ASSERT_TRUE(in1.IsValid());

  for(s32 x=0; x<6; x++) {
    in1[0][x] = x;
  }

  in1(0,0,0,2).Set(2000);

  Find<s32,Comparison::LessThanOrEqual<s32,s32>,s32> find(in1, 5);

  Array<s16> inB(6,6,scratchOnchip);

  s16 i1 = 0;
  for(s32 y=0; y<6; y++) {
    for(s32 x=0; x<6; x++) {
      inB[y][x] = i1++;
    }
  }

  // Case 1-1
  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s16> outB = find.SetArray(inB, 0, scratchOnchip);
    ASSERT_TRUE(outB.IsValid());

    ASSERT_TRUE(AreEqualSize(3, 6, outB));

    for(s32 y=0; y<3; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE(outB[y][x] == inB[y+3][x]);
      }
    }
  } // PUSH_MEMORY_STACK(memory);

  // Case 1-2
  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s16> outB = find.SetArray(inB, 1, scratchOnchip);
    ASSERT_TRUE(outB.IsValid());

    ASSERT_TRUE(AreEqualSize(6, 3, outB));

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<f32> in1(1,6,scratchOnchip);
  Array<f32> in2(1,6,scratchOnchip);

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

  Array<s16> inB(6,6,scratchOnchip);

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
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s16> outB = find.SetArray(inB, 0, scratchOnchip);
    ASSERT_TRUE(outB.IsValid());

    ASSERT_TRUE(AreEqualSize(3, 6, outB));

    for(s32 y=0; y<3; y++) {
      for(s32 x=0; x<6; x++) {
        ASSERT_TRUE(outB[y][x] == inB[y+3][x]);
      }
    }
  } // PUSH_MEMORY_STACK(memory);

  // Case 2-2
  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s16> outB = find.SetArray(inB, 1, scratchOnchip);
    ASSERT_TRUE(outB.IsValid());

    ASSERT_TRUE(AreEqualSize(6, 3, outB));

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u16> in1(5,6,scratchOnchip);
  Array<u16> in2(5,6,scratchOnchip);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s32> in1(1,6,scratchOnchip);
  Array<s32> in2(1,6,scratchOnchip);

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

  ASSERT_TRUE(find.Evaluate(yIndexes, xIndexes, scratchOnchip) == RESULT_OK);

  ASSERT_TRUE(yIndexes.IsValid());
  ASSERT_TRUE(xIndexes.IsValid());
  ASSERT_TRUE(yIndexes.get_size(0) == 1);
  ASSERT_TRUE(yIndexes.get_size(1) == 0);
  ASSERT_TRUE(yIndexes.get_numElements() == 0);
  ASSERT_TRUE(xIndexes.get_size(0) == 1);
  ASSERT_TRUE(xIndexes.get_size(1) == 0);
  ASSERT_TRUE(xIndexes.get_numElements() == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, ZeroSizedArray)

GTEST_TEST(CoreTech_Common, Find_Evaluate1D)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s32> in1(1,6,scratchOnchip);
  Array<s32> in2(1,6,scratchOnchip);

  Array<s32> in1b(6,1,scratchOnchip);
  Array<s32> in2b(6,1,scratchOnchip);

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
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> yIndexes;
    Array<s32> xIndexes;

    ASSERT_TRUE(find.Evaluate(yIndexes, xIndexes, scratchOnchip) == RESULT_OK);

    ASSERT_TRUE(AreEqualSize(1, 3, yIndexes));
    ASSERT_TRUE(AreEqualSize(1, 3, xIndexes));

    //yIndexes.Print("yIndexes");
    //xIndexes.Print("xIndexes");

    const s32 yIndexes_groundTruth[3] = {0, 0, 0};
    const s32 xIndexes_groundTruth[3] = {3, 4, 5};

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(yIndexes[0][i] == yIndexes_groundTruth[i]);
      ASSERT_TRUE(xIndexes[0][i] == xIndexes_groundTruth[i]);
    }
  } // PUSH_MEMORY_STACK(scratchOnchip)

  // 1D indexes horizontal
  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> indexes;

    ASSERT_TRUE(find.Evaluate(indexes, scratchOnchip) == RESULT_OK);

    ASSERT_TRUE(AreEqualSize(1, 3, indexes));

    //indexes.Print("indexes");

    const s32 indexes_groundTruth[3] = {3, 4, 5};

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(indexes[0][i] == indexes_groundTruth[i]);
    }
  } // PUSH_MEMORY_STACK(scratchOnchip)

  // 2D indexes vertical
  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> yIndexes;
    Array<s32> xIndexes;

    ASSERT_TRUE(findB.Evaluate(yIndexes, xIndexes, scratchOnchip) == RESULT_OK);

    ASSERT_TRUE(AreEqualSize(1, 3, yIndexes));
    ASSERT_TRUE(AreEqualSize(1, 3, xIndexes));

    //yIndexes.Print("yIndexes");
    //xIndexes.Print("xIndexes");

    const s32 xIndexes_groundTruth[3] = {0, 0, 0};
    const s32 yIndexes_groundTruth[3] = {3, 4, 5};

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(yIndexes[0][i] == yIndexes_groundTruth[i]);
      ASSERT_TRUE(xIndexes[0][i] == xIndexes_groundTruth[i]);
    }
  } // PUSH_MEMORY_STACK(scratchOnchip)

  // 1D indexes vertical
  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> indexes;

    ASSERT_TRUE(findB.Evaluate(indexes, scratchOnchip) == RESULT_FAIL_INVALID_PARAMETER);
  } // PUSH_MEMORY_STACK(scratchOnchip)

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Find_Evaluate1D)

GTEST_TEST(CoreTech_Common, Find_Evaluate2D)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s32> in1(5,6,scratchOnchip);
  Array<s32> in2(5,6,scratchOnchip);

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

  ASSERT_TRUE(find.Evaluate(yIndexes, scratchOnchip) == RESULT_FAIL_INVALID_PARAMETER);
  ASSERT_TRUE(find.Evaluate(yIndexes, xIndexes, scratchOnchip) == RESULT_OK);

  ASSERT_TRUE(AreEqualSize(1, 22, yIndexes));
  ASSERT_TRUE(AreEqualSize(1, 22, xIndexes));

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s32> in1(5,6,scratchOnchip);
  Array<s32> in2(5,6,scratchOnchip);
  Array<s32> out(5,6,scratchOnchip);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<s32> in1(5,6,scratchOnchip);
  Array<s32> in2(5,6,scratchOnchip);
  Array<s32> out(5,6,scratchOnchip);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> array1(5,6,scratchOnchip);
  Array<u8> array2(5,6,scratchOnchip);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> array1(20,30,scratchOnchip);

  // This is okay
  ArraySlice<u8> slice1 = array1(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  ASSERT_TRUE(slice1.IsValid());

  const Array<u8> array2(20,30,scratchOnchip);

  // Will not compile
  //ArraySlice<u8> slice2 = array2(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  // This is okay
  ConstArraySlice<u8> slice1b = array1(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  ASSERT_TRUE(slice1b.IsValid());

  // This is okay
  ConstArraySlice<u8> slice2 = array2(LinearSequence<s32>(1,5), LinearSequence<s32>(0,7,30));

  ASSERT_TRUE(slice2.IsValid());

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, SliceArrayCompileTest)

GTEST_TEST(CoreTech_Common, MatrixMinAndMaxAndSum)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> array(5,5,scratchOnchip);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> array1(20,30,scratchOnchip);
  Array<u8> array2(20,30,scratchOnchip);
  Array<u8> array3(20,30,scratchOnchip);

  ASSERT_TRUE(array1.IsValid());
  ASSERT_TRUE(array2.IsValid());
  ASSERT_TRUE(array3.IsValid());

  ASSERT_TRUE(array1.IsValid());
  ASSERT_TRUE(array1.Resize(20,15,scratchOnchip) == RESULT_FAIL_UNINITIALIZED_MEMORY);
  ASSERT_TRUE(!array1.IsValid());

  ASSERT_TRUE(array3.IsValid());
  ASSERT_TRUE(array3.Resize(20,15,scratchOnchip) == RESULT_OK);
  ASSERT_TRUE(array3.IsValid());

  ASSERT_TRUE(array2.IsValid());
  ASSERT_TRUE(array2.Resize(20,15,scratchOnchip) == RESULT_FAIL_UNINITIALIZED_MEMORY);
  ASSERT_TRUE(!array2.IsValid());

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, ReallocateArray)

GTEST_TEST(CoreTech_Common, ReallocateMemoryStack)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  void *memory1 = scratchOnchip.Allocate(100);
  void *memory2 = scratchOnchip.Allocate(100);
  void *memory3 = scratchOnchip.Allocate(100);

  ASSERT_TRUE(memory3 != NULL);
  memory3 = scratchOnchip.Reallocate(memory3, 50);
  ASSERT_TRUE(memory3 != NULL);

  ASSERT_TRUE(memory2 != NULL);
  memory2 = scratchOnchip.Reallocate(memory2, 50);
  ASSERT_TRUE(memory2 == NULL);

  ASSERT_TRUE(memory1 != NULL);
  memory1 = scratchOnchip.Reallocate(memory1, 50);
  ASSERT_TRUE(memory1 == NULL);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, ReallocateMemoryStack)

GTEST_TEST(CoreTech_Common, LinearSequence)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  // Test s32 sequences
  const s32 sequenceLimits1[15][3] = {
    {0,1,1}, {0,2,1}, {-1,1,1}, {-1,2,1}, {-1,3,1},
    {-50,2,-3}, {-3,3,-50}, {10,-4,1}, {10,3,1}, {-10,3,1},
    {-7,7,0}, {12,-4,-10}, {100,-1,0}, {0,3,100}, {0,2,10000}};

  // [length(0:1:1), length(0:2:1), length(-1:1:1), length(-1:2:1), length(-1:3:1), length(-50:2:-3), length(-3:3:-50), length(10:-4:1), length(10:3:1), length(-10:3:1), length(-7:7:0), length(12:-4:-10), length(100:-1:0), length(0:3:100), length(0:2:10000)]'
  const s32 length1_groundTruth[15] = {2, 1, 3, 2, 1, 24, 0, 3, 0, 4, 2, 6, 101, 34, 5001};

  for(s32 i=0; i<15; i++) {
    LinearSequence<s32> sequence(sequenceLimits1[i][0], sequenceLimits1[i][1], sequenceLimits1[i][2]);
    //CoreTechPrint("A %d) %d %d\n", i, sequence.get_size(), length1_groundTruth[i]);
    ASSERT_TRUE(sequence.get_size() == length1_groundTruth[i]);
  }

  // Tests f32 sequences
  const f32 sequenceLimits2[25][3] = {
    {0.0f,1.0f,1.0f}, {0.0f,2.0f,1.0f}, {-1.0f,1.0f,1.0f}, {-1.0f,2.0f,1.0f}, {-1.0f,3.0f,1.0f},
    {-50.0f,2.0f,-3.0f}, {-3.0f,3.0f,-50.0f}, {10.0f,-4.0f,1.0f}, {10.0f,3.0f,1.0f}, {-10.0f,3.0f,1.0f},
    {-7.0f,7.0f,0.0f}, {12.0f,-4.0f,-10.0f}, {100.0f,-1.0f,0.0f}, {0.0f,3.0f,100.0f}, {0.0f,2.0f,10000.0f},
    {0.1f,0.1f,0.1f}, {0.1f,0.11f,0.3f}, {0.1f,0.09f,0.3f}, {-0.5f,0.1f,0.5f}, {0.3f,0.1f,0.2f},
    {-0.5f,-0.05f,-0.4f}, {-0.5f,0.6f,0.0f}, {-0.5f,0.25f,0.0f}, {-4.0f,0.01f,100.0f}, {0.1f,-0.1f,-0.4f}};

  // [length(0.1:0.1:0.1), length(0.1:0.11:0.3), length(0.1:0.09:0.3), length(-0.5:0.1:0.5), length(0.3:0.1:0.2), length(-0.5:-0.05:-0.4), length(-0.5:0.6:0.0), length(-0.5:0.25:0.0), length(-4.0:0.01:100.0), length(0.1:-0.1:-0.4)]'
  const s32 length2_groundTruth[25] = {2, 1, 3, 2, 1, 24, 0, 3, 0, 4, 2, 6, 101, 34, 5001, 1, 2, 3, 11, 0, 0, 1, 3, 10401, 6};

  for(s32 i=0; i<25; i++) {
    LinearSequence<f32> sequence(sequenceLimits2[i][0], sequenceLimits2[i][1], sequenceLimits2[i][2]);
    //CoreTechPrint("B %d) %d %d\n", i, sequence.get_size(), length2_groundTruth[i]);
    ASSERT_TRUE(sequence.get_size() == length2_groundTruth[i]);
  }

  // Test sequence assignment to an Array
  const LinearSequence<s32> sequence(-4,2,4);

  // Test a normal Array
  {
    PUSH_MEMORY_STACK(scratchOnchip);
    Array<s32> sequenceArray = sequence.Evaluate(scratchOnchip);

    ASSERT_TRUE(sequenceArray.IsValid());

    const s32 sequenceArray_groundTruth[5] = {-4, -2, 0, 2, 4};

    ASSERT_TRUE(AreEqualSize(1, 5, sequenceArray));

    for(s32 i=0; i<5; i++) {
      ASSERT_TRUE(sequenceArray[0][i] == sequenceArray_groundTruth[i]);
    }
  } // PUSH_MEMORY_STACK(scratchOnchip)

  // Test an ArraySlice
  {
    PUSH_MEMORY_STACK(scratchOnchip);

    Array<s32> sequenceArray(4,100,scratchOnchip);

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
  } // PUSH_MEMORY_STACK(scratchOnchip)

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, LinearSequence)

GTEST_TEST(CoreTech_Common, MemoryStackId)
{
  //CoreTechPrint("%f %f %f %f %f\n", 43423442334324.010203, 15.500, 15.0, 0.05, 0.12004333);

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

  const f64 valP0 = ApproximateExp(0.00001);
  const f64 valP1 = ApproximateExp(0.0001);
  const f64 valP2 = ApproximateExp(0.001);
  const f64 valP3 = ApproximateExp(0.01);
  const f64 valP4 = ApproximateExp(0.1);
  const f64 valP5 = ApproximateExp(0.0);
  const f64 valP6 = ApproximateExp(1.0);

  const f64 valN0 = ApproximateExp(-0.00001);
  const f64 valN1 = ApproximateExp(-0.0001);
  const f64 valN2 = ApproximateExp(-0.001);
  const f64 valN3 = ApproximateExp(-0.01);
  const f64 valN4 = ApproximateExp(-0.1);
  const f64 valN5 = ApproximateExp(-1.0);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

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

  Array<f64> mat1(2, 5, scratchOnchip);
  Array<f64> mat2(5, 3, scratchOnchip);
  Array<f64> matOut(2, 3, scratchOnchip);
  Array<f64> matOut_groundTruth(2, 3, scratchOnchip);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<f64> homography_groundTruth(3, 3, scratchOnchip);
  Array<f64> homography(3, 3, scratchOnchip);
  Array<f64> originalPoints(3, 4, scratchOnchip);
  Array<f64> transformedPoints(3, 4, scratchOnchip);

  FixedLengthList<Point<f64> > originalPointsList(4, scratchOnchip);
  FixedLengthList<Point<f64> > transformedPointsList(4, scratchOnchip);

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

  bool numericalFailure = false;
  const Result result = Matrix::EstimateHomography(originalPointsList, transformedPointsList, homography, numericalFailure, scratchOnchip);

  ASSERT_TRUE(result == RESULT_OK);
  ASSERT_TRUE(numericalFailure == false);

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
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  // Create a matrix, and manually set a few values
  Array<s16> simpleArray(10, 6, scratchOnchip);
  ASSERT_TRUE(simpleArray.get_buffer() != NULL);
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
    cv::Mat_<s16> simpleArray_cvMat;
    ASSERT_TRUE(ArrayToCvMat(simpleArray, &simpleArray_cvMat) == RESULT_OK);
    CoreTechPrint("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    CoreTechPrint("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat(2,0));

    ASSERT_EQ(7, *simpleArray.Pointer(2,0));
    ASSERT_EQ(7, simpleArray_cvMat(2,0));

    CoreTechPrint("Setting OpenCV matrix\n");
    simpleArray_cvMat(2,0) = 100;
    CoreTechPrint("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));

    CoreTechPrint("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat(2,0));
    ASSERT_EQ(100, *simpleArray.Pointer(2,0));
    ASSERT_EQ(100, simpleArray_cvMat(2,0));

    CoreTechPrint("Setting CoreTech_Common matrix\n");
    *simpleArray.Pointer(2,0) = 42;
    CoreTechPrint("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    CoreTechPrint("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat(2,0));
    ASSERT_EQ(42, *simpleArray.Pointer(2,0));
    ASSERT_EQ(42, simpleArray_cvMat(2,0));
  }

  CoreTechPrint("\n\n");

  // Check that the non-templated OpenCV matrix works
  {
    cv::Mat simpleArray_cvMat;
    ASSERT_TRUE(ArrayToCvMat(simpleArray, &simpleArray_cvMat) == RESULT_OK);
    CoreTechPrint("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    CoreTechPrint("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat.at<s16>(2,0));
    ASSERT_EQ(42, *simpleArray.Pointer(2,0));
    ASSERT_EQ(42, simpleArray_cvMat.at<s16>(2,0));

    CoreTechPrint("Setting OpenCV matrix\n");
    simpleArray_cvMat.at<s16>(2,0) = 300;
    CoreTechPrint("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    CoreTechPrint("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat.at<s16>(2,0));
    ASSERT_EQ(300, *simpleArray.Pointer(2,0));
    ASSERT_EQ(300, simpleArray_cvMat.at<s16>(2,0));

    CoreTechPrint("Setting CoreTech_Common matrix\n");
    *simpleArray.Pointer(2,0) = 90;
    CoreTechPrint("simpleArray(2,0) = %d\n", *simpleArray.Pointer(2,0));
    CoreTechPrint("simpleArray_cvMat(2,0) = %d\n", simpleArray_cvMat.at<s16>(2,0));
    ASSERT_EQ(90, *simpleArray.Pointer(2,0));
    ASSERT_EQ(90, simpleArray_cvMat.at<s16>(2,0));
  }
#endif //#if ANKICORETECH_EMBEDDED_USE_OPENCV

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest)

GTEST_TEST(CoreTech_Common, ArraySpecifiedClass)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> simpleArray(3, 3, scratchOnchip);

#define ArraySpecifiedClass_imgDataLength 9
  const s32 imgData[ArraySpecifiedClass_imgDataLength] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1};

  simpleArray.SetCast<s32>(imgData, ArraySpecifiedClass_imgDataLength);

  CoreTechPrint("*simpleArray.Pointer(0,0) = %d\n", *simpleArray.Pointer(0,0));

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

// This test requires a stopwatch, and takes about ten seconds to do manually
//#define TEST_BENCHMARKING
#ifdef TEST_BENCHMARKING
GTEST_TEST(CoreTech_Common, Benchmarking)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  InitBenchmarking();

  BeginBenchmark("testOuterSpace");

  BeginBenchmark("testOuter");

  const double startTimeN1 = GetTimeF64();
  while((GetTimeF64() - startTimeN1) < 2.0/1) {}

  BeginBenchmark("testMiddle");

  const double startTime0 = GetTimeF64();
  while((GetTimeF64() - startTime0) < 1.0/1) {}

  BeginBenchmark("testInner");
  const double startTime1 = GetTimeF64();
  while((GetTimeF64() - startTime1) < 2.0/1) {}
  EndBenchmark("testInner");

  BeginBenchmark("testInner");
  const double startTime2 = GetTimeF64();
  while((GetTimeF64() - startTime2) < 3.0/1) {}
  EndBenchmark("testInner");

  BeginBenchmark("testInner");
  const double startTime3 = GetTimeF64();
  while((GetTimeF64() - startTime3) < 3.0/1) {}
  EndBenchmark("testInner");

  EndBenchmark("testMiddle");

  EndBenchmark("testOuter");

  EndBenchmark("testOuterSpace");

  ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

  CoreTechPrint("Done with benchmarking test\n");

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Common, Benchmarking)
#endif //#ifdef TEST_BENCHMARKING

#if defined(__ARM_ARCH_7A__) && defined(RUN_A7_ASSEMBLY_TEST)
#define HUGE_BUFFER_SIZE 100000000
static char hugeBuffer[HUGE_BUFFER_SIZE];

NO_INLINE GTEST_TEST(Coretech_Common, A7Speed)
{
  MemoryStack scratchHuge(&hugeBuffer[0], HUGE_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchHuge));

/*  const s32 numRepeats = 1;
  const s32 numItems = 20000001;*/

/*  const s32 numRepeats = 10000;
  const s32 numItems = 200;*/

  const s32 numRepeats = 10000;
  const s32 numItems = 2000;

  s32 * restrict buffer = reinterpret_cast<s32*>( scratchHuge.Allocate(numItems*sizeof(s32)) );

  s32 total1 = 1, total2 = 1, total3 = 1, total4 = 1, total5 = 1, total6 = 1, total7 = 1, total8 = 1, total9 = 1, total10 = 1, total11 = 1, total12 = 1;

  for(s32 i=0; i<numItems; i++) {
    buffer[i] = i;
  }

/*  s32 mrcRegister = 0;
  asm volatile(
  "MRC p15, 0, %[mrcRegister], c0, c0, 1"
    : [mrcRegister] "+r"(mrcRegister));

  printf("0x%x\n", mrcRegister);*/

  const f64 t0 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    for(s32 i=0; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total1 += curItem;
//      printf("%d %d\n", curItem, total1);
    }
  } // for(s32 j=0; j<10; j++)

  const f64 t1 = GetTimeF64();

//  printf("\n\n");
/*
  for(s32 j=0; j<numRepeats; j++) {
    for(s32 i=0; i<numItems; i++) {
      const s32 curItem = buffer[i];

      asm volatile("add	%0, %0, %2\n\t"
                     : "=r" (total2)
                     : "r" (total2), "r" (curItem));
    }
  }
*/
  const f64 t2 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 * restrict bufferLocal = buffer;
    s32 tmp;
    for(s32 i=0; i<numItems; i++) {
      asm volatile("ldr	%[tmp], [%[buffer]]\n\t"
                   "add %[total], %[total], %[tmp]\n\t"
                   "add %[buffer], %[buffer], #4\n\t"
                     : [total] "+r" (total3), [tmp] "+r" (tmp), [buffer] "+r"(bufferLocal));
    }
  }

  const f64 t3 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 * restrict bufferLocal = buffer;
    s32 tmp;
    s32 i=0;
    //for(s32 i=0; i<numItems; i++) {
      asm volatile(
        ".L_iLoopStart:\n\t"
        "ldr	%[tmp], [%[buffer]]\n\t"
        "add %[i], %[i], #1\n\t"
        "cmp %[i], %[numItems]\n\t"
        "add %[total], %[total], %[tmp]\n\t"
        "add %[buffer], %[buffer], #4\n\t"
        "ble .L_iLoopStart\n\t"
          : [total] "+r" (total4), 
            [tmp] "+r" (tmp), 
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems] "r"(numItems));
//    }
  }

  const f64 t4 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 i=0;

    // 4x unrolled
    const s32 numItems_simd = 4 * (numItems / 4) - 3;
    s32 * restrict bufferLocal = buffer;
    s32 tmp1, tmp2, tmp3, tmp4;
    //for(s32 i=0; i<numItems_simd; i+=4) {
      asm volatile(
        ".L_iLoopStart2:\n\t"
        "ldr	%[tmp1], [%[buffer]]\n\t"
        "ldr	%[tmp2], [%[buffer], #4]\n\t"
        "ldr	%[tmp3], [%[buffer], #8]\n\t"
        "ldr	%[tmp4], [%[buffer], #12]\n\t"
        "add %[i], %[i], #4\n\t"
        "cmp %[i], %[numItems_simd]\n\t"
        "add %[total], %[total], %[tmp1]\n\t"
        "add %[total], %[total], %[tmp2]\n\t"
        "add %[total], %[total], %[tmp3]\n\t"
        "add %[total], %[total], %[tmp4]\n\t"
        "add %[buffer], %[buffer], #16\n\t"
        "ble .L_iLoopStart2\n\t"
          : [total] "+r" (total5), 
            [tmp1] "+r" (tmp1), [tmp2] "+r" (tmp2), [tmp3] "+r" (tmp3), [tmp4] "+r" (tmp4),
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems_simd] "r"(numItems_simd));
//    }

    // Finish up the remainder
    for(; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total5 += curItem;
    }
  }

  const f64 t5 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 i=0;

    // 8x unrolled
    const s32 numItems_simd = 8 * (numItems / 8) - 7;
    s32 * restrict bufferLocal = buffer;
    s32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    //for(s32 i=0; i<numItems_simd; i+=8) {
      asm volatile(
        ".L_iLoopStart3:\n\t"
        "ldr	%[tmp1], [%[buffer]]\n\t"
        "ldr	%[tmp2], [%[buffer], #4]\n\t"
        "ldr	%[tmp3], [%[buffer], #8]\n\t"
        "ldr	%[tmp4], [%[buffer], #12]\n\t"
        "ldr	%[tmp5], [%[buffer], #16]\n\t"
        "ldr	%[tmp6], [%[buffer], #20]\n\t"
        "ldr	%[tmp7], [%[buffer], #24]\n\t"
        "ldr	%[tmp8], [%[buffer], #28]\n\t"
        "add %[i], %[i], #8\n\t"
        "cmp %[i], %[numItems_simd]\n\t"
        "add %[total], %[total], %[tmp1]\n\t"
        "add %[total], %[total], %[tmp2]\n\t"
        "add %[total], %[total], %[tmp3]\n\t"
        "add %[total], %[total], %[tmp4]\n\t"
        "add %[total], %[total], %[tmp5]\n\t"
        "add %[total], %[total], %[tmp6]\n\t"
        "add %[total], %[total], %[tmp7]\n\t"
        "add %[total], %[total], %[tmp8]\n\t"
        "add %[buffer], %[buffer], #32\n\t"
        "ble .L_iLoopStart3\n\t"
          : [total] "+r" (total6), 
            [tmp1] "+r" (tmp1), [tmp2] "+r" (tmp2), [tmp3] "+r" (tmp3), [tmp4] "+r" (tmp4), [tmp5] "+r" (tmp5), [tmp6] "+r" (tmp6), [tmp7] "+r" (tmp7), [tmp8] "+r" (tmp8),
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems_simd] "r"(numItems_simd));
//    }

    // Finish up the remainder
    for(; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total6 += curItem;
    }
  }

  const f64 t6 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 i=0;

    // 8x unrolled
    const s32 numItems_simd = 8 * (numItems / 8) - 7;
    s32 * restrict bufferLocal = buffer;
    s32 tmp;
    //for(s32 i=0; i<numItems_simd; i+=8) {
      asm volatile(
        ".L_iLoopStart4:\n\t"
        "vld1.32 {q10}, [%[buffer]]!\n\t" // 3, 2, 1, 0
        "vld1.32 {q11}, [%[buffer]]!\n\t" // 7, 6, 5, 4
        "add %[i], %[i], #8\n\t"
        "cmp %[i], %[numItems_simd]\n\t"
        "vadd.i32 q10, q10, q11\n\t" // 37, 26, 15, 04
        "vpadd.i32 d20, d20, d21\n\t" // 2367, 0145
        "vpadd.i32 d20, d20, d20\n\t" // 01234567, 01234567
        "vmov.32 %[tmp], d20[0]\n\t"
        "add %[total], %[total], %[tmp]\n\t"
        "ble .L_iLoopStart4\n\t"
          : 
            [total] "+r" (total7), 
            [tmp] "+r" (tmp),
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems_simd] "r"(numItems_simd)
          :
            "q10", "q11", "d20", "d21", "d22", "d23");
//    }

    // Finish up the remainder
    for(; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total7 += curItem;
    }
  }

  const f64 t7 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 i=0;

    // 32x unrolled
    const s32 numItems_simd = 64 * (numItems / 64) - 63;
    s32 * restrict bufferLocal = buffer;
    s32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    //for(s32 i=0; i<numItems_simd; i+=64) {
      asm volatile(
        ".L_iLoopStart5:\n\t"
        "vld1.32 {q0}, [%[buffer]]!\n\t" 
        "vld1.32 {q1}, [%[buffer]]!\n\t" 
        "vld1.32 {q2}, [%[buffer]]!\n\t"
        "vld1.32 {q3}, [%[buffer]]!\n\t"
        "vld1.32 {q4}, [%[buffer]]!\n\t"
        "vld1.32 {q5}, [%[buffer]]!\n\t"
        "vld1.32 {q6}, [%[buffer]]!\n\t"
        "vld1.32 {q7}, [%[buffer]]!\n\t"

        "add %[i], %[i], #32\n\t"

        "vadd.i32 q0,  q0,  q1\n\t" // 1
        "vadd.i32 q2,  q2,  q3\n\t"
        "vadd.i32 q4,  q4,  q5\n\t"
        "vadd.i32 q6,  q6,  q7\n\t"
      
        "vpadd.i32 d0,  d0,  d1\n\t" // 2
        "vpadd.i32 d4,  d4,  d5\n\t"
        "vpadd.i32 d8,  d8,  d9\n\t"
        "vpadd.i32 d12, d12, d13\n\t"

        "vpadd.i32 d0,  d0,  d0\n\t" // 3
        "vpadd.i32 d4,  d4,  d4\n\t"
        "vpadd.i32 d8,  d8,  d8\n\t"
        "vpadd.i32 d12, d12, d12\n\t"

        "vmov.32 %[tmp1], d0[0]\n\t" // 4
        "vmov.32 %[tmp2], d4[0]\n\t"
        "vmov.32 %[tmp3], d8[0]\n\t"
        "vmov.32 %[tmp4], d12[0]\n\t"

        "add %[total], %[total], %[tmp1]\n\t" // 5
        "add %[total], %[total], %[tmp2]\n\t"
        "add %[total], %[total], %[tmp3]\n\t"
        "add %[total], %[total], %[tmp4]\n\t"

        "cmp %[i], %[numItems_simd]\n\t"
        "ble .L_iLoopStart5\n\t"
          : 
            [total] "+r" (total8), 
            [tmp1] "+r" (tmp1), [tmp2] "+r" (tmp2), [tmp3] "+r" (tmp3), [tmp4] "+r" (tmp4), [tmp5] "+r" (tmp5), [tmp6] "+r" (tmp6), [tmp7] "+r" (tmp7), [tmp8] "+r" (tmp8),
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems_simd] "r"(numItems_simd)
          :
            "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
            "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
//    }

    // Finish up the remainder
    for(; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total8 += curItem;
    }
  }

  const f64 t8 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 i=0;

    // 64x unrolled
    const s32 numItems_simd = 64 * (numItems / 64) - 63;
    s32 * restrict bufferLocal = buffer;
    s32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    //for(s32 i=0; i<numItems_simd; i+=64) {
      asm volatile(
        ".L_iLoopStart6:\n\t"
        "vld1.32 {q0}, [%[buffer]]!\n\t" 
        "vld1.32 {q1}, [%[buffer]]!\n\t" 
        "vld1.32 {q2}, [%[buffer]]!\n\t"
        "vld1.32 {q3}, [%[buffer]]!\n\t"
        "vld1.32 {q4}, [%[buffer]]!\n\t"
        "vld1.32 {q5}, [%[buffer]]!\n\t"
        "vld1.32 {q6}, [%[buffer]]!\n\t"
        "vld1.32 {q7}, [%[buffer]]!\n\t"
        "vld1.32 {q8}, [%[buffer]]!\n\t"
        "vld1.32 {q9}, [%[buffer]]!\n\t"
        "vld1.32 {q10}, [%[buffer]]!\n\t"
        "vld1.32 {q11}, [%[buffer]]!\n\t"
        "vld1.32 {q12}, [%[buffer]]!\n\t"
        "vld1.32 {q13}, [%[buffer]]!\n\t"
        "vld1.32 {q14}, [%[buffer]]!\n\t"
        "vld1.32 {q15}, [%[buffer]]!\n\t"

        "add %[i], %[i], #64\n\t"

        "vadd.i32 q0,  q0,  q1\n\t" // 1
        "vadd.i32 q2,  q2,  q3\n\t"
        "vadd.i32 q4,  q4,  q5\n\t"
        "vadd.i32 q6,  q6,  q7\n\t"
        "vadd.i32 q8,  q8,  q9\n\t"
        "vadd.i32 q10, q10, q11\n\t"
        "vadd.i32 q12, q12, q13\n\t"
        "vadd.i32 q14, q14, q15\n\t"
      
        "vpadd.i32 d0,  d0,  d1\n\t" // 2
        "vpadd.i32 d4,  d4,  d5\n\t"
        "vpadd.i32 d8,  d8,  d9\n\t"
        "vpadd.i32 d12, d12, d13\n\t"
        "vpadd.i32 d16, d16, d17\n\t"
        "vpadd.i32 d20, d20, d21\n\t"
        "vpadd.i32 d24, d24, d25\n\t"
        "vpadd.i32 d28, d28, d29\n\t"

        "vpadd.i32 d0,  d0,  d0\n\t" // 3
        "vpadd.i32 d4,  d4,  d4\n\t"
        "vpadd.i32 d8,  d8,  d8\n\t"
        "vpadd.i32 d12, d12, d12\n\t"
        "vpadd.i32 d16, d16, d16\n\t"
        "vpadd.i32 d20, d20, d20\n\t"
        "vpadd.i32 d24, d24, d24\n\t"
        "vpadd.i32 d28, d28, d28\n\t"

        "vmov.32 %[tmp1], d0[0]\n\t" // 4
        "vmov.32 %[tmp2], d4[0]\n\t"
        "vmov.32 %[tmp3], d8[0]\n\t"
        "vmov.32 %[tmp4], d12[0]\n\t"
        "vmov.32 %[tmp5], d16[0]\n\t"
        "vmov.32 %[tmp6], d20[0]\n\t"
        "vmov.32 %[tmp7], d24[0]\n\t"
        "vmov.32 %[tmp8], d28[0]\n\t"

     /*   "add %[tmp1], %[tmp1], %[tmp2]\n\t" // 5
        "add %[tmp3], %[tmp3], %[tmp4]\n\t"
        "add %[tmp5], %[tmp5], %[tmp6]\n\t"
        "add %[tmp7], %[tmp7], %[tmp8]\n\t"

        "add %[tmp1], %[tmp1], %[tmp3]\n\t"
        "add %[tmp5], %[tmp5], %[tmp7]\n\t"

        "add %[tmp1], %[tmp1], %[tmp5]\n\t"

        "add %[total], %[total], %[tmp1]\n\t"*/

        "add %[total], %[total], %[tmp1]\n\t"
        "add %[total], %[total], %[tmp2]\n\t"
        "add %[total], %[total], %[tmp3]\n\t"
        "add %[total], %[total], %[tmp4]\n\t"
        "add %[total], %[total], %[tmp5]\n\t"
        "add %[total], %[total], %[tmp6]\n\t"
        "add %[total], %[total], %[tmp7]\n\t"
        "add %[total], %[total], %[tmp8]\n\t"

        "cmp %[i], %[numItems_simd]\n\t"
        "ble .L_iLoopStart6\n\t"
          : 
            [total] "+r" (total9), 
            [tmp1] "+r" (tmp1), [tmp2] "+r" (tmp2), [tmp3] "+r" (tmp3), [tmp4] "+r" (tmp4), [tmp5] "+r" (tmp5), [tmp6] "+r" (tmp6), [tmp7] "+r" (tmp7), [tmp8] "+r" (tmp8),
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems_simd] "r"(numItems_simd)
          :
            "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
            "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
//    }

    // Finish up the remainder
    for(; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total9 += curItem;
    }
  }

  const f64 t9 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 i=0;

    // 32x unrolled
    const s32 numItems_simd = 32 * (numItems / 32) - 31;
    s32 * restrict bufferLocal = buffer;
    s32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    //for(s32 i=0; i<numItems_simd; i+=64) {
      asm volatile(
        ".L_iLoopStart7:\n\t"
        "vld1.32 {q0}, [%[buffer]]!\n\t" 
        "add %[i], %[i], #32\n\t"
        "vld1.32 {q1}, [%[buffer]]!\n\t" 

        "vadd.i32 q0,  q0,  q1\n\t" // 1

        "vld1.32 {q2}, [%[buffer]]!\n\t"
        "vld1.32 {q3}, [%[buffer]]!\n\t"

        "vpadd.i32 d0,  d0,  d1\n\t"
        "vadd.i32 q2,  q2,  q3\n\t"

        "vld1.32 {q4}, [%[buffer]]!\n\t"
        "vld1.32 {q5}, [%[buffer]]!\n\t"

        "vpadd.i32 d0,  d0,  d0\n\t"
        "vpadd.i32 d4,  d4,  d5\n\t"
        "vadd.i32 q4,  q4,  q5\n\t"

        "vld1.32 {q6}, [%[buffer]]!\n\t"
        "vld1.32 {q7}, [%[buffer]]!\n\t"

        "vpadd.i32 d4,  d4,  d4\n\t"
        "vpadd.i32 d8,  d8,  d9\n\t"
        "vadd.i32 q6,  q6,  q7\n\t"

        "vpadd.i32 d12, d12, d13\n\t"

        "vpadd.i32 d8,  d8,  d8\n\t"
        "vpadd.i32 d12, d12, d12\n\t"

        "vmov.32 %[tmp1], d0[0]\n\t"
        "vmov.32 %[tmp2], d4[0]\n\t"
        "vmov.32 %[tmp3], d8[0]\n\t"
        "vmov.32 %[tmp4], d12[0]\n\t"

        "add %[total], %[total], %[tmp1]\n\t"
        "add %[total], %[total], %[tmp2]\n\t"
        "add %[total], %[total], %[tmp3]\n\t"
        "add %[total], %[total], %[tmp4]\n\t"

        "cmp %[i], %[numItems_simd]\n\t"
        "ble .L_iLoopStart7\n\t"
          : 
            [total] "+r" (total10), 
            [tmp1] "+r" (tmp1), [tmp2] "+r" (tmp2), [tmp3] "+r" (tmp3), [tmp4] "+r" (tmp4), [tmp5] "+r" (tmp5), [tmp6] "+r" (tmp6), [tmp7] "+r" (tmp7), [tmp8] "+r" (tmp8),
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems_simd] "r"(numItems_simd)
          :
            "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
            "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
//    }

    // Finish up the remainder
    for(; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total10 += curItem;
    }
  }

  const f64 t10 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 i=0;

    // 32x unrolled
    const s32 numItems_simd = 32 * (numItems / 32) - 31;
    s32 * restrict bufferLocal = buffer;
    s32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    //for(s32 i=0; i<numItems_simd; i+=64) {
      asm volatile(
        ".L_iLoopStart8:\n\t"
        "vld1.32 {q0}, [%[buffer]]!\n\t" 
        
        "add %[i], %[i], #32\n\t"

        "vld1.32 {q1}, [%[buffer]]!\n\t" 

        "vadd.i32 q0,  q0,  q1\n\t"

        "vld1.32 {q2}, [%[buffer]]!\n\t"

        "vpadd.i32 d0,  d0,  d1\n\t"

        "vld1.32 {q3}, [%[buffer]]!\n\t"

        "vadd.i32 q2,  q2,  q3\n\t"

        "vld1.32 {q4}, [%[buffer]]!\n\t"

        "vpadd.i32 d0,  d0,  d0\n\t"
        "vpadd.i32 d4,  d4,  d5\n\t"

        "vld1.32 {q5}, [%[buffer]]!\n\t"
        "vadd.i32 q4,  q4,  q5\n\t"

        "vld1.32 {q6}, [%[buffer]]!\n\t"

        "vpadd.i32 d4,  d4,  d4\n\t"
        "vpadd.i32 d8,  d8,  d9\n\t"

        "vld1.32 {q7}, [%[buffer]]!\n\t"

        "vadd.i32 q6,  q6,  q7\n\t"

        "vpadd.i32 d12, d12, d13\n\t"

        "vpadd.i32 d8,  d8,  d8\n\t"
        "vpadd.i32 d12, d12, d12\n\t"

        "vmov.32 %[tmp1], d0[0]\n\t"
        "vmov.32 %[tmp2], d4[0]\n\t"
        "vmov.32 %[tmp3], d8[0]\n\t"
        "vmov.32 %[tmp4], d12[0]\n\t"

        "add %[total], %[total], %[tmp1]\n\t"
        "add %[total], %[total], %[tmp2]\n\t"
        "add %[total], %[total], %[tmp3]\n\t"
        "add %[total], %[total], %[tmp4]\n\t"

        "cmp %[i], %[numItems_simd]\n\t"
        "ble .L_iLoopStart8\n\t"
          : 
            [total] "+r" (total11), 
            [tmp1] "+r" (tmp1), [tmp2] "+r" (tmp2), [tmp3] "+r" (tmp3), [tmp4] "+r" (tmp4), [tmp5] "+r" (tmp5), [tmp6] "+r" (tmp6), [tmp7] "+r" (tmp7), [tmp8] "+r" (tmp8),
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems_simd] "r"(numItems_simd)
          :
            "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
            "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
//    }

    // Finish up the remainder
    for(; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total11 += curItem;
    }
  }

  const f64 t11 = GetTimeF64();

  for(s32 j=0; j<numRepeats; j++) {
    s32 i=0;

    // 64x unrolled
    const s32 numItems_simd = 64 * (numItems / 64) - 63;
    s32 * restrict bufferLocal = buffer;
    s32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    //for(s32 i=0; i<numItems_simd; i+=64) {
    asm volatile(
        ".L_iLoopStart9:\n\t"
        "vld1.32 {q0}, [%[buffer]]!\n\t" 

        "add %[i], %[i], #64\n\t"
        "cmp %[i], %[numItems_simd]\n\t"

        "vld1.32 {q1}, [%[buffer]]!\n\t" 

        "vadd.i32 q0,  q0,  q1\n\t" // 1

        "vld1.32 {q2}, [%[buffer]]!\n\t"

        "vpadd.i32 d0,  d0,  d1\n\t"

        "vld1.32 {q3}, [%[buffer]]!\n\t"

        "vadd.i32 q2,  q2,  q3\n\t"

        "vld1.32 {q4}, [%[buffer]]!\n\t"

        "vpadd.i32 d4,  d4,  d5\n\t"
        "vpadd.i32 d0,  d0,  d0\n\t"

        "vld1.32 {q5}, [%[buffer]]!\n\t"

        "vadd.i32 q4,  q4,  q5\n\t"

        "vld1.32 {q6}, [%[buffer]]!\n\t"
        "vld1.32 {q7}, [%[buffer]]!\n\t"

        "vadd.i32 q6,  q6,  q7\n\t"

        "vld1.32 {q8}, [%[buffer]]!\n\t"

        "vpadd.i32 d8,  d8,  d9\n\t"
        "vpadd.i32 d4,  d4,  d4\n\t"
        "vmov.32 %[tmp1], d0[0]\n\t"

        "vld1.32 {q9}, [%[buffer]]!\n\t"

        "vadd.i32 q8,  q8,  q9\n\t"

        "vld1.32 {q10}, [%[buffer]]!\n\t"

        "vpadd.i32 d12, d12, d13\n\t"
        "vpadd.i32 d8,  d8,  d8\n\t"
        "vmov.32 %[tmp2], d4[0]\n\t"
        "add %[total], %[total], %[tmp1]\n\t"

        "vld1.32 {q11}, [%[buffer]]!\n\t"

        "vadd.i32 q10, q10, q11\n\t"

        "vld1.32 {q12}, [%[buffer]]!\n\t"

        "vpadd.i32 d16, d16, d17\n\t"
        "vpadd.i32 d12, d12, d12\n\t"
        "vmov.32 %[tmp3], d8[0]\n\t"
        "add %[total], %[total], %[tmp2]\n\t"

        "vld1.32 {q13}, [%[buffer]]!\n\t"

        "vadd.i32 q12, q12, q13\n\t"

        "vld1.32 {q14}, [%[buffer]]!\n\t"

        "vpadd.i32 d20, d20, d21\n\t"
        "vpadd.i32 d16, d16, d16\n\t"
        "vmov.32 %[tmp4], d12[0]\n\t"
        "add %[total], %[total], %[tmp3]\n\t"

        "vld1.32 {q15}, [%[buffer]]!\n\t"

        "vadd.i32 q14, q14, q15\n\t"
      
        "vpadd.i32 d24, d24, d25\n\t"
        "vpadd.i32 d20, d20, d20\n\t"
        "vmov.32 %[tmp5], d16[0]\n\t"
        "add %[total], %[total], %[tmp4]\n\t"

        "vpadd.i32 d28, d28, d29\n\t"
        "vpadd.i32 d24, d24, d24\n\t"
        "vpadd.i32 d28, d28, d28\n\t"
        "add %[total], %[total], %[tmp5]\n\t"

        "vmov.32 %[tmp6], d20[0]\n\t"
        "vmov.32 %[tmp7], d24[0]\n\t"
        "vmov.32 %[tmp8], d28[0]\n\t"

        "add %[total], %[total], %[tmp6]\n\t"
        "add %[total], %[total], %[tmp7]\n\t"
        "add %[total], %[total], %[tmp8]\n\t"

        "ble .L_iLoopStart9\n\t"
          : 
            [total] "+r" (total12), 
            [tmp1] "+r" (tmp1), [tmp2] "+r" (tmp2), [tmp3] "+r" (tmp3), [tmp4] "+r" (tmp4), [tmp5] "+r" (tmp5), [tmp6] "+r" (tmp6), [tmp7] "+r" (tmp7), [tmp8] "+r" (tmp8),
            [buffer] "+r"(bufferLocal),
            [i] "+r"(i)
          :
            [numItems_simd] "r"(numItems_simd)
          :
            "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
            "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31");
//    }

    // Finish up the remainder
    for(; i<numItems; i++) {
      const s32 curItem = buffer[i];
      total12 += curItem;
    }
  }

  const f64 t12 = GetTimeF64();

  printf("\n\n");

  printf("c loop %d in %f\n", total1, t1-t0);
//  printf("%d in %f\n", total2, t2-t1);
  printf("inline assembly %d in %f\n", total3, t3-t2);
  printf("assembly loop %d in %f\n", total4, t4-t3);
  printf("4x unrolled %d in %f\n", total5, t5-t4);
  printf("8x unrolled %d in %f\n", total6, t6-t5);
  printf("simd 8x %d in %f\n", total7, t7-t6);
  printf("simd 32x %d in %f\n", total8, t8-t7);
  printf("simd 64x %d in %f\n", total9, t9-t8);
  printf("simd2 64x %d in %f\n", total10, t10-t9);
  printf("simd3 32x %d in %f\n", total11, t11-t10);
  printf("simd4 64x %d in %f\n", total12, t12-t11);

  ASSERT_TRUE(total1 == (562894465*numRepeats));
//  ASSERT_TRUE(total2 == (562894465*numRepeats));
  ASSERT_TRUE(total3 == (562894465*numRepeats));
  ASSERT_TRUE(total4 == (562894465*numRepeats));
  ASSERT_TRUE(total5 == (562894465*numRepeats));
  ASSERT_TRUE(total6 == (562894465*numRepeats));
  ASSERT_TRUE(total7 == (562894465*numRepeats));
  ASSERT_TRUE(total8 == (562894465*numRepeats));
  ASSERT_TRUE(total9 == (562894465*numRepeats));
  ASSERT_TRUE(total10 == (562894465*numRepeats));
  ASSERT_TRUE(total11 == (562894465*numRepeats));
  ASSERT_TRUE(total12 == (562894465*numRepeats));

  GTEST_RETURN_HERE;
} // GTEST_TEST(Coretech_Common, A7Speed)
#endif // #if defined(__ARM_ARCH_7A__) && defined(RUN_A7_ASSEMBLY_TEST)

#ifdef RUN_PC_ONLY_TESTS

#if ANKICORETECH_EMBEDDED_USE_MATLAB
GTEST_TEST(CoreTech_Common, SimpleMatlabTest1)
{
  matlab.EvalStringEcho("simpleVector = double([1.1,2.1,3.1,4.1,5.1]);");
  double *simpleVector = matlab.Get<double>("simpleVector");

  CoreTechPrint("simple vector:\n%f %f %f %f %f\n", simpleVector[0], simpleVector[1], simpleVector[2], simpleVector[3], simpleVector[4]);

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
  CoreTechPrint("simple matrix:\n");
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

  CoreTechPrint("%f %f\n%f %f\n%f %f\n",
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
#endif // #ifdef RUN_PC_ONLY_TESTS

#if !ANKICORETECH_EMBEDDED_USE_GTEST
s32 RUN_ALL_COMMON_TESTS(s32 &numPassedTests, s32 &numFailedTests)
{
  numPassedTests = 0;
  numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Common, SerializeStrings);
  CALL_GTEST_TEST(CoreTech_Common, DrawQuadrilateral);
  CALL_GTEST_TEST(CoreTech_Common, HostIntrinsics_m4);
  CALL_GTEST_TEST(CoreTech_Common, VectorTypes);
  CALL_GTEST_TEST(CoreTech_Common, RoundUpAndDown);
  CALL_GTEST_TEST(CoreTech_Common, RoundAndSaturate);
  CALL_GTEST_TEST(CoreTech_Common, Rotate90);
  CALL_GTEST_TEST(CoreTech_Common, RunLengthEncode);
  CALL_GTEST_TEST(CoreTech_Common, IsConvex);
  CALL_GTEST_TEST(CoreTech_Common, RoundFloat);
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
  CALL_GTEST_TEST(CoreTech_Common, Interp2_Projective_oneDimensional);
  CALL_GTEST_TEST(CoreTech_Common, Interp2_Projective_twoDimensional);
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

#ifdef TEST_BENCHMARKING
  CALL_GTEST_TEST(CoreTech_Common, Benchmarking);
#endif

#if defined(__ARM_ARCH_7A__) && defined(RUN_A7_ASSEMBLY_TEST)
  CALL_GTEST_TEST(Coretech_Common, A7Speed);
#endif

  return numFailedTests;
} // int RUN_ALL_COMMON_TESTS()
#endif // #if !defined(ANKICORETECH_EMBEDDED_USE_GTEST)
