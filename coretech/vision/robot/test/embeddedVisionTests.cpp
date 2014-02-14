/**
File: embeddedVisionTests.cpp
Author: Peter Barnum
Created: 2013

Various tests of the coretech vision embedded library.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef COZMO_ROBOT
#define COZMO_ROBOT
#endif

#include "anki/common/robot/gtestLight.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/benchmarking_c.h"
#include "anki/common/robot/comparisons.h"
#include "anki/common/robot/arrayPatterns.h"

#include "anki/vision/robot/miscVisionKernels.h"
#include "anki/vision/robot/integralImage.h"
#include "anki/vision/robot/draw_vision.h"
#include "anki/vision/robot/lucasKanade.h"
#include "anki/vision/robot/docking_vision.h"
#include "anki/vision/robot/imageProcessing.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#ifdef __cplusplus
}
#endif

using namespace Anki::Embedded;

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

#if ANKICORETECH_EMBEDDED_USE_MATLAB
Matlab matlab(false);
#endif

#if ANKICORETECH_EMBEDDED_USE_GTEST
#include "gtest/gtest.h"
#endif

// #define RUN_MATLAB_IMAGE_TEST
//#define RUN_MAIN_BIG_MEMORY_TESTS
//#define RUN_ALL_BIG_MEMORY_TESTS
#define RUN_LOW_MEMORY_IMAGE_TESTS
#define RUN_TRACKER_TESTS // equivalent to RUN_BROKEN_KANADE_TESTS
//#define BENCHMARK_AFFINE
//#define RUN_LITTLE_TESTS

//#if defined(RUN_TRACKER_TESTS) && defined(RUN_LOW_MEMORY_IMAGE_TESTS)
//Cannot run tracker and low memory tests at the same time
//#endif

#if defined(RUN_MAIN_BIG_MEMORY_TESTS) || defined(RUN_LOW_MEMORY_IMAGE_TESTS)
#include "../../blockImages/blockImage50_320x240.h"
#endif

#ifdef RUN_TRACKER_TESTS   // This prevents the .elf from loading
#include "../../blockImages/blockImages00189_80x60.h"
#include "../../blockImages/blockImages00190_80x60.h"

#include "../../../systemTestImages/cozmo_2014_01_29_11_41_05_10_320x240.h"
#include "../../../systemTestImages/cozmo_2014_01_29_11_41_05_12_320x240.h"
#endif

#ifdef RUN_ALL_BIG_MEMORY_TESTS
#include "../../blockImages/fiducial105_6ContrastReduced.h"
#include "../src/fiducialMarkerDefinitionType0.h"
#endif

#if defined(USING_MOVIDIUS_COMPILER)
#define DDR_BUFFER_LOCATION __attribute__((section(".ddr.bss")))
#define CMX_BUFFER_LOCATION __attribute__((section(".cmx.bss")))
#elif defined(__EDG__)  // ARM-MDK
#define DDR_BUFFER_LOCATION __attribute__((section(".ram1")))
#define CMX_BUFFER_LOCATION __attribute__((section(".ram1")))
#else
#define DDR_BUFFER_LOCATION
#define CMX_BUFFER_LOCATION
#endif

#if (defined(RUN_LOW_MEMORY_IMAGE_TESTS) || defined(RUN_TRACKER_TESTS)) &&\
  !defined(RUN_MAIN_BIG_MEMORY_TESTS) && !defined(RUN_ALL_BIG_MEMORY_TESTS)
#define DDR_BUFFER_SIZE 320000
#define CMX_BUFFER_SIZE 600000
#else
#define DDR_BUFFER_SIZE 4000000
#define CMX_BUFFER_SIZE 6000000
#endif

DDR_BUFFER_LOCATION char ddrBuffer[DDR_BUFFER_SIZE];
CMX_BUFFER_LOCATION char cmxBuffer[CMX_BUFFER_SIZE];

GTEST_TEST(CoreTech_Vision, LucasKanadeTrackerBinary)
{
  MemoryStack scratch_CMX(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(scratch_CMX.IsValid());

  Array<u8> templateImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratch_CMX);
  Array<u8> nextImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratch_CMX);
  Array<u8> warpedTemplateImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratch_CMX);

  const Quadrilateral<f32> templateQuad(Point<f32>(128,78), Point<f32>(220,74), Point<f32>(229,167), Point<f32>(127,171));
  const u8 edgeDetection_grayvalueThreshold = 100;
  const s32 edgeDetection_minComponentWidth = 2;
  const s32 edgeDetection_maxDetectionsPerType = 3000;

  const s32 matching_maxDistance = 7;
  const s32 matching_maxCorrespondences = 10000;

  templateImage.Set(&cozmo_2014_01_29_11_41_05_10_320x240[0], cozmo_2014_01_29_11_41_05_10_320x240_WIDTH*cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT);
  nextImage.Set(&cozmo_2014_01_29_11_41_05_12_320x240[0], cozmo_2014_01_29_11_41_05_12_320x240_WIDTH*cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT);

  InitBenchmarking();

  BeginBenchmark("LucasKanadeTrackerBinary init");
  TemplateTracker::LucasKanadeTrackerBinary lktb(templateImage, templateQuad, edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, edgeDetection_maxDetectionsPerType, scratch_CMX);
  EndBenchmark("LucasKanadeTrackerBinary init");

  {
    PUSH_MEMORY_STACK(scratch_CMX);

    BeginBenchmark("LucasKanadeTrackerBinary update");
    const Result result = lktb.UpdateTrack(nextImage,
      edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, edgeDetection_maxDetectionsPerType,
      matching_maxDistance, matching_maxCorrespondences, scratch_CMX);
    EndBenchmark("LucasKanadeTrackerBinary update");

    ASSERT_TRUE(result == RESULT_OK);

    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch_CMX);
    transform_groundTruth[0][0] = 1.0693f;  transform_groundTruth[0][1] = 0.0008f; transform_groundTruth[0][2] = 2.2256f;
    transform_groundTruth[1][0] = 0.0010f;  transform_groundTruth[1][1] = 1.0604f; transform_groundTruth[1][2] = -4.1188f;
    transform_groundTruth[2][0] = -0.0001f; transform_groundTruth[2][1] = 0.0f;    transform_groundTruth[2][2] = 1.0f;

    //lktb.get_transformation().get_homography().Print("h");

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(lktb.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  PrintBenchmarkResults_OnlyTotals();

  //lktb.get_transformation().TransformArray(templateImage, warpedTemplateImage, scratch_CMX, 1.0f);

  //templateImage.Show("templateImage", false, false, true);
  //nextImage.Show("nextImage", false, false, true);
  //warpedTemplateImage.Show("warpedTemplateImage", false, false, true);
  //lktb.ShowTemplate(false, true);
  //cv::waitKey();

  GTEST_RETURN_HERE;
}

// TODO: If needed, test these protected methods
//GTEST_TEST(CoreTech_Vision, LucasKanadeTrackerBinary_ComputeIndexLimits)
//{
//  MemoryStack scratch_CMX(&cmxBuffer[0], CMX_BUFFER_SIZE);
//  ASSERT_TRUE(scratch_CMX.IsValid());
//
//  FixedLengthList<Point<s16> > points(5, scratch_CMX);
//  Array<s32> xStartIndexes(1, 641, scratch_CMX);
//  Array<s32> yStartIndexes(1, 481, scratch_CMX);
//
//  points.PushBack(Point<s16>(1,0));
//  points.PushBack(Point<s16>(2,4));
//  points.PushBack(Point<s16>(4,4));
//  points.PushBack(Point<s16>(4,5));
//  points.PushBack(Point<s16>(6,479));
//
//  {
//    const Result result = TemplateTracker::LucasKanadeTrackerBinary::ComputeIndexLimitsHorizontal(points, xStartIndexes);
//    ASSERT_TRUE(result == RESULT_OK);
//  }
//
//  //xStartIndexes.Print("xStartIndexes");
//
//  for(s32 x=0; x<=1; x++) {
//    ASSERT_TRUE(xStartIndexes[0][x] == 0);
//  }
//
//  ASSERT_TRUE(xStartIndexes[0][2] == 1);
//
//  for(s32 x=3; x<=4; x++) {
//    ASSERT_TRUE(xStartIndexes[0][x] == 2);
//  }
//
//  for(s32 x=5; x<=6; x++) {
//    ASSERT_TRUE(xStartIndexes[0][x] == 4);
//  }
//
//  for(s32 x=7; x<=640; x++) {
//    ASSERT_TRUE(xStartIndexes[0][x] == 5);
//  }
//
//  {
//    const Result result = TemplateTracker::LucasKanadeTrackerBinary::ComputeIndexLimitsVertical(points, yStartIndexes);
//    ASSERT_TRUE(result == RESULT_OK);
//  }
//
//  //yStartIndexes.Print("yStartIndexes");
//
//  ASSERT_TRUE(yStartIndexes[0][0] == 0);
//
//  for(s32 y=1; y<=4; y++) {
//    ASSERT_TRUE(yStartIndexes[0][y] == 1);
//  }
//
//  ASSERT_TRUE(yStartIndexes[0][5] == 3);
//
//  for(s32 y=6; y<=479; y++) {
//    ASSERT_TRUE(yStartIndexes[0][y] == 4);
//  }
//
//  ASSERT_TRUE(yStartIndexes[0][480] == 5);
//
//  GTEST_RETURN_HERE;
//}

GTEST_TEST(CoreTech_Vision, DetectBlurredEdge)
{
  const u8 grayvalueThreshold = 128;
  const s32 minComponentWidth = 3;
  const s32 maxExtrema = 500;

  MemoryStack scratch_CMX(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(scratch_CMX.IsValid());

  Array<u8> image(48, 64, scratch_CMX);

  EdgeLists edges;

  edges.xDecreasing = FixedLengthList<Point<s16> >(maxExtrema, scratch_CMX);
  edges.xIncreasing = FixedLengthList<Point<s16> >(maxExtrema, scratch_CMX);
  edges.yDecreasing = FixedLengthList<Point<s16> >(maxExtrema, scratch_CMX);
  edges.yIncreasing = FixedLengthList<Point<s16> >(maxExtrema, scratch_CMX);

  for(s32 y=0; y<24; y++) {
    for(s32 x=0; x<32; x++) {
      image[y][x] = (y)*8;
    }
  }

  for(s32 y=24; y<48; y++) {
    for(s32 x=0; x<32; x++) {
      image[y][x] = 250 - (((y)*4));
    }
  }

  for(s32 x=31; x<48; x++) {
    for(s32 y=0; y<48; y++) {
      image[y][x] = (x-31)*10;
    }
  }
  for(s32 x=48; x<64; x++) {
    for(s32 y=0; y<48; y++) {
      image[y][x] = 250 - (((x-31)*6) - (x+1)/2);
    }
  }

  //image.Print("image");

  //image.Show("image", true);

  const Result result = DetectBlurredEdges(image, grayvalueThreshold, minComponentWidth, edges);

  ASSERT_TRUE(result == RESULT_OK);

  //xDecreasing.Print("xDecreasing");
  //xIncreasing.Print("xIncreasing");
  //yDecreasing.Print("yDecreasing");
  //yIncreasing.Print("yIncreasing");

  ASSERT_TRUE(edges.xDecreasing.get_size() == 62);
  ASSERT_TRUE(edges.xIncreasing.get_size() == 48);
  ASSERT_TRUE(edges.yDecreasing.get_size() == 31);
  ASSERT_TRUE(edges.yIncreasing.get_size() == 31);

  for(s32 i=0; i<=47; i++) {
    bool valueFound = false;

    for(s32 j=0; j<62; j++) {
      if(edges.xDecreasing[j] == Point<s16>(56,i)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  for(s32 i=17; i<=30; i++) {
    bool valueFound = false;

    for(s32 j=0; j<62; j++) {
      if(edges.xDecreasing[j] == Point<s16>(31,i)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  for(s32 i=0; i<=47; i++) {
    bool valueFound = false;

    for(s32 j=0; j<48; j++) {
      if(edges.xIncreasing[j] == Point<s16>(44,i)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  for(s32 i=0; i<=30; i++) {
    bool valueFound = false;

    for(s32 j=0; j<31; j++) {
      if(edges.yDecreasing[j] == Point<s16>(i,31)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  for(s32 i=0; i<=30; i++) {
    bool valueFound = false;

    for(s32 j=0; j<31; j++) {
      if(edges.yIncreasing[j] == Point<s16>(i,16)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, IsDDROkay)
{
  printf("Starting DDR test\n");

  // This test may crash the board every other time
  for(s32 i=0; i<DDR_BUFFER_SIZE; i++) {
    ddrBuffer[i] = 0;
  }

  printf("DDR test didn't crash\n");

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, DownsampleByPowerOfTwo)
{
  MemoryStack scratch_CMX(&cmxBuffer[0], CMX_BUFFER_SIZE);

  ASSERT_TRUE(scratch_CMX.IsValid());

  ASSERT_TRUE(blockImage50_320x240_WIDTH % MEMORY_ALIGNMENT == 0);
  ASSERT_TRUE(reinterpret_cast<size_t>(&blockImage50_320x240[0]) % MEMORY_ALIGNMENT == 0);

  Array<u8> in(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, const_cast<u8*>(&blockImage50_320x240[0]), blockImage50_320x240_WIDTH*blockImage50_320x240_HEIGHT, Flags::Buffer(false,false,false));

  Array<u8> out(60, 80, scratch_CMX);
  //in.Print("in");

  const Result result = ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in, 2, out, scratch_CMX);
  ASSERT_TRUE(result == RESULT_OK);

  printf("%d %d %d %d", out[0][0], out[0][17], out[40][80-1], out[59][80-3]);

  ASSERT_TRUE(out[0][0] == 155);
  ASSERT_TRUE(out[0][17] == 157);
  ASSERT_TRUE(out[40][80-1] == 143);
  ASSERT_TRUE(out[59][80-3] == 127);

  //{
  //  Matlab matlab(false);
  //  matlab.PutArray(in, "in");
  //  matlab.PutArray(out, "out");
  //}

  GTEST_RETURN_HERE;
}

#if defined(RUN_MAIN_BIG_MEMORY_TESTS) || defined(RUN_LOW_MEMORY_IMAGE_TESTS)
bool IsBlockImage50_320x240Valid(const u8 * const imageBuffer, const bool isBigEndian)
{
  //printf("%d %d %d %d\n", imageBuffer[0], imageBuffer[1000], imageBuffer[320*120], imageBuffer[320*240-1]);

  if(isBigEndian) {
    const u8 pixel1 = ((((int*)(&blockImage50_320x240[0]))[0]) & 0xFF000000)>>24;
    const u8 pixel2 = ((((int*)(&blockImage50_320x240[0]))[1000>>2]) & 0xFF000000)>>24;
    const u8 pixel3 = ((((int*)(&blockImage50_320x240[0]))[(320*120)>>2]) & 0xFF000000)>>24;
    const u8 pixel4 = ((((int*)(&blockImage50_320x240[0]))[((320*240)>>2)-1]) & 0xFF);

    if(pixel1 != 157) return false;
    if(pixel2 != 153) return false;
    if(pixel3 != 157) return false;
    if(pixel4 != 130) return false;
  } else {
    const u8 pixel1 = imageBuffer[0];
    const u8 pixel2 = imageBuffer[1000];
    const u8 pixel3 = imageBuffer[320*120];
    const u8 pixel4 = imageBuffer[320*240-1];

    if(pixel1 != 157) return false;
    if(pixel2 != 153) return false;
    if(pixel3 != 157) return false;
    if(pixel4 != 130) return false;
  }

  return true;
} // bool IsBlockImage50_320x240Valid()
#endif // #if defined(RUN_MAIN_BIG_MEMORY_TESTS) || defined(RUN_LOW_MEMORY_IMAGE_TESTS)

s32 invalidateCacheWithGarbage(bool setddrBuffer, bool setcmxBuffer)
{
  // "Invalidate" the cache by copying garbage between the buffers

  const s32 bufferSize = MIN(DDR_BUFFER_SIZE, CMX_BUFFER_SIZE);

  s32 totalValue = 5;
  for(s32 i=0; i<DDR_BUFFER_SIZE; i++) {
    totalValue += ddrBuffer[i];
  }

  printf("Ignore these values: %d ", totalValue);

  for(s32 i=0; i<CMX_BUFFER_SIZE; i++) {
    totalValue += cmxBuffer[i];
  }

  printf("%d ", totalValue);

  if(setcmxBuffer) {
    for(s32 i=0; i<bufferSize; i++) {
      cmxBuffer[i] += ddrBuffer[i];
    }

    printf("%d ", cmxBuffer[1001]);
  }

  if(setddrBuffer) {
    for(s32 i=0; i<bufferSize; i++) {
      ddrBuffer[i] += cmxBuffer[i];
    }

    printf("%d ", ddrBuffer[10001]);
  }

  for(s32 i=0; i<bufferSize; i++) {
    totalValue += ddrBuffer[i];
  }

  printf("%d\n", totalValue);

  return totalValue;
} // void flushCache()

GTEST_TEST(CoreTech_Vision, EndianCopying)
{
  enum EndianType {ENDIAN_UNKNOWN, ENDIAN_BIG, ENDIAN_LITTLE};

  const s32 bufferSize = MIN(DDR_BUFFER_SIZE, CMX_BUFFER_SIZE);

  MemoryStack scratch_DDR(&ddrBuffer[0], DDR_BUFFER_SIZE);
  MemoryStack scratch_CMX(&cmxBuffer[0], CMX_BUFFER_SIZE);

  ASSERT_TRUE(scratch_DDR.IsValid());
  ASSERT_TRUE(scratch_CMX.IsValid());

  EndianType detectedEndian = ENDIAN_UNKNOWN;

  invalidateCacheWithGarbage(true, true);

  // Create a test pattern in CMX and DDR with 32-bit accesses
  for(s32 i=0; i<(bufferSize>>2); i++) {
    reinterpret_cast<u32*>(cmxBuffer)[i] = i;
    reinterpret_cast<u32*>(ddrBuffer)[i] = i;
  }

  printf("Checking small buffer, test 1: ");
  if(cmxBuffer[100] == 25) {
    printf("Little endian detected\n");
    if(detectedEndian != ENDIAN_UNKNOWN && detectedEndian != ENDIAN_LITTLE) {
      ASSERT_TRUE(false);
    }
    detectedEndian = ENDIAN_LITTLE;
  } else if(cmxBuffer[103] == 25) {
    printf("Big endian detected\n");
    if(detectedEndian != ENDIAN_UNKNOWN && detectedEndian != ENDIAN_BIG) {
      ASSERT_TRUE(false);
    }
    detectedEndian = ENDIAN_BIG;
  } else {
    printf("Endian mixup detected\n");
    ASSERT_TRUE(false);
  }

  printf("Checking big buffer, test 1: ");
  if(ddrBuffer[100] == 25) {
    printf("Little endian detected\n");
    if(detectedEndian != ENDIAN_UNKNOWN && detectedEndian != ENDIAN_LITTLE) {
      ASSERT_TRUE(false);
    }
    detectedEndian = ENDIAN_LITTLE;
  } else if(ddrBuffer[103] == 25) {
    printf("Big endian detected\n");
    if(detectedEndian != ENDIAN_UNKNOWN && detectedEndian != ENDIAN_BIG) {
      ASSERT_TRUE(false);
    }
    detectedEndian = ENDIAN_BIG;
  } else {
    printf("Endian mixup detected\n");
    ASSERT_TRUE(false);
  }

  invalidateCacheWithGarbage(true, true);

  // Create a test pattern in CMX and DDR with 8-bit accesses
  for(s32 i=0; i<bufferSize; i++) {
    reinterpret_cast<u8*>(cmxBuffer)[i] = i % 256;
    reinterpret_cast<u8*>(ddrBuffer)[i] = i % 256;
  }

  printf("Checking small buffer, test 2: ");
  if(cmxBuffer[100] == 100) {
    printf("Endian is correct\n");
  } else {
    printf("Endian mixup detected\n");
    ASSERT_TRUE(false);
  }

  printf("Checking big buffer, test 2: ");
  if(ddrBuffer[100] == 100) {
    printf("Endian is correct\n");
  } else {
    // This type of failure is expected on the Leon
#ifndef USING_MOVIDIUS_GCC_COMPILER
    printf("Endian mixup detected\n");
    ASSERT_TRUE(false);
#else
    printf("Endian mixup detected, but this is on the Leon\n");
#endif
  }

  invalidateCacheWithGarbage(true, true);

  for(s32 i=0; i<(bufferSize>>2); i++) {
    reinterpret_cast<u32*>(ddrBuffer)[i] = i;
  }

  invalidateCacheWithGarbage(false, true);

  for(s32 i=0; i<(bufferSize>>2); i++) {
    reinterpret_cast<u32*>(cmxBuffer)[i] = reinterpret_cast<u32*>(ddrBuffer)[i];
  }

  printf("Checking small buffer, test 3: ");
  if(detectedEndian == ENDIAN_LITTLE) {
    if(cmxBuffer[100] == 25) {
      printf("Endian is correct\n");
    } else {
      printf("Endian mixup detected\n");
      ASSERT_TRUE(false);
    }
  } else {
    if(cmxBuffer[103] == 25) {
      printf("Endian is correct\n");
    } else {
      printf("Endian mixup detected\n");
      ASSERT_TRUE(false);
    }
  }

  invalidateCacheWithGarbage(true, true);

  for(s32 i=0; i<bufferSize; i++) {
    reinterpret_cast<u8*>(cmxBuffer)[i] = i % 256;
  }

  invalidateCacheWithGarbage(true, false);

  for(s32 i=0; i<bufferSize; i++) {
    reinterpret_cast<u8*>(ddrBuffer)[i] = reinterpret_cast<u8*>(cmxBuffer)[i];
  }

  printf("Checking small buffer, test 4: ");
  if(detectedEndian == ENDIAN_LITTLE) {
    if(cmxBuffer[100] == 100) {
      printf("Endian is correct\n");
    } else {
      printf("Endian mixup detected\n");
      ASSERT_TRUE(false);
    }
  } else {
    if(cmxBuffer[103] == 100) {
      printf("Endian is correct\n");
    } else {
      // This type of failure is expected on the Leon
#ifndef USING_MOVIDIUS_GCC_COMPILER
      printf("Endian mixup detected\n");
      ASSERT_TRUE(false);
#else
      printf("Endian mixup detected, but this is on the Leon\n");
#endif
    }
  }

  for(s32 i=0; i<bufferSize; i++) {
    reinterpret_cast<u8*>(ddrBuffer)[i] = i % 256;
  }

  invalidateCacheWithGarbage(false, true);

  for(s32 i=0; i<bufferSize; i++) {
    reinterpret_cast<u8*>(cmxBuffer)[i] = reinterpret_cast<u8*>(ddrBuffer)[i];
  }

  printf("Checking small buffer, test 5: ");
  if(cmxBuffer[100] == 100) {
    printf("Endian is correct\n");
  } else {
    // This type of failure is expected on the Leon
#ifndef USING_MOVIDIUS_GCC_COMPILER
    printf("Endian mixup detected\n");
    ASSERT_TRUE(false);
#else
    printf("Endian mixup detected, but this is on the Leon\n");
#endif
  }

  // Even though the Leon is messed up, the big and small buffers should be messed up in the same way
  // On the PC, they are correct in the same way
  ASSERT_TRUE(cmxBuffer[100] == ddrBuffer[100]);
  ASSERT_TRUE(cmxBuffer[103] == ddrBuffer[103]);

#ifdef USING_MOVIDIUS_GCC_COMPILER
  ASSERT_TRUE(cmxBuffer[100] == 103);
  ASSERT_TRUE(cmxBuffer[103] == 100);
#else
  ASSERT_TRUE(cmxBuffer[100] == 100);
  ASSERT_TRUE(cmxBuffer[103] == 103);
#endif

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, ComputeDockingErrorSignalAffine)
{
  // TODO: make these the real values
  const s32 horizontalTrackingResolution = 80;
  const f32 blockMarkerWidthInMM = 50.0f;
  const f32 horizontalFocalLengthInMM = 5.0f;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  const Quadrilateral<f32> initialCorners(Point<f32>(5.0f,5.0f), Point<f32>(100.0f,100.0f), Point<f32>(50.0f,20.0f), Point<f32>(10.0f,80.0f));
  const TemplateTracker::PlanarTransformation_f32 transform(TemplateTracker::TRANSFORM_AFFINE, initialCorners, ms);

  f32 rel_x, rel_y, rel_rad;
  ASSERT_TRUE(Docking::ComputeDockingErrorSignal(transform,
    horizontalTrackingResolution, blockMarkerWidthInMM, horizontalFocalLengthInMM,
    rel_x, rel_y, rel_rad, ms) == RESULT_OK);

  //printf("%f %f %f\n", rel_x, rel_y, rel_rad);

  // TODO: manually compute the correct output
  ASSERT_TRUE(FLT_NEAR(rel_x,2.7116308f));
  ASSERT_TRUE(NEAR(rel_y,-8.1348925f, 0.1f)); // The Myriad inexact floating point mode is imprecise here
  ASSERT_TRUE(FLT_NEAR(rel_rad,-0.87467581f));

  GTEST_RETURN_HERE;
}

#ifdef BENCHMARK_AFFINE
GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_BenchmarkAffine)
  // Benchmark Affine LK
{
#ifndef RUN_TRACKER_TESTS
  ASSERT_TRUE(false);
#else // This prevents the .elf from loading
  const s32 imageHeight = 60;
  const s32 imageWidth = 80;

  const s32 numPyramidLevels = 2;

  const f32 ridgeWeight = 0.0f;

  const Rectangle<f32> templateRegion(13, 34, 22, 43);

  const s32 maxIterations = 25;
  const f32 convergenceTolerance = static_cast<f32>(1e-3);

  // TODO: add check that images were loaded correctly

  //MemoryStack scratch0(&ddrBuffer[0], 80*60*2 + 256);
  MemoryStack scratch1(&cmxBuffer[0], 600000);

  //ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratch1);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratch1);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  // Affine LK
  for(s32 i=0; i<10000; i++)
  {
    PUSH_MEMORY_STACK(scratch1);

    TemplateTracker::LucasKanadeTracker_f32 tracker(image1, templateRegion, numPyramidLevels, TemplateTracker::TRANSFORM_AFFINE, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, scratch1) == RESULT_OK);
  }

#endif // RUN_TRACKER_TESTS

  GTEST_RETURN_HERE;
}
#endif // BENCHMARK_AFFINE

GTEST_TEST(CoreTech_Vision, LucasKanadeTrackerFast)
{
#ifndef RUN_TRACKER_TESTS
  ASSERT_TRUE(false);
#else
  const s32 imageHeight = 60;
  const s32 imageWidth = 80;

  const s32 numPyramidLevels = 2;

  const f32 ridgeWeight = 0.0f;

  const Rectangle<f32> templateRegion(13, 34, 22, 43);
  const Quadrilateral<f32> templateQuad(templateRegion);

  const s32 maxIterations = 25;
  const f32 convergenceTolerance = .05f;

  // TODO: add check that images were loaded correctly

  MemoryStack scratch1(&cmxBuffer[0], 600000);

  ASSERT_TRUE(scratch1.IsValid());

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratch1);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratch1);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  ASSERT_TRUE(*image1.Pointer(0,0) == 45);
  //printf("%d %d %d %d\n", *image1.Pointer(0,0), *image1.Pointer(0,1), *image1.Pointer(0,2), *image1.Pointer(0,3));
  /*image1.Print("image1");
  image2.Print("image2");*/

  // Translation-only LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTrackerFast tracker(image1, templateRegion, numPyramidLevels, TemplateTracker::TRANSFORM_TRANSLATION, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, converged, scratch1) == RESULT_OK);
    ASSERT_TRUE(converged == true);

    const f64 time2 = GetTime();

    printf("Translation-only FAST-LK totalTime:%f initTime:%f updateTrack:%f\n", time2-time0, time1-time0, time2-time1);
    PrintBenchmarkResults_All();

    //tracker.get_transformation().Print("Translation-only Fast LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][2] = -0.334f;
    transform_groundTruth[1][2] = -0.240f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTrackerFast tracker(image1, templateRegion, numPyramidLevels, TemplateTracker::TRANSFORM_AFFINE, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, converged, scratch1) == RESULT_OK);

    ASSERT_TRUE(converged);

    const f64 time2 = GetTime();

    printf("Affine FAST-LK totalTime:%f initTime:%f updateTrack:%f\n", time2-time0, time1-time0, time2-time1);
    PrintBenchmarkResults_All();

    //tracker.get_transformation().Print("Affine Fast LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][0] = 1.005f; transform_groundTruth[0][1] = 0.027f; transform_groundTruth[0][2] = -0.315f;
    transform_groundTruth[1][0] = -0.033f; transform_groundTruth[1][1] = 0.993f; transform_groundTruth[1][2] = -0.230f;
    transform_groundTruth[2][0] = 0.0f; transform_groundTruth[2][1] = 0.0f; transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

#endif // RUN_TRACKER_TESTS

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, LucasKanadeTracker)
{
#ifndef RUN_TRACKER_TESTS
  ASSERT_TRUE(false);
#else
  const s32 imageHeight = 60;
  const s32 imageWidth = 80;

  const s32 numPyramidLevels = 2;

  const f32 ridgeWeight = 0.0f;

  const Rectangle<f32> templateRegion(13, 34, 22, 43);

  const s32 maxIterations = 25;
  const f32 convergenceTolerance = .05f;

  // TODO: add check that images were loaded correctly

  //MemoryStack scratch0(&ddrBuffer[0], 80*60*2 + 256);
  MemoryStack scratch1(&cmxBuffer[0], 600000);

  //ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratch1);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratch1);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  ASSERT_TRUE(*image1.Pointer(0,0) == 45);
  //printf("%d %d %d %d\n", *image1.Pointer(0,0), *image1.Pointer(0,1), *image1.Pointer(0,2), *image1.Pointer(0,3));
  /*image1.Print("image1");
  image2.Print("image2");*/

  // Translation-only LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_f32 tracker(image1, templateRegion, numPyramidLevels, TemplateTracker::TRANSFORM_TRANSLATION, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, false, converged, scratch1) == RESULT_OK);

    ASSERT_TRUE(converged == true);

    const f64 time2 = GetTime();

    printf("Translation-only LK totalTime:%f initTime:%f updateTrack:%f\n", time2-time0, time1-time0, time2-time1);
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Translation-only LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][2] = -0.334f;
    transform_groundTruth[1][2] = -0.240f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_f32 tracker(image1, templateRegion, numPyramidLevels, TemplateTracker::TRANSFORM_AFFINE, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, false, converged, scratch1) == RESULT_OK);
    ASSERT_TRUE(converged == true);

    const f64 time2 = GetTime();

    printf("Affine LK totalTime:%f initTime:%f updateTrack:%f\n", time2-time0, time1-time0, time2-time1);
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Affine LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][0] = 1.005f; transform_groundTruth[0][1] = 0.027f; transform_groundTruth[0][2] = -0.315f;
    transform_groundTruth[1][0] = -0.033f; transform_groundTruth[1][1] = 0.993f; transform_groundTruth[1][2] = -0.230f;
    transform_groundTruth[2][0] = 0.0f; transform_groundTruth[2][1] = 0.0f; transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Projective LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_f32 tracker(image1, templateRegion, numPyramidLevels, TemplateTracker::TRANSFORM_PROJECTIVE, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, true, converged, scratch1) == RESULT_OK);

    ASSERT_TRUE(converged == true);

    const f64 time2 = GetTime();

    printf("Projective LK totalTime:%f initTime:%f updateTrack:%f\n", time2-time0, time1-time0, time2-time1);
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Projective LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][0] = 1.008f; transform_groundTruth[0][1] = 0.027f; transform_groundTruth[0][2] = -0.360f;
    transform_groundTruth[1][0] = -0.034f; transform_groundTruth[1][1] = 0.992f; transform_groundTruth[1][2] = -0.270f;
    transform_groundTruth[2][0] = -0.001f; transform_groundTruth[2][1] = -0.001f; transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

#endif // RUN_TRACKER_TESTS

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageFiltering)
{
  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(3,16,ms);
  ASSERT_TRUE(image.IsValid());

  image[0][0] = 1; image[0][1] = 2; image[0][2] = 3;
  image[1][0] = 9; image[1][1] = 9; image[1][2] = 9;
  image[2][0] = 0; image[2][1] = 0; image[2][2] = 1;

  //image.Print("image");

  Array<u8> image2(4,16,ms);
  ASSERT_TRUE(image2.IsValid());

  image2[0][0] = 1; image2[0][1] = 2; image2[0][2] = 3;
  image2[1][0] = 9; image2[1][1] = 9; image2[1][2] = 9;
  image2[2][0] = 0; image2[2][1] = 0; image2[2][2] = 1;
  image2[3][0] = 5; image2[3][1] = 5; image2[3][2] = 5;

  //image2.Print("image2");

  Array<s32> filteredOutput(1, 16, ms);

  //
  // Test with border of 2
  //

  ScrollingIntegralImage_u8_s32 ii_border2(4, 16, 2, ms);
  ASSERT_TRUE(ii_border2.ScrollDown(image, 4, ms) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder2_0(-1, 1, -1, 1);
    const s32 imageRow_testBorder2_0 = 0;
    const s32 filteredOutput_groundTruth_testBorder2_0[] = {35, 39, 28};
    ASSERT_TRUE(ii_border2.FilterRow(filter_testBorder2_0, imageRow_testBorder2_0, filteredOutput) == RESULT_OK);

    //ii_border2.Print("ii_border2");
    //filteredOutput.Print("filteredOutput1");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder2_0[i]);
    }
  }

  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, ms) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder2_1(-1, 1, -1, 1);
    const s32 imageRow_testBorder2_1 = 2;
    const s32 filteredOutput_groundTruth_testBorder2_1[] = {27, 29, 20};
    ASSERT_TRUE(ii_border2.FilterRow(filter_testBorder2_1, imageRow_testBorder2_1, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput2");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder2_1[i]);
    }
  }

  //
  // Test with border of 1
  //

  // imBig2 = [1,1,2,3,3;1,1,2,3,3;9,9,9,9,9;0,0,0,1,1;0,0,0,1,1];

  ScrollingIntegralImage_u8_s32 ii_border1(4, 16, 1, ms);

  ASSERT_TRUE(ii_border1.ScrollDown(image, 4, ms) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder1_0(0, 0, 0, 2);
    const s32 imageRow_testBorder1_0 = 0;
    const s32 filteredOutput_groundTruth_testBorder1_0[] = {10, 11, 13};
    ASSERT_TRUE(ii_border1.FilterRow(filter_testBorder1_0, imageRow_testBorder1_0, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput3");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder1_0[i]);
    }
  }

  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, ms) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder1_1(0, 1, 0, 2);
    const s32 imageRow_testBorder1_1 = 1;
    const s32 filteredOutput_groundTruth_testBorder1_1[] = {18, 20, 11};
    ASSERT_TRUE(ii_border1.FilterRow(filter_testBorder1_1, imageRow_testBorder1_1, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput4");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder1_1[i]);
    }
  }

  //
  // Test with border of 0
  //

  ScrollingIntegralImage_u8_s32 ii_border0(3, 16, 0, ms);

  ASSERT_TRUE(ii_border0.get_rowOffset() == 0);

  ASSERT_TRUE(ii_border0.ScrollDown(image2, 3, ms) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder2_0(-1, 0, 0, 0);
    const s32 imageRow_testBorder2_0 = 1;
    const s32 filteredOutput_groundTruth_testBorder2_0[] = {0, 0, 18};
    ASSERT_TRUE(ii_border0.FilterRow(filter_testBorder2_0, imageRow_testBorder2_0, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput5");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder2_0[i]);
    }
  }

  ASSERT_TRUE(ii_border0.ScrollDown(image2, 1, ms) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder2_1(0, 1, 0, 0);
    const s32 imageRow_testBorder2_1 = 2;
    const s32 filteredOutput_groundTruth_testBorder2_1[] = {0, 1, 1};
    ASSERT_TRUE(ii_border0.FilterRow(filter_testBorder2_1, imageRow_testBorder2_1, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput5");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder2_1[i]);
    }
  }

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageGeneration)
{
  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(3,16,ms);
  ASSERT_TRUE(image.IsValid());

  image[0][0] = 1; image[0][1] = 2; image[0][2] = 3;
  image[1][0] = 9; image[1][1] = 9; image[1][2] = 9;
  image[2][0] = 0; image[2][1] = 0; image[2][2] = 1;

  Array<u8> image2(4,16,ms);
  ASSERT_TRUE(image2.IsValid());

  image2[0][0] = 1; image2[0][1] = 2; image2[0][2] = 3;
  image2[1][0] = 9; image2[1][1] = 9; image2[1][2] = 9;
  image2[2][0] = 0; image2[2][1] = 0; image2[2][2] = 1;
  image2[3][0] = 5; image2[3][1] = 5; image2[3][2] = 5;

  //
  // Test with border of 2
  //

  // imBig = [1,1,1,2,3,3,3;1,1,1,2,3,3,3;1,1,1,2,3,3,3;9,9,9,9,9,9,9;0,0,0,0,1,1,1;0,0,0,0,1,1,1;0,0,0,0,1,1,1]
  const s32 border2_groundTruthRows[7][5] = {
    {1, 2, 3, 5, 8},
    {2, 4, 6, 10, 16},
    {3, 6, 9, 15, 24},
    {12, 24, 36, 51, 69},
    {12, 24, 36, 51, 70},
    {12, 24, 36, 51, 71},
    {12, 24, 36, 51, 72}};

  ScrollingIntegralImage_u8_s32 ii_border2(3, 16, 2, ms);
  ASSERT_TRUE(ii_border2.get_rowOffset() == -2);

  //Result ScrollDown(const Array<u8> &image, s32 numRowsToScroll, MemoryStack scratch);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2)+4,'int32'), 1, 3, 2)
  ASSERT_TRUE(ii_border2.ScrollDown(image, 3, ms) == RESULT_OK);

  //ASSERT_TRUE(ii_border2.get_minRow(1) == 0);
  ASSERT_TRUE(ii_border2.get_maxRow(1) == -1);
  ASSERT_TRUE(ii_border2.get_rowOffset() == -2);

  //ii_border2.Print("ii_border2");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<5; x++) {
      ASSERT_TRUE(ii_border2[y][x] == border2_groundTruthRows[y][x]);
    }
  }

  // scrolledII = computeScrollingIntegralImage(im, initialII, 1+3-2, 2, 2)
  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, ms) == RESULT_OK);

  //ASSERT_TRUE(ii_border2.get_minRow(1) == 2);
  ASSERT_TRUE(ii_border2.get_maxRow(1) == 1);
  ASSERT_TRUE(ii_border2.get_rowOffset() == 0);

  //ii_border2.Print("ii_border2");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<5; x++) {
      ASSERT_TRUE(ii_border2[y][x] == border2_groundTruthRows[y+2][x]);
    }
  }

  // scrolledII2 = computeScrollingIntegralImage(im, scrolledII, 1+3-2+2, 2, 2)
  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, ms) == RESULT_OK);

  //ASSERT_TRUE(ii_border2.get_minRow(1) == 2);
  ASSERT_TRUE(ii_border2.get_maxRow(1) == 3);
  ASSERT_TRUE(ii_border2.get_rowOffset() == 2);

  //ii_border2.Print("ii_border2");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<5; x++) {
      ASSERT_TRUE(ii_border2[y][x] == border2_groundTruthRows[y+4][x]);
    }
  }

  //
  // Test with border of 1
  //

  // imBig2 = [1,1,2,3,3;1,1,2,3,3;9,9,9,9,9;0,0,0,1,1;0,0,0,1,1];
  const s32 border1_groundTruthRows[5][4] = {
    {1, 2, 4, 7},
    {2, 4, 8, 14},
    {11, 22, 35, 50},
    {11, 22, 35, 51},
    {11, 22, 35, 52}};

  ScrollingIntegralImage_u8_s32 ii_border1(3, 16, 1, ms);

  ASSERT_TRUE(ii_border1.get_rowOffset() == -1);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2)+2,'int32'), 1, 3, 1)
  ASSERT_TRUE(ii_border1.ScrollDown(image, 3, ms) == RESULT_OK);

  //ASSERT_TRUE(ii_border1.get_minRow(1) == 0);
  ASSERT_TRUE(ii_border1.get_maxRow(1) == 0);
  ASSERT_TRUE(ii_border1.get_rowOffset() == -1);

  //ii_border1.Print("ii_border1");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<4; x++) {
      ASSERT_TRUE(ii_border1[y][x] == border1_groundTruthRows[y][x]);
    }
  }

  // scrolledII = computeScrollingIntegralImage(im, initialII, 1+3-1, 1, 1)
  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, ms) == RESULT_OK);

  //ASSERT_TRUE(ii_border1.get_minRow(1) == 0);
  ASSERT_TRUE(ii_border1.get_maxRow(1) == 1);
  ASSERT_TRUE(ii_border1.get_rowOffset() == 0);

  //ii_border1.Print("ii_border1");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<4; x++) {
      ASSERT_TRUE(ii_border1[y][x] == border1_groundTruthRows[y+1][x]);
    }
  }

  // scrolledII2 = computeScrollingIntegralImage(im, scrolledII, 1+3-1+1, 1, 1)
  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, ms) == RESULT_OK);

  //ASSERT_TRUE(ii_border1.get_minRow(1) == 1);
  ASSERT_TRUE(ii_border1.get_maxRow(1) == 2);
  ASSERT_TRUE(ii_border1.get_rowOffset() == 1);

  //ii_border1.Print("ii_border1");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<4; x++) {
      ASSERT_TRUE(ii_border1[y][x] == border1_groundTruthRows[y+2][x]);
    }
  }

  //
  // Test with border of 0
  //

  const s32 border0_groundTruthRows[4][3] = {
    {1,  3,  6},
    {10, 21, 33},
    {10, 21, 34},
    {15, 31, 49}};

  ScrollingIntegralImage_u8_s32 ii_border0(3, 16, 0, ms);

  ASSERT_TRUE(ii_border0.get_rowOffset() == 0);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2),'int32'), 1, 3, 0)
  ASSERT_TRUE(ii_border0.ScrollDown(image2, 3, ms) == RESULT_OK);

  //ASSERT_TRUE(ii_border0.get_minRow(1) == 0);
  ASSERT_TRUE(ii_border0.get_maxRow(1) == 1);
  ASSERT_TRUE(ii_border0.get_rowOffset() == 0);

  //ii_border0.Print("ii_border0");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<3; x++) {
      ASSERT_TRUE(ii_border0[y][x] == border0_groundTruthRows[y][x]);
    }
  }

  // scrolledII = computeScrollingIntegralImage(im, initialII, 1+3, 1, 0)
  ASSERT_TRUE(ii_border0.ScrollDown(image2, 1, ms) == RESULT_OK);

  //ASSERT_TRUE(ii_border0.get_minRow(1) == 1);
  ASSERT_TRUE(ii_border0.get_maxRow(1) == 2);
  ASSERT_TRUE(ii_border0.get_rowOffset() == 1);

  //ii_border0.Print("ii_border0");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<3; x++) {
      ASSERT_TRUE(ii_border0[y][x] == border0_groundTruthRows[y+1][x]);
    }
  }

  GTEST_RETURN_HERE;
}

#ifdef RUN_MAIN_BIG_MEMORY_TESTS
GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps12345_realImage)
{
  // Check that the image loaded correctly
  ASSERT_TRUE(IsBlockImage50_320x240Valid(&blockImage50_320x240[0], imagesAreEndianSwapped));

  const CharacteristicScaleAlgorithm scaleImage_useWhichAlgorithm = CHARACTERISTIC_SCALE_MEDIUM_MEMORY; // CHARACTERISTIC_SCALE_ORIGINAL, CHARACTERISTIC_SCALE_MEDIUM_MEMORY
  const s32 scaleImage_thresholdMultiplier = 49152; // .75 * (2^16) = 49152
  const s32 scaleImage_numPyramidLevels = 4;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(blockImage50_320x240_HEIGHT,blockImage50_320x240_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(blockImage50_320x240_HEIGHT,blockImage50_320x240_WIDTH);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8

  const s32 maxExtractedQuads = 1000;
  const s32 quads_minQuadArea = 100;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const f32 decode_minContrastRatio = 1.25;

  const s32 maxMarkers = 100;

  MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
  MemoryStack scratch1(&cmxBuffer[0], CMX_BUFFER_SIZE/2);
  MemoryStack scratch2(&cmxBuffer[0] + CMX_BUFFER_SIZE/2, CMX_BUFFER_SIZE/2);

  ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());
  ASSERT_TRUE(scratch2.IsValid());

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, blockImage50_320x240_WIDTH, scratch0);

  Array<u8> image(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, scratch0);
  image.Set(blockImage50, blockImage50_320x240_WIDTH*blockImage50_320x240_HEIGHT);

  ASSERT_TRUE(IsBlockImage50_320x240Valid(image.Pointer(0,0), false);

  FixedLengthList<BlockMarker> markers(maxMarkers, scratch0);
  FixedLengthList<Array<f64> > homographies(maxMarkers, scratch0);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f64> newArray(3, 3, scratch0);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  //InitBenchmarking();

  {
    const f64 time0 = GetTime();
    const Result result = SimpleDetector_Steps12345(
      image,
      markers,
      homographies,
      scaleImage_useWhichAlgorithm,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_percentHorizontal, component_percentVertical,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      scratch1,
      scratch2);
    const f64 time1 = GetTime();

    printf("totalTime: %f\n", time1-time0);

    //PrintBenchmarkResults();

    ASSERT_TRUE(result == RESULT_OK);
  }

  //markers.Print("markers");

  ASSERT_TRUE(markers.get_size() == 1);

  ASSERT_TRUE(markers[0].blockType == 15);
  ASSERT_TRUE(markers[0].faceType == 5);
  ASSERT_TRUE(markers[0].orientation == BlockMarker::ORIENTATION_LEFT);

  if(scaleImage_useWhichAlgorithm == CHARACTERISTIC_SCALE_ORIGINAL) {
    ASSERT_TRUE(markers[0].corners[0] == Point<s16>(207,261));
    ASSERT_TRUE(markers[0].corners[1] == Point<s16>(204,335));
    ASSERT_TRUE(markers[0].corners[2] == Point<s16>(284,264));
    ASSERT_TRUE(markers[0].corners[3] == Point<s16>(279,339));
  } else if(scaleImage_useWhichAlgorithm == CHARACTERISTIC_SCALE_MEDIUM_MEMORY) {
    ASSERT_TRUE(markers[0].corners[0] == Point<s16>(209,261));
    ASSERT_TRUE(markers[0].corners[1] == Point<s16>(205,334));
    ASSERT_TRUE(markers[0].corners[2] == Point<s16>(282,265));
    ASSERT_TRUE(markers[0].corners[3] == Point<s16>(279,339));
  }

  GTEST_RETURN_HERE;
}
#endif // RUN_MAIN_BIG_MEMORY_TESTS

GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps12345_realImage_lowMemory)
{
#ifndef RUN_LOW_MEMORY_IMAGE_TESTS
  // TODO: change back to false
  //ASSERT_TRUE(false);
  ASSERT_TRUE(true);
#else

  // TODO: Check that the image loaded correctly
  //ASSERT_TRUE(IsBlockImage50_320x240Valid(&blockImage50_320x240[0], imagesAreEndianSwapped));

  const s32 scaleImage_thresholdMultiplier = 49152; // .75 * (2^16) = 49152
  const s32 scaleImage_numPyramidLevels = 3;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(blockImage50_320x240_HEIGHT,blockImage50_320x240_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(blockImage50_320x240_HEIGHT,blockImage50_320x240_WIDTH);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8

  const s32 maxExtractedQuads = 1000/2;
  const s32 quads_minQuadArea = 100/4;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const f32 decode_minContrastRatio = 1.25;

  const s32 maxMarkers = 100;
  const s32 maxConnectedComponentSegments = 25000/2;

  //MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
  MemoryStack scratch1(&cmxBuffer[0], CMX_BUFFER_SIZE/2);
  MemoryStack scratch2(&cmxBuffer[0] + CMX_BUFFER_SIZE/2, CMX_BUFFER_SIZE/2);

  //ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());
  ASSERT_TRUE(scratch2.IsValid());

  /*Array<u8> image(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, scratch0);*/
  Array<u8> image(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, scratch1);
  image.Set(blockImage50_320x240, blockImage50_320x240_WIDTH*blockImage50_320x240_HEIGHT);

  //image.Print("image", 0, 0, 0, 50);

  ASSERT_TRUE(IsBlockImage50_320x240Valid(image.Pointer(0,0), false));

  FixedLengthList<BlockMarker> markers(maxMarkers, scratch2);
  FixedLengthList<Array<f64> > homographies(maxMarkers, scratch2);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f64> newArray(3, 3, scratch2);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  InitBenchmarking();

  {
    const f64 time0 = GetTime();
    const Result result = SimpleDetector_Steps12345_lowMemory(
      image,
      markers,
      homographies,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_percentHorizontal, component_percentVertical,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      maxConnectedComponentSegments,
      maxExtractedQuads,
      scratch1,
      scratch2);
    const f64 time1 = GetTime();

    printf("totalTime: %f\n", time1-time0);

    PrintBenchmarkResults_All();

    ASSERT_TRUE(result == RESULT_OK);
  }

  markers.Print("markers");

  ASSERT_TRUE(markers.get_size() == 1);

  ASSERT_TRUE(markers[0].blockType == 15);
  ASSERT_TRUE(markers[0].faceType == 5);
  ASSERT_TRUE(markers[0].orientation == BlockMarker::ORIENTATION_LEFT);

  ASSERT_TRUE(markers[0].corners[0] == Point<s16>(105,132));
  ASSERT_TRUE(markers[0].corners[1] == Point<s16>(103,167));
  ASSERT_TRUE(markers[0].corners[2] == Point<s16>(140,133));
  ASSERT_TRUE(markers[0].corners[3] == Point<s16>(139,169));

#endif // RUN_LOW_MEMORY_IMAGE_TESTS

  GTEST_RETURN_HERE;
}

#ifdef RUN_ALL_BIG_MEMORY_TESTS
// Create a test pattern image, full of a grid of squares for probes
static Result DrawExampleProbesImage(Array<u8> &image, Quadrilateral<s16> &quad, Array<f64> &homography)
{
  for(s32 bit=0; bit<NUM_BITS_TYPE_0; bit++) {
    for(s32 probe=0; probe<NUM_PROBES_PER_BIT_TYPE_0; probe++) {
      const s32 x = static_cast<s32>(Round(static_cast<f64>(100 *probesX_type0[bit][probe]) / pow(2.0, NUM_FRACTIONAL_BITS_TYPE_0)));
      const s32 y = static_cast<s32>(Round(static_cast<f64>(100 *probesY_type0[bit][probe]) / pow(2.0, NUM_FRACTIONAL_BITS_TYPE_0)));
      image[y][x] = 10 * (bit+1);
    }
  }

  quad[0] = Point<s16>(0,0);
  quad[1] = Point<s16>(0,100);
  quad[2] = Point<s16>(100,0);
  quad[3] = Point<s16>(100,100);

  homography[0][0] = 100; homography[0][1] = 0;   homography[0][2] = 0;
  homography[1][0] = 0;   homography[1][1] = 100; homography[1][2] = 0;
  homography[2][0] = 0;   homography[2][1] = 0;   homography[2][2] = 1;

  return RESULT_OK;
}
#endif // #ifdef RUN_ALL_BIG_MEMORY_TESTS

#ifdef RUN_ALL_BIG_MEMORY_TESTS
GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps12345_fiducialImage)
{
  const CharacteristicScaleAlgorithm scaleImage_useWhichAlgorithm = CHARACTERISTIC_SCALE_ORIGINAL; // CHARACTERISTIC_SCALE_ORIGINAL, CHARACTERISTIC_SCALE_MEDIUM_MEMORY
  const s32 scaleImage_thresholdMultiplier = 49152; // .75 * (2^16) = 49152
  const s32 scaleImage_numPyramidLevels = 4;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(fiducial105_6_HEIGHT,fiducial105_6_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(fiducial105_6_HEIGHT,fiducial105_6_WIDTH);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8

  const s32 maxExtractedQuads = 1000;
  const s32 quads_minQuadArea = 100;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const f32 decode_minContrastRatio = 1.25;

  const s32 maxMarkers = 100;

  MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
  MemoryStack scratch1(&cmxBuffer[0], CMX_BUFFER_SIZE/2);
  MemoryStack scratch2(&cmxBuffer[0] + CMX_BUFFER_SIZE/2, CMX_BUFFER_SIZE/2);

  ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());
  ASSERT_TRUE(scratch2.IsValid());

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, fiducial105_6_WIDTH, scratch0);

  Array<u8> image(fiducial105_6_HEIGHT, fiducial105_6_WIDTH, scratch0);
  image.Set(fiducial105_6, fiducial105_6_WIDTH*fiducial105_6_HEIGHT);

  FixedLengthList<BlockMarker> markers(maxMarkers, scratch0);
  FixedLengthList<Array<f64> > homographies(maxMarkers, scratch0);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f64> newArray(3, 3, scratch0);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  {
    const Result result = SimpleDetector_Steps12345(
      image,
      markers,
      homographies,
      scaleImage_useWhichAlgorithm,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_percentHorizontal, component_percentVertical,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      scratch1,
      scratch2);

    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(markers.get_size() == 1);

  ASSERT_TRUE(markers[0].blockType == 105);
  ASSERT_TRUE(markers[0].faceType == 6);
  ASSERT_TRUE(markers[0].orientation == BlockMarker::ORIENTATION_LEFT);
  ASSERT_TRUE(markers[0].corners[0] == Point<s16>(21,21));
  ASSERT_TRUE(markers[0].corners[1] == Point<s16>(21,235));
  ASSERT_TRUE(markers[0].corners[2] == Point<s16>(235,21));
  ASSERT_TRUE(markers[0].corners[3] == Point<s16>(235,235));

  GTEST_RETURN_HERE;
}
#endif // #ifdef RUN_ALL_BIG_MEMORY_TESTS

#ifdef RUN_ALL_BIG_MEMORY_TESTS
// The test is if it can run without crashing
GTEST_TEST(CoreTech_Vision, FiducialMarker)
{
  const s32 imageWidth = fiducial105_6_WIDTH;
  const s32 imageHeight = fiducial105_6_HEIGHT;
  const f32 minContrastRatio = 1.25f;

  MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
  ASSERT_TRUE(scratch0.IsValid());

  FiducialMarkerParser parser = FiducialMarkerParser();

  Array<u8> image(imageHeight,imageWidth,scratch0);
  Quadrilateral<s16> quad = Quadrilateral<s16>();
  Array<f64> homography(3,3,scratch0);

  quad[0] = Point<s16>(21,21);
  quad[1] = Point<s16>(21,235);
  quad[2] = Point<s16>(235,21);
  quad[3] = Point<s16>(235,235);

  homography[0][0] = 214; homography[0][1] = 0;   homography[0][2] = 21;
  homography[1][0] = 0;   homography[1][1] = 214; homography[1][2] = 21;
  homography[2][0] = 0;   homography[2][1] = 0;   homography[2][2] = 1;

  image.Set(fiducial105_6, fiducial105_6_WIDTH*fiducial105_6_HEIGHT);

  BlockMarker marker;

  {
    const Result result = parser.ExtractBlockMarker(image, quad, homography, minContrastRatio, marker, scratch0);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(marker.blockType == 105);
  ASSERT_TRUE(marker.faceType == 6);
  ASSERT_TRUE(marker.orientation == BlockMarker::ORIENTATION_UP);

  for(s32 i=0; i<4; i++) {
    ASSERT_TRUE(marker.corners[i] == quad[i]);
  }

  GTEST_RETURN_HERE;
}
#endif // #ifdef RUN_ALL_BIG_MEMORY_TESTS

#ifdef RUN_ALL_BIG_MEMORY_TESTS
// The test is if it can run without crashing
GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps1234_realImage)
{
  const CharacteristicScaleAlgorithm scaleImage_useWhichAlgorithm = CHARACTERISTIC_SCALE_MEDIUM_MEMORY; // CHARACTERISTIC_SCALE_ORIGINAL, CHARACTERISTIC_SCALE_MEDIUM_MEMORY
  const s32 scaleImage_thresholdMultiplier = 49152; // .75 * (2^16) = 49152
  const s32 scaleImage_numPyramidLevels = 4;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(blockImage50_320x240_HEIGHT,blockImage50_320x240_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(blockImage50_320x240_HEIGHT,blockImage50_320x240_WIDTH);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8

  const s32 maxExtractedQuads = 1000;
  const s32 quads_minQuadArea = 100;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const s32 maxMarkers = 100;

  MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
  MemoryStack scratch1(&cmxBuffer[0], CMX_BUFFER_SIZE/2);
  MemoryStack scratch2(&cmxBuffer[0] + CMX_BUFFER_SIZE/2, CMX_BUFFER_SIZE/2);

  ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());
  ASSERT_TRUE(scratch2.IsValid());

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, blockImage50_320x240_WIDTH, scratch0);

  Array<u8> image(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, scratch0);
  image.Set(&blockImage50_320x240[0], blockImage50_320x240_HEIGHT*blockImage50_320x240_WIDTH);

  FixedLengthList<BlockMarker> markers(maxMarkers, scratch0);
  FixedLengthList<Array<f64> > homographies(maxMarkers, scratch0);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f64> newArray(3, 3, scratch0);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  //markers.Pointer(0)->homography.Show("markers", true, false);

  {
    const Result result = SimpleDetector_Steps1234(
      image,
      markers,
      homographies,
      scaleImage_useWhichAlgorithm,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_percentHorizontal, component_percentVertical,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      scratch1,
      scratch2);

    ASSERT_TRUE(result == RESULT_OK);
  }

  GTEST_RETURN_HERE;
}
#endif // #ifdef RUN_ALL_BIG_MEMORY_TESTS

GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents)
{
  const s32 numComponents = 60;
  const s32 minQuadArea = 100;
  const s32 quadSymmetryThreshold = 384;
  const s32 imageHeight = 480;
  const s32 imageWidth = 640;
  const s32 minDistanceFromImageEdge = 2;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  FixedLengthList<BlockMarker> markers(50, ms);

  // TODO: check these by hand
  const Quadrilateral<s16> quads_groundTruth[] = {
    Quadrilateral<s16>(Point<s16>(25,5), Point<s16>(11,5), Point<s16>(25,19), Point<s16>(11,19)) ,
    Quadrilateral<s16>(Point<s16>(130,51),  Point<s16>(101,51), Point<s16>(130,80), Point<s16>(101,80)) };

  ConnectedComponents components(numComponents, imageWidth, ms);

  // Small square
  for(s32 y=0; y<15; y++) {
    components.PushBack(ConnectedComponentSegment(10,24,y+4,1));
  }

  // Big square
  for(s32 y=0; y<30; y++) {
    components.PushBack(ConnectedComponentSegment(100,129,y+50,2));
  }

  // Skewed quad
  components.PushBack(ConnectedComponentSegment(100,300,100,3));
  for(s32 y=0; y<10; y++) {
    components.PushBack(ConnectedComponentSegment(100,110,y+100,3));
  }

  // Tiny square
  for(s32 y=0; y<5; y++) {
    components.PushBack(ConnectedComponentSegment(10,14,y,4));
  }

  FixedLengthList<Quadrilateral<s16> > extractedQuads(2, ms);

  components.SortConnectedComponentSegments();

  const Result result =  ComputeQuadrilateralsFromConnectedComponents(components, minQuadArea, quadSymmetryThreshold, minDistanceFromImageEdge, imageHeight, imageWidth, extractedQuads, ms);
  ASSERT_TRUE(result == RESULT_OK);

  // extractedQuads.Print("extractedQuads");

  for(s32 i=0; i<extractedQuads.get_size(); i++) {
    ASSERT_TRUE(extractedQuads[i] == quads_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents)

GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput)
{
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 5000);
  MemoryStack ms(&cmxBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  FixedPointArray<s32> image(1,15,2,ms);
  FixedPointArray<s32> filter(1,5,2,ms);
  FixedPointArray<s32> out(1,15,4,ms);

  for(s32 i=0; i<image.get_size(1); i++) {
    *image.Pointer(0,i) = 1 + i;
  }

  for(s32 i=0; i<filter.get_size(1); i++) {
    *filter.Pointer(0,i) = 2*(1 + i);
  }

  const s32 out_groundTruth[] = {140, 110, 110, 140, 170, 200, 230, 260, 290, 320, 350, 380, 410, 290, 200};

  const Result result = ImageProcessing::Correlate1dCircularAndSameSizeOutput(image, filter, out);
  ASSERT_TRUE(result == RESULT_OK);

  for(s32 i=0; i<out.get_size(1); i++) {
    ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput)

GTEST_TEST(CoreTech_Vision, LaplacianPeaks)
{
#define LaplacianPeaks_BOUNDARY_LENGTH 65
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 5000);

  MemoryStack ms(&cmxBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  FixedLengthList<Point<s16> > boundary(LaplacianPeaks_BOUNDARY_LENGTH, ms);

  const s32 componentsX_groundTruth[66] = {105, 105, 106, 107, 108, 109, 109, 108, 107, 106, 105, 105, 105, 105, 106, 107, 108, 109, 108, 107, 106, 105, 105, 104, 104, 104, 104, 104, 103, 103, 103, 103, 103, 102, 101, 101, 101, 101, 101, 100, 100, 100, 100, 100, 101, 102, 103, 104, 104, 104, 103, 102, 101, 100, 100, 101, 102, 102, 102, 102, 102, 103, 104, 104, 105};
  const s32 componentsY_groundTruth[66] = {200, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 203, 204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 206, 206, 205, 204, 204, 204, 204, 204, 203, 203, 203, 202, 201, 200, 201, 201, 201, 200, 200};

  //swcLeonDisableCaches();
  //swcLeonFlushDcache();
  //swcLeonFlushCaches();
  //do{ printf("a\n"); } while(swcLeonIsCacheFlushPending());

  //swcLeonDisableCaches();
  //swcLeonFlushDcache();
  //swcLeonFlushCaches();
  //do{ printf("a:%d\n", componentsX_groundTruth[0]); } while(swcLeonIsCacheFlushPending());

  for(s32 i=0; i<LaplacianPeaks_BOUNDARY_LENGTH; i++) {
    boundary.PushBack(Point<s16>(componentsX_groundTruth[i], componentsY_groundTruth[i]));
  }

  FixedLengthList<Point<s16> > peaks(4, ms);

  const Result result = ExtractLaplacianPeaks(boundary, peaks, ms);
  ASSERT_TRUE(result == RESULT_OK);

  //boundary.Print("boundary");
  //peaks.Print("peaks");

  ASSERT_TRUE(*peaks.Pointer(0) == Point<s16>(109,201));
  ASSERT_TRUE(*peaks.Pointer(1) == Point<s16>(109,205));
  ASSERT_TRUE(*peaks.Pointer(2) == Point<s16>(104,210));
  ASSERT_TRUE(*peaks.Pointer(3) == Point<s16>(100,210));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LaplacianPeaks)

GTEST_TEST(CoreTech_Vision, Correlate1d)
{
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 5000);
  MemoryStack ms(&cmxBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  {
    PUSH_MEMORY_STACK(ms);

    FixedPointArray<s32> in1(1,1,2,ms);
    FixedPointArray<s32> in2(1,4,2,ms);
    FixedPointArray<s32> out(1,4,4,ms);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {2, 4, 6, 8};

    const Result result = ImageProcessing::Correlate1d(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(ms);

    FixedPointArray<s32> in1(1,3,5,ms);
    FixedPointArray<s32> in2(1,6,1,ms);
    FixedPointArray<s32> out(1,8,3,ms);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {0, 2, 3, 5, 6, 8, 4, 1};

    const Result result = ImageProcessing::Correlate1d(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(ms);

    FixedPointArray<s32> in1(1,4,2,ms);
    FixedPointArray<s32> in2(1,4,2,ms);
    FixedPointArray<s32> out(1,7,3,ms);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {4, 11, 20, 30, 20, 11, 4};

    const Result result = ImageProcessing::Correlate1d(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(ms);

    FixedPointArray<s32> in1(1,4,1,ms);
    FixedPointArray<s32> in2(1,5,5,ms);
    FixedPointArray<s32> out(1,8,8,ms);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {32, 88, 160, 240, 320, 208, 112, 40};

    const Result result = ImageProcessing::Correlate1d(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, Correlate1d)

GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary)
{
  const s32 numComponents = 17;
  const s32 boundaryLength = 65;
  const s32 startComponentIndex = 0;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const s32 yValues[128] = {200, 200, 201, 201, 202, 203, 204, 205, 206, 207, 207, 208, 208, 209, 209, 210, 210};
  const s32 xStartValues[128] = {102, 104, 102, 108, 102, 100, 100, 104, 100, 100, 103, 100, 103, 100, 103, 100, 103};
  const s32 xEndValues[128] = { 102, 105, 105, 109, 109, 105, 105, 109, 105, 101, 104, 101, 104, 101, 104, 101, 104};

  const s32 componentsX_groundTruth[128] = {105, 105, 106, 107, 108, 109, 109, 108, 107, 106, 105, 105, 105, 105, 106, 107, 108, 109, 108, 107, 106, 105, 105, 104, 104, 104, 104, 104, 103, 103, 103, 103, 103, 102, 101, 101, 101, 101, 101, 100, 100, 100, 100, 100, 101, 102, 103, 104, 104, 104, 103, 102, 101, 100, 100, 101, 102, 102, 102, 102, 102, 103, 104, 104, 105};
  const s32 componentsY_groundTruth[128] = {200, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 203, 204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 206, 206, 205, 204, 204, 204, 204, 204, 203, 203, 203, 202, 201, 200, 201, 201, 201, 200, 200};

  for(s32 i=0; i<numComponents; i++) {
    components.PushBack(ConnectedComponentSegment(xStartValues[i],xEndValues[i],yValues[i],1));
  }

  components.SortConnectedComponentSegments();

  //#define DRAW_TraceNextExteriorBoundary
#ifdef DRAW_TraceNextExteriorBoundary
  {
    MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
    ASSERT_TRUE(scratch0.IsValid());

    Array<u8> drawnComponents(480, 640, scratch0);
    DrawComponents<u8>(drawnComponents, components, 64, 255);

    matlab.PutArray(drawnComponents, "drawnComponents");
    //drawnComponents.Show("drawnComponents", true, false);
  }
#endif // DRAW_TraceNextExteriorBoundary

  FixedLengthList<Point<s16> > extractedBoundary(boundaryLength, ms);

  {
    s32 endComponentIndex = -1;
    const Result result = TraceNextExteriorBoundary(components, startComponentIndex, extractedBoundary, endComponentIndex, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  //extractedBoundary.Print();

  for(s32 i=0; i<boundaryLength; i++) {
    ASSERT_TRUE(*extractedBoundary.Pointer(i) == Point<s16>(componentsX_groundTruth[i], componentsY_groundTruth[i]));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary)

GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes)
{
  const s32 numComponents = 10;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 10, 0, 1);
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(12, 12, 1, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(16, 1004, 2, 1);
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 4, 3, 2);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 2, 4, 3);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(4, 6, 5, 3);
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(8, 10, 6, 3);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 4, 7, 4);
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(6, 6, 8, 4);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(5, 1000, 9, 5);

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  FixedLengthList<Anki::Embedded::Rectangle<s16> > componentBoundingBoxes(numComponents, ms);
  {
    const Result result = components.ComputeComponentBoundingBoxes(componentBoundingBoxes);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(*componentBoundingBoxes.Pointer(1) == Anki::Embedded::Rectangle<s16>(0,1004,0,2));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(2) == Anki::Embedded::Rectangle<s16>(0,4,3,3));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(3) == Anki::Embedded::Rectangle<s16>(0,10,4,6));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(4) == Anki::Embedded::Rectangle<s16>(0,6,7,8));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(5) == Anki::Embedded::Rectangle<s16>(5,1000,9,9));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes)

GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids)
{
  const s32 numComponents = 10;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 10, 0, 1);
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(12, 12, 1, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(16, 1004, 2, 1);
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 4, 3, 2);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 2, 4, 3);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(4, 6, 5, 3);
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(8, 10, 6, 3);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 4, 7, 4);
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(6, 6, 8, 4);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 1000, 9, 5);

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  FixedLengthList<Point<s16> > componentCentroids(numComponents, ms);
  {
    const Result result = components.ComputeComponentCentroids(componentCentroids, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(*componentCentroids.Pointer(1) == Point<s16>(503,1));
  ASSERT_TRUE(*componentCentroids.Pointer(2) == Point<s16>(2,3));
  ASSERT_TRUE(*componentCentroids.Pointer(3) == Point<s16>(5,5));
  ASSERT_TRUE(*componentCentroids.Pointer(4) == Point<s16>(2,7));
  ASSERT_TRUE(*componentCentroids.Pointer(5) == Point<s16>(500,9));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids)

#ifdef RUN_ALL_BIG_MEMORY_TESTS
// The test is if it can run without crashing
GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123_realImage)
{
  const CharacteristicScaleAlgorithm scaleImage_useWhichAlgorithm = CHARACTERISTIC_SCALE_MEDIUM_MEMORY; // CHARACTERISTIC_SCALE_ORIGINAL, CHARACTERISTIC_SCALE_MEDIUM_MEMORY
  const s32 scaleImage_thresholdMultiplier = 49152; // .75 * (2^16) = 49152
  const s32 scaleImage_numPyramidLevels = 4;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(blockImage50_320x240_HEIGHT,blockImage50_320x240_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(blockImage50_320x240_HEIGHT,blockImage50_320x240_WIDTH);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8

  MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
  MemoryStack scratch1(&cmxBuffer[0], CMX_BUFFER_SIZE/2);
  MemoryStack scratch2(&cmxBuffer[0] + CMX_BUFFER_SIZE/2, CMX_BUFFER_SIZE/2);

  ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());
  ASSERT_TRUE(scratch2.IsValid());

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, blockImage50_320x240_WIDTH, scratch0);

  Array<u8> image(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, scratch0);
  image.Set(&blockImage50_320x240[0], blockImage50_320x240_HEIGHT*blockImage50_320x240_WIDTH);

  const Result result = SimpleDetector_Steps123(
    image,
    scaleImage_useWhichAlgorithm,
    scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
    component1d_minComponentWidth, component1d_maxSkipDistance,
    component_minimumNumPixels, component_maximumNumPixels,
    component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
    extractedComponents,
    scratch1,
    scratch2);

  ASSERT_TRUE(result == RESULT_OK);

  Array<u8> drawnComponents(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, scratch0);
  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

  //matlab.PutArray(drawnComponents, "drawnComponents0");
  //drawnComponents.Show("drawnComponents0", false, false);

  extractedComponents.InvalidateFilledCenterComponents(component_percentHorizontal, component_percentVertical, scratch1);
  drawnComponents.SetZero();
  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
  //matlab.PutArray(drawnComponents, "drawnComponents1");
  //drawnComponents.Show("drawnComponents1", true, false);

  GTEST_RETURN_HERE;
}
#endif // #ifdef RUN_ALL_BIG_MEMORY_TESTS

#ifdef RUN_ALL_BIG_MEMORY_TESTS
// The test is if it can run without crashing
GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123)
{
  const s32 imageWidth = 640;
  const s32 imageHeight = 480;

  const CharacteristicScaleAlgorithm scaleImage_useWhichAlgorithm = CHARACTERISTIC_SCALE_MEDIUM_MEMORY; // CHARACTERISTIC_SCALE_ORIGINAL, CHARACTERISTIC_SCALE_MEDIUM_MEMORY
  const s32 scaleImage_thresholdMultiplier = 49152; // .75 * (2^16) = 49152
  const s32 scaleImage_numPyramidLevels = 4;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(imageHeight,imageWidth);
  const f32 maxSideLength = 0.97f*MIN(imageHeight,imageWidth);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
  MemoryStack scratch1(&cmxBuffer[0], CMX_BUFFER_SIZE/2);
  MemoryStack scratch2(&cmxBuffer[0] + CMX_BUFFER_SIZE/2, CMX_BUFFER_SIZE/2);

  ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());
  ASSERT_TRUE(scratch2.IsValid());

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, imageWidth, scratch0);

  Array<u8> image(imageHeight, imageWidth, scratch0);

  {
    Result result = DrawExampleSquaresImage(image);
    ASSERT_TRUE(result == RESULT_OK);
  }

  const Result result = SimpleDetector_Steps123(
    image,
    scaleImage_useWhichAlgorithm,
    scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
    component1d_minComponentWidth, component1d_maxSkipDistance,
    component_minimumNumPixels, component_maximumNumPixels,
    component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
    extractedComponents,
    scratch1,
    scratch2);

  ASSERT_TRUE(result == RESULT_OK);

  Array<u8> drawnComponents(imageHeight, imageWidth, scratch0);
  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

  //matlab.PutArray(drawnComponents, "drawnComponents");
  //drawnComponents.Show("drawnComponents", true, false);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123)
#endif // #ifdef RUN_ALL_BIG_MEMORY_TESTS

GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents)
{
  const s32 numComponents = 10;
  const s32 sparseMultiplyThreshold = 10 << 5;
  const s32 solidMultiplyThreshold = 2 << 5;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 10, 0, 1); // Ok
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(0, 10, 3, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(0, 10, 5, 2); // Too solid
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 10, 6, 2);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 10, 8, 2);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(0, 10, 10, 3); // Too sparse
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(0, 10, 100, 3);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 0, 105, 4); // Ok
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(0, 0, 108, 4);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 10, 110, 5); // Too solid

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  {
    const Result result = components.InvalidateSolidOrSparseComponents(sparseMultiplyThreshold, solidMultiplyThreshold, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(components.Pointer(0)->id == 1);
  ASSERT_TRUE(components.Pointer(1)->id == 1);
  ASSERT_TRUE(components.Pointer(2)->id == 0);
  ASSERT_TRUE(components.Pointer(3)->id == 0);
  ASSERT_TRUE(components.Pointer(4)->id == 0);
  ASSERT_TRUE(components.Pointer(5)->id == 0);
  ASSERT_TRUE(components.Pointer(6)->id == 0);
  ASSERT_TRUE(components.Pointer(7)->id == 4);
  ASSERT_TRUE(components.Pointer(8)->id == 4);
  ASSERT_TRUE(components.Pointer(9)->id == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents)

GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents)
{
  const s32 numComponents = 10;
  const s32 minimumNumPixels = 6;
  const s32 maximumNumPixels = 1000;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 10, 0, 1);
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(12, 12, 1, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(16, 1004, 2, 1);
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 4, 3, 2);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 2, 4, 3);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(4, 6, 5, 3);
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(8, 10, 6, 3);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 4, 7, 4);
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(6, 6, 8, 4);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 1000, 9, 5);

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  {
    const Result result = components.InvalidateSmallOrLargeComponents(minimumNumPixels, maximumNumPixels, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(components.Pointer(0)->id == 0);
  ASSERT_TRUE(components.Pointer(1)->id == 0);
  ASSERT_TRUE(components.Pointer(2)->id == 0);
  ASSERT_TRUE(components.Pointer(3)->id == 0);
  ASSERT_TRUE(components.Pointer(4)->id == 3);
  ASSERT_TRUE(components.Pointer(5)->id == 3);
  ASSERT_TRUE(components.Pointer(6)->id == 3);
  ASSERT_TRUE(components.Pointer(7)->id == 4);
  ASSERT_TRUE(components.Pointer(8)->id == 4);
  ASSERT_TRUE(components.Pointer(9)->id == 0);

  {
    const Result result = components.CompressConnectedComponentSegmentIds(ms);
    ASSERT_TRUE(result == RESULT_OK);

    const s32 maximumId = components.get_maximumId();
    ASSERT_TRUE(maximumId == 2);
  }

  ASSERT_TRUE(components.Pointer(0)->id == 0);
  ASSERT_TRUE(components.Pointer(1)->id == 0);
  ASSERT_TRUE(components.Pointer(2)->id == 0);
  ASSERT_TRUE(components.Pointer(3)->id == 0);
  ASSERT_TRUE(components.Pointer(4)->id == 1);
  ASSERT_TRUE(components.Pointer(5)->id == 1);
  ASSERT_TRUE(components.Pointer(6)->id == 1);
  ASSERT_TRUE(components.Pointer(7)->id == 2);
  ASSERT_TRUE(components.Pointer(8)->id == 2);
  ASSERT_TRUE(components.Pointer(9)->id == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents)

GTEST_TEST(CoreTech_Vision, CompressComponentIds)
{
  const s32 numComponents = 10;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 0, 0, 5);  // 3
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(0, 0, 0, 10); // 4
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(0, 0, 0, 0);  // 0
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 0, 0, 101);// 6
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(0, 0, 0, 4);  // 2
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(0, 0, 0, 11); // 5
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 0, 0, 5);  // 3

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  {
    const Result result = components.CompressConnectedComponentSegmentIds(ms);
    ASSERT_TRUE(result == RESULT_OK);

    const s32 maximumId = components.get_maximumId();
    ASSERT_TRUE(maximumId == 6);
  }

  ASSERT_TRUE(components.Pointer(0)->id == 3);
  ASSERT_TRUE(components.Pointer(1)->id == 4);
  ASSERT_TRUE(components.Pointer(2)->id == 0);
  ASSERT_TRUE(components.Pointer(3)->id == 6);
  ASSERT_TRUE(components.Pointer(4)->id == 1);
  ASSERT_TRUE(components.Pointer(5)->id == 2);
  ASSERT_TRUE(components.Pointer(6)->id == 5);
  ASSERT_TRUE(components.Pointer(7)->id == 1);
  ASSERT_TRUE(components.Pointer(8)->id == 1);
  ASSERT_TRUE(components.Pointer(9)->id == 3);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, CompressComponentIds)

// Not really a test, but computes the size of a list of ComponentSegments, to ensure that c++ isn't adding junk
GTEST_TEST(CoreTech_Vision, ComponentsSize)
{
  const s32 numComponents = 500;
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 10000);

  MemoryStack ms(&cmxBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  const s32 usedBytes0 = ms.get_usedBytes();

#ifdef PRINTF_SIZE_RESULTS
  printf("Original size: %d\n", usedBytes0);
#endif

  FixedLengthList<ConnectedComponentSegment> segmentList(numComponents, ms);

  const s32 usedBytes1 = ms.get_usedBytes();
  const double actualSizePlusOverhead = double(usedBytes1 - usedBytes0) / double(numComponents);

#ifdef PRINTF_SIZE_RESULTS
  printf("Final size: %d\n"
    "Difference: %d\n"
    "Expected size of a components: %d\n"
    "Actual size (includes overhead): %f\n",
    usedBytes1,
    usedBytes1 - usedBytes0,
    sizeof(ConnectedComponentSegment),
    actualSizePlusOverhead);
#endif // #ifdef PRINTF_SIZE_RESULTS

  const double difference = actualSizePlusOverhead - double(sizeof(ConnectedComponentSegment));
  ASSERT_TRUE(difference > -0.0001 && difference < 1.0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComponentsSize)

GTEST_TEST(CoreTech_Vision, SortComponents)
{
  const s32 numComponents = 10;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(50, 100, 50, u16_MAX); // 2
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(s16_MAX, s16_MAX, s16_MAX, 0); // 9
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(s16_MAX, s16_MAX, 0, 0); // 7
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(s16_MAX, s16_MAX, s16_MAX, u16_MAX); // 4
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, s16_MAX, 0, 0); // 5
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(0, s16_MAX, s16_MAX, 0); // 8
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(0, s16_MAX, s16_MAX, u16_MAX); // 3
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(s16_MAX, s16_MAX, 0, u16_MAX); // 1
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(0, s16_MAX, 0, 0); // 6
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(42, 42, 42, 42); // 0

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  const Result result = components.SortConnectedComponentSegments();
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(*components.Pointer(0) == component9);
  ASSERT_TRUE(*components.Pointer(1) == component7);
  ASSERT_TRUE(*components.Pointer(2) == component0);
  ASSERT_TRUE(*components.Pointer(3) == component6);
  ASSERT_TRUE(*components.Pointer(4) == component3);
  ASSERT_TRUE(*components.Pointer(5) == component4);
  ASSERT_TRUE(*components.Pointer(6) == component8);
  ASSERT_TRUE(*components.Pointer(7) == component2);
  ASSERT_TRUE(*components.Pointer(8) == component5);
  ASSERT_TRUE(*components.Pointer(9) == component1);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, SortComponents)

GTEST_TEST(CoreTech_Vision, SortComponentsById)
{
  const s32 numComponents = 10;

  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(1, 1, 1, 3); // 6
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(2, 2, 2, 1); // 0
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(3, 3, 3, 1); // 1
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(4, 4, 4, 0); // X
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(5, 5, 5, 1); // 2
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(6, 6, 6, 1); // 3
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(7, 7, 7, 1); // 4
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(8, 8, 8, 4); // 7
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(9, 9, 9, 5); // 8
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 0, 0, 1); // 5

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  const Result result = components.SortConnectedComponentSegmentsById(ms);
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(components.get_size() == 9);

  ASSERT_TRUE(*components.Pointer(0) == component1);
  ASSERT_TRUE(*components.Pointer(1) == component2);
  ASSERT_TRUE(*components.Pointer(2) == component4);
  ASSERT_TRUE(*components.Pointer(3) == component5);
  ASSERT_TRUE(*components.Pointer(4) == component6);
  ASSERT_TRUE(*components.Pointer(5) == component9);
  ASSERT_TRUE(*components.Pointer(6) == component0);
  ASSERT_TRUE(*components.Pointer(7) == component7);
  ASSERT_TRUE(*components.Pointer(8) == component8);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, SortComponents)

GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d)
{
  const s32 imageWidth = 18;
  const s32 imageHeight = 5;

  const s32 minComponentWidth = 2;
  const s32 maxSkipDistance = 0;
  const s32 maxComponentSegments = 100;

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&cmxBuffer[0], CMX_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

#define ApproximateConnectedComponents2d_binaryImageDataLength (18*5)
  const s32 binaryImageData[] = {
    0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0};

  const s32 numComponents_groundTruth = 13;

  //#define UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
#define SORTED_BY_ID_GROUND_TRUTH_ApproximateConnectedComponents2d
#ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
  const s32 xStart_groundTruth[] = {4,  3, 10, 13, 5, 9,  13, 6, 12, 16, 7, 11, 14};
  const s32 xEnd_groundTruth[]   = {11, 5, 11, 15, 6, 11, 14, 9, 14, 17, 8, 12, 16};
  const s32 y_groundTruth[]      = {0,  1, 1,  1,  2, 2,  2,  3, 3,  3,  4, 4,  4};
  const s32 id_groundTruth[]     = {1,  1, 1,  2,  1, 1,  2,  1, 2,  2,  1, 2,  2};
#else // #ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
#ifdef SORTED_BY_ID_GROUND_TRUTH_ApproximateConnectedComponents2d
  const s32 xStart_groundTruth[] = {4,  3, 10, 5, 9,  6, 7, 13, 13, 12, 16, 11, 14};
  const s32 xEnd_groundTruth[]   = {11, 5, 11, 6, 11, 9, 8, 15, 14, 14, 17, 12, 16};
  const s32 y_groundTruth[]      = {0,  1, 1,  2, 2,  3, 4, 1,  2,  3,  3,  4,  4};
  const s32 id_groundTruth[]     = {1,  1, 1,  1, 1,  1, 1, 2,  2,  2,  2,  2,  2};
#else // Sorted by id, y, and xStart
  const s32 xStart_groundTruth[] = {4,  3, 10, 5, 9,  6, 7, 13, 13, 12, 16, 11, 14};
  const s32 xEnd_groundTruth[]   = {11, 5, 11, 6, 11, 9, 8, 15, 14, 14, 17, 12, 16};
  const s32 y_groundTruth[]      = {0,  1, 1,  2, 2,  3, 4, 1,  2,  3,  3,  4,  4};
  const s32 id_groundTruth[]     = {1,  1, 1,  1, 1,  1, 1, 2,  2,  2,  2,  2,  2};
#endif // #ifdef SORTED_BY_ID_GROUND_TRUTH_ApproximateConnectedComponents2d
#endif // #ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d ... #else

  Array<u8> binaryImage(imageHeight, imageWidth, ms);
  ASSERT_TRUE(binaryImage.IsValid());

  //binaryImage.Print("binaryImage");

  ASSERT_TRUE(binaryImage.SetCast<s32>(binaryImageData, ApproximateConnectedComponents2d_binaryImageDataLength) == imageWidth*imageHeight);

  //binaryImage.Print("binaryImage");

  //FixedLengthList<ConnectedComponentSegment> extractedComponents(maxComponentSegments, ms);
  ConnectedComponents components(maxComponentSegments, imageWidth, ms);
  ASSERT_TRUE(components.IsValid());

  const Result result = components.Extract2dComponents_FullImage(binaryImage, minComponentWidth, maxSkipDistance);
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(components.SortConnectedComponentSegmentsById(ms) == RESULT_OK);

  ASSERT_TRUE(components.get_size() == 13);

  //components.Print();

  for(s32 i=0; i<numComponents_groundTruth; i++) {
    ASSERT_TRUE(components.Pointer(i)->xStart == xStart_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->xEnd == xEnd_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->y == y_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->id == id_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d)

GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d)
{
  const s32 imageWidth = 50;
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 5000);

  const s32 minComponentWidth = 3;
  const s32 maxComponents = 10;
  const s32 maxSkipDistance = 1;

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&cmxBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  u8 * binaryImageRow = reinterpret_cast<u8*>(ms.Allocate(imageWidth));
  memset(binaryImageRow, 0, imageWidth);

  FixedLengthList<ConnectedComponentSegment> extractedComponentSegments(maxComponents, ms);

  for(s32 i=10; i<=15; i++) binaryImageRow[i] = 1;
  for(s32 i=25; i<=35; i++) binaryImageRow[i] = 1;
  for(s32 i=38; i<=38; i++) binaryImageRow[i] = 1;
  for(s32 i=43; i<=45; i++) binaryImageRow[i] = 1;
  for(s32 i=47; i<=49; i++) binaryImageRow[i] = 1;

  const Result result = ConnectedComponents::Extract1dComponents(binaryImageRow, imageWidth, minComponentWidth, maxSkipDistance, extractedComponentSegments);

  ASSERT_TRUE(result == RESULT_OK);

  //extractedComponentSegments.Print("extractedComponentSegments");

  ASSERT_TRUE(extractedComponentSegments.get_size() == 3);

  ASSERT_TRUE(extractedComponentSegments.Pointer(0)->xStart == 10 && extractedComponentSegments.Pointer(0)->xEnd == 15);
  ASSERT_TRUE(extractedComponentSegments.Pointer(1)->xStart == 25 && extractedComponentSegments.Pointer(1)->xEnd == 35);
  ASSERT_TRUE(extractedComponentSegments.Pointer(2)->xStart == 43 && extractedComponentSegments.Pointer(2)->xEnd == 49);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d)

GTEST_TEST(CoreTech_Vision, BinomialFilter)
{
  const s32 imageWidth = 10;
  const s32 imageHeight = 5;
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 5000);

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&cmxBuffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(imageHeight, imageWidth, ms);
  image.SetZero();

  Array<u8> imageFiltered(imageHeight, imageWidth, ms);

  ASSERT_TRUE(image.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imageFiltered.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<imageWidth; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  const Result result = ImageProcessing::BinomialFilter<u8,u32,u8>(image, imageFiltered, ms);

  //printf("image:\n");
  //image.Print();

  //printf("imageFiltered:\n");
  //imageFiltered.Print();

  ASSERT_TRUE(result == RESULT_OK);

  const s32 correctResults[16][16] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 1, 1, 1, 2, 2, 2, 3}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  for(s32 y=0; y<imageHeight; y++) {
    for(s32 x=0; x<imageWidth; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageFiltered.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageFiltered.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, BinomialFilter)

GTEST_TEST(CoreTech_Vision, DownsampleByFactor)
{
  const s32 imageWidth = 10;
  const s32 imageHeight = 4;

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 1000);

  MemoryStack ms(&cmxBuffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(imageHeight, imageWidth, ms);
  Array<u8> imageDownsampled(imageHeight/2, imageWidth/2, ms);

  ASSERT_TRUE(image.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imageDownsampled.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<imageWidth; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  Result result = ImageProcessing::DownsampleByTwo<u8,u32,u8>(image, imageDownsampled);

  ASSERT_TRUE(result == RESULT_OK);

  const s32 correctResults[2][5] = {{0, 0, 0, 0, 0}, {0, 1, 2, 3, 4}};

  for(s32 y=0; y<imageDownsampled.get_size(0); y++) {
    for(s32 x=0; x<imageDownsampled.get_size(1); x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageDownsampled.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DownsampleByFactor)

GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale)
{
  const s32 imageWidth = 16;
  const s32 imageHeight = 16;
  const s32 numPyramidLevels = 3;

#define ComputeCharacteristicScale_imageDataLength (16*16)
  const u8 imageData[ComputeCharacteristicScale_imageDataLength] = {
    0, 0, 0, 107, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 160, 89,
    0, 255, 0, 251, 255, 0, 0, 255, 0, 255, 255, 255, 255, 0, 197, 38,
    0, 0, 0, 77, 255, 0, 0, 255, 0, 255, 255, 255, 255, 0, 238, 149,
    18, 34, 27, 179, 255, 255, 255, 255, 0, 255, 255, 255, 255, 0, 248, 67,
    226, 220, 173, 170, 40, 210, 108, 255, 0, 255, 255, 255, 255, 0, 49, 11,
    25, 100, 51, 137, 218, 251, 24, 255, 0, 0, 0, 0, 0, 0, 35, 193,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 49, 162, 65, 133, 178, 62,
    255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 188, 241, 156, 253, 24, 113,
    255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 62, 53, 148, 56, 134, 175,
    255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 234, 181, 138, 27, 135, 92,
    255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 69, 60, 222, 28, 220, 188,
    255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 195, 30, 68, 16, 124, 101,
    255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 48, 155, 81, 103, 100, 174,
    255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 73, 115, 30, 114, 171, 180,
    255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 23, 117, 240, 93, 189, 113,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 147, 169, 165, 195, 133, 5};

  const u32 correctResults[16 + 16][16 + 16] = {
    {983040, 2097152, 4390912, 8585216, 12124160, 12976128, 12386304, 10158080, 6488064, 4587520, 4849664, 4849664, 4259840, 4784128, 6225920, 6422528},
    {1638400, 4063232, 6029312, 8847360, 9580544, 10285056, 9109504, 9519104, 8896512, 8667136, 10747904, 10747904, 7880704, 7344128, 6959104, 7012352},
    {2293760, 4947968, 6814720, 7451648, 8088576, 8725504, 9028608, 8997888, 8967168, 10043392, 14680064, 14680064, 11599872, 8269824, 7995392, 7471104},
    {4587520, 6164480, 7009280, 7478272, 9654272, 10133504, 8670208, 8709120, 8748032, 10444800, 14680064, 14680064, 9486336, 8364032, 7331840, 6488064},
    {7536640, 7712768, 8425472, 9195520, 10022912, 10346496, 10166272, 9998336, 8528896, 8637440, 10084352, 9650176, 8568832, 6094848, 5373952, 5570560},
    {9699328, 8581120, 8605696, 8863744, 9355264, 9543680, 11403264, 9338880, 8309760, 8192000, 8060928, 7602176, 6356992, 5308416, 5439488, 6160384},
    {11468800, 8769536, 7925760, 9109504, 9764864, 9764864, 9306112, 9109504, 7880704, 8159232, 8222720, 7536640, 7077888, 7393280, 7073792, 7012352},
    {12255232, 8323072, 5242880, 6555648, 6420480, 6285312, 6422528, 6832128, 7241728, 9699328, 9961472, 8458240, 8716288, 8192000, 7426048, 7536640},
    {11796480, 7241728, 1966080, 3457024, 2965504, 2945024, 3395584, 1966080, 6602752, 9371648, 10420224, 8630272, 8097792, 7743488, 7794688, 8257536},
    {11468800, 6520832, 3735552, 2019328, 1372160, 1335296, 1908736, 3289088, 5963776, 9043968, 10223616, 8482816, 7983104, 7208960, 7979008, 8847360},
    {11468800, 6160384, 3244032, 1437696, 741376, 696320, 1302528, 2756608, 5795840, 8912896, 9633792, 8015872, 7565312, 6750208, 7979008, 9109504},
    {11468800, 6205440, 3280896, 1470464, 774144, 724992, 1323008, 2744320, 6098944, 8585216, 8978432, 7766016, 5963776, 6291456, 8036352, 9109504},
    {11468800, 6656000, 3846144, 2117632, 1470464, 1421312, 1970176, 3252224, 6402048, 8126464, 8454144, 6815744, 5963776, 6553600, 8151040, 9502720},
    {11796480, 5898240, 1966080, 3846144, 3280896, 3231744, 3698688, 1966080, 6365184, 7270400, 7620608, 7274496, 7974912, 7974912, 8847360, 9437184},
    {13107200, 9793536, 5898240, 6656000, 6205440, 6156288, 6508544, 7217152, 8282112, 8904704, 9084928, 9056256, 8818688, 9306112, 8781824, 7667712},
    {15073280, 13107200, 11796480, 11468800, 11468800, 11468800, 11468800, 11796480, 12517376, 12255232, 10813440, 10158080, 10551296, 10158080, 7929856, 5046272}};

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 10000);

  MemoryStack ms(&cmxBuffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(imageHeight, imageWidth, ms);
  ASSERT_TRUE(image.IsValid());
  ASSERT_TRUE(image.Set(imageData, ComputeCharacteristicScale_imageDataLength) == imageWidth*imageHeight);

  FixedPointArray<u32> scaleImage(imageHeight, imageWidth, 16, ms);
  ASSERT_TRUE(scaleImage.IsValid());

  ASSERT_TRUE(ComputeCharacteristicScaleImage(image, numPyramidLevels, scaleImage, ms) == RESULT_OK);

  // TODO: manually compute results, and check
  //scaleImage.Print();

  for(s32 y=0; y<imageHeight; y++) {
    for(s32 x=0; x<imageWidth; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *scaleImage.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale)

#if ANKICORETECH_EMBEDDED_USE_MATLAB && defined(RUN_MATLAB_IMAGE_TEST)
GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale2)
{
  const s32 imageWidth = 640;
  const s32 imageHeight = 480;
  const s32 numPyramidLevels = 6;

  /*const s32 imageWidth = 320;
  const s32 imageHeight = 240;
  const s32 numPyramidLevels = 5;*/

  MemoryStack scratch0(&ddrBuffer[0], DDR_BUFFER_SIZE);
  ASSERT_TRUE(scratch0.IsValid());

  Matlab matlab(false);

  matlab.EvalStringEcho("image = rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png'));");
  //matlab.EvalStringEcho("image = imresize(rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png')), [240,320]);");
  Array<u8> image = matlab.GetArray<u8>("image");
  ASSERT_TRUE(image.get_rawDataPointer() != NULL);

  Array<u32> scaleImage(imageHeight, imageWidth, scratch0);
  ASSERT_TRUE(scaleImage.get_rawDataPointer() != NULL);

  ASSERT_TRUE(ComputeCharacteristicScaleImage(image, numPyramidLevels, scaleImage, scratch0) == RESULT_OK);

  matlab.PutArray<u32>(scaleImage, "scaleImage6_c");

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale2)
#endif // #if ANKICORETECH_EMBEDDED_USE_MATLAB

GTEST_TEST(CoreTech_Vision, TraceInteriorBoundary)
{
  const s32 imageWidth = 16;
  const s32 imageHeight = 16;

#define TraceInteriorBoundary_imageDataLength (16*16)
  const u8 imageData[TraceInteriorBoundary_imageDataLength] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  const s32 numPoints = 9;
  Point<s16> groundTruth[16];

  groundTruth[0] = Point<s16>(8,6);
  groundTruth[1] = Point<s16>(8,5);
  groundTruth[2] = Point<s16>(7,4);
  groundTruth[3] = Point<s16>(7,3);
  groundTruth[4] = Point<s16>(8,3);
  groundTruth[5] = Point<s16>(9,3);
  groundTruth[6] = Point<s16>(9,4);
  groundTruth[7] = Point<s16>(9,5);
  groundTruth[8] = Point<s16>(9,6);

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(CMX_BUFFER_SIZE, 10000);

  MemoryStack ms(&cmxBuffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> binaryImage(imageHeight, imageWidth, ms);
  const Point<s16> startPoint(8,6);
  const BoundaryDirection initialDirection = BOUNDARY_N;
  FixedLengthList<Point<s16> > boundary(MAX_BOUNDARY_LENGTH, ms);

  ASSERT_TRUE(binaryImage.IsValid());
  ASSERT_TRUE(boundary.IsValid());

  binaryImage.Set(imageData, TraceInteriorBoundary_imageDataLength);

  ASSERT_TRUE(TraceInteriorBoundary(binaryImage, startPoint, initialDirection, boundary) == RESULT_OK);

  ASSERT_TRUE(boundary.get_size() == numPoints);
  for(s32 iPoint=0; iPoint<numPoints; iPoint++) {
    //printf("%d) (%d,%d) and (%d,%d)\n", iPoint, boundary.Pointer(iPoint)->x, boundary.Pointer(iPoint)->y, groundTruth[iPoint].x, groundTruth[iPoint].y);
    ASSERT_TRUE(*boundary.Pointer(iPoint) == groundTruth[iPoint]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, TraceInteriorBoundary)

#if !ANKICORETECH_EMBEDDED_USE_GTEST
int RUN_ALL_TESTS()
{
  s32 numPassedTests = 0;
  s32 numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTrackerBinary);
  //CALL_GTEST_TEST(CoreTech_Vision, ComputeIndexLimits);
  CALL_GTEST_TEST(CoreTech_Vision, DetectBlurredEdge);

#ifdef BENCHMARK_AFFINE
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_BenchmarkAffine);
  return 0;
#endif

  CALL_GTEST_TEST(CoreTech_Vision, IsDDROkay);

  CALL_GTEST_TEST(CoreTech_Vision, DownsampleByPowerOfTwo);
  CALL_GTEST_TEST(CoreTech_Vision, EndianCopying);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeDockingErrorSignalAffine);

  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTrackerFast);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker);

  CALL_GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageFiltering);
  CALL_GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageGeneration);

#ifdef RUN_LOW_MEMORY_IMAGE_TESTS
  CALL_GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps12345_realImage_lowMemory);
#endif

#ifdef RUN_MAIN_BIG_MEMORY_TESTS
  CALL_GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps12345_realImage);
#endif

#ifdef RUN_ALL_BIG_MEMORY_TESTS
  CALL_GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps12345_fiducialImage);
  CALL_GTEST_TEST(CoreTech_Vision, FiducialMarker);
  CALL_GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps1234_realImage);
  CALL_GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123_realImage);
  CALL_GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123);
#endif

#ifdef RUN_LITTLE_TESTS
  CALL_GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents);
  CALL_GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput);
  CALL_GTEST_TEST(CoreTech_Vision, LaplacianPeaks);
  CALL_GTEST_TEST(CoreTech_Vision, Correlate1d);
  CALL_GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids);

  CALL_GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents);
  CALL_GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents);
  CALL_GTEST_TEST(CoreTech_Vision, CompressComponentIds);
  CALL_GTEST_TEST(CoreTech_Vision, ComponentsSize);
  CALL_GTEST_TEST(CoreTech_Vision, SortComponents);
  CALL_GTEST_TEST(CoreTech_Vision, SortComponentsById);
  CALL_GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d);
  CALL_GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d);
  CALL_GTEST_TEST(CoreTech_Vision, BinomialFilter);
  CALL_GTEST_TEST(CoreTech_Vision, DownsampleByFactor);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale);
  CALL_GTEST_TEST(CoreTech_Vision, TraceInteriorBoundary);
#endif // #ifdef RUN_LITTLE_TESTS

  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);

  return numFailedTests;
} // int RUN_ALL_TESTS()
#endif // #if !ANKICORETECH_EMBEDDED_USE_GTEST
