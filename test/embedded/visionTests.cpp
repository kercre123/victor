//#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedVision.h"

using namespace Anki::Embedded;

// #define RUN_MATLAB_IMAGE_TEST

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
Matlab matlab(false);
#endif

#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
#include "gtest/gtest.h"
#endif

//#define BUFFER_IN_DDR_WITH_L2
#define BUFFER_IN_CMX

#if defined(BUFFER_IN_DDR_WITH_L2) && defined(BUFFER_IN_CMX)
You cannot use both CMX and L2 Cache;
#endif

#define MAX_BYTES 10000

#ifdef _MSC_VER
static char buffer[MAX_BYTES];
#else

#ifdef BUFFER_IN_CMX
static char buffer[MAX_BYTES] __attribute__((section(".cmx.bss,CMX")));
#else // #ifdef BUFFER_IN_CMX

#ifdef BUFFER_IN_DDR_WITH_L2
static char buffer[MAX_BYTES] __attribute__((section(".ddr.bss,DDR"))); // With L2 cache
#else
static char buffer[MAX_BYTES] __attribute__((section(".ddr_direct.bss,DDR_DIRECT"))); // No L2 cache
#endif

#endif // #ifdef BUFFER_IN_CMX ... #else

#endif // #ifdef USING_MOVIDIUS_COMPILER

#include "blockImage50.h"

//IN_DDR GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput)
//{
//  const s32 numBytes = MIN(MAX_BYTES, 5000);
//  MemoryStack ms(&buffer[0], numBytes);
//  ASSERT_TRUE(ms.IsValid());
//
//  FixedPointArray<s32> image(1,15,2,ms);
//  FixedPointArray<s32> filter(1,5,2,ms);
//  FixedPointArray<s32> out(1,15,4,ms);
//
//  for(s32 i=0; i<image.get_size(1); i++) {
//    *image.Pointer(0,i) = 1 + i;
//  }
//
//  for(s32 i=0; i<filter.get_size(1); i++) {
//    *filter.Pointer(0,i) = 2*(1 + i);
//  }
//
//  const s32 out_groundTruth[] = {140, 110, 110, 140, 170, 200, 230, 260, 290, 320, 350, 380, 410, 290, 200};
//
//  const Result result = Correlate1dCircularAndSameSizeOutput(image, filter, out);
//  ASSERT_TRUE(result == RESULT_OK);
//
//  out.Print();
//
//  for(s32 i=0; i<out.get_size(1); i++) {
//    ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
//  }
//
//  GTEST_RETURN_HERE;
//} // GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput)

IN_DDR GTEST_TEST(CoreTech_Vision, LaplacianPeaks)
{
  const s32 boundaryLength = 65;
  const s32 numBytes = MIN(MAX_BYTES, 5000);

  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  FixedLengthList<Point<s16>> boundary(boundaryLength, ms);

  const s16 componentsX_groundTruth[] = {105, 105, 106, 107, 108, 109, 109, 108, 107, 106, 105, 105, 105, 105, 106, 107, 108, 109, 108, 107, 106, 105, 105, 104, 104, 104, 104, 104, 103, 103, 103, 103, 103, 102, 101, 101, 101, 101, 101, 100, 100, 100, 100, 100, 101, 102, 103, 104, 104, 104, 103, 102, 101, 100, 100, 101, 102, 102, 102, 102, 102, 103, 104, 104, 105};
  const s16 componentsY_groundTruth[] = {200, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 203, 204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 206, 206, 205, 204, 204, 204, 204, 204, 203, 203, 203, 202, 201, 200, 201, 201, 201, 200, 200};

  for(s32 i=0; i<boundaryLength; i++) {
    boundary.PushBack(Point<s16>(componentsX_groundTruth[i], componentsY_groundTruth[i]));
  }

  FixedLengthList<Point<s16>> peaks(4, ms);

  const Result result = ExtractLaplacianPeaks(boundary, peaks, ms);
  ASSERT_TRUE(result == RESULT_OK);

  peaks.Print();

  ASSERT_TRUE(*peaks.Pointer(0) == Point<s16>(109,201));
  ASSERT_TRUE(*peaks.Pointer(1) == Point<s16>(109,205));
  ASSERT_TRUE(*peaks.Pointer(2) == Point<s16>(104,210));
  ASSERT_TRUE(*peaks.Pointer(3) == Point<s16>(100,210));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LaplacianPeaks)

IN_DDR GTEST_TEST(CoreTech_Vision, Correlate1d)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  MemoryStack ms(&buffer[0], numBytes);
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

    const Result result = Correlate1d(in1, in2, out);
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

    const Result result = Correlate1d(in1, in2, out);
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

    const Result result = Correlate1d(in1, in2, out);
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

    const Result result = Correlate1d(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, Correlate1d)

IN_DDR GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary)
{
  const s32 numComponents = 17;
  const s32 boundaryLength = 65;
  const s32 startComponentIndex = 0;
  const s32 numBytes = MIN(MAX_BYTES, 5000);

  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, ms);

  const s16 yValues[] = {200, 200, 201, 201, 202, 203, 204, 205, 206, 207, 207, 208, 208, 209, 209, 210, 210};
  const s16 xStartValues[] = {102, 104, 102, 108, 102, 100, 100, 104, 100, 100, 103, 100, 103, 100, 103, 100, 103};
  const s16 xEndValues[] = { 102, 105, 105, 109, 109, 105, 105, 109, 105, 101, 104, 101, 104, 101, 104, 101, 104};

  const s16 componentsX_groundTruth[] = {105, 105, 106, 107, 108, 109, 109, 108, 107, 106, 105, 105, 105, 105, 106, 107, 108, 109, 108, 107, 106, 105, 105, 104, 104, 104, 104, 104, 103, 103, 103, 103, 103, 102, 101, 101, 101, 101, 101, 100, 100, 100, 100, 100, 101, 102, 103, 104, 104, 104, 103, 102, 101, 100, 100, 101, 102, 102, 102, 102, 102, 103, 104, 104, 105};
  const s16 componentsY_groundTruth[] = {200, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 203, 204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 206, 206, 205, 204, 204, 204, 204, 204, 203, 203, 203, 202, 201, 200, 201, 201, 201, 200, 200};

  for(s32 i=0; i<numComponents; i++) {
    components.PushBack(ConnectedComponentSegment(xStartValues[i],xEndValues[i],yValues[i],1));
  }

  //#define DRAW_TraceNextExteriorBoundary
#ifdef DRAW_TraceNextExteriorBoundary
  {
    const u32 numBytes0 = 10000000;
    MemoryStack scratch0(calloc(numBytes0,1), numBytes0);
    ASSERT_TRUE(scratch0.IsValid());
    Array<u8> drawnComponents(480, 640, scratch0);
    DrawComponents<u8>(drawnComponents, components, 64, 255);

    matlab.PutArray(drawnComponents, "drawnComponents");
    drawnComponents.Show("drawnComponents", true, false);

    free(scratch0.get_buffer());
  }
#endif // DRAW_TraceNextExteriorBoundary

  FixedLengthList<Point<s16>> extractedBoundary(boundaryLength, ms);

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
} // IN_DDR GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary)

IN_DDR GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes)
{
  const s32 numComponents = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);

  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, ms);

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

  FixedLengthList<Anki::Embedded::Rectangle<s16>> componentBoundingBoxes(numComponents, ms);
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
} // IN_DDR GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes)

IN_DDR GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids)
{
  const s32 numComponents = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);

  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, ms);

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

  FixedLengthList<Point<s16>> componentCentroids(numComponents, ms);
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
} // IN_DDR GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids)

// The test is if it can run without crashing
IN_DDR GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123_realImage)
{
  const s32 scaleImage_numPyramidLevels = 6;

  const s16 component1d_minComponentWidth = 0;
  const s16 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(blockImage50_HEIGHT,blockImage50_WIDTH);
  const f32 maxSideLength = 0.9f*MIN(blockImage50_HEIGHT,blockImage50_WIDTH);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8

  const u32 numBytes0 = 10000000;
  MemoryStack scratch0(calloc(numBytes0,1), numBytes0);
  ASSERT_TRUE(scratch0.IsValid());

  const u32 numBytes1 = 10000000;
  MemoryStack scratch1(calloc(numBytes1,1), numBytes0);
  ASSERT_TRUE(scratch1.IsValid());

  const u32 numBytes2 = 10000000;
  MemoryStack scratch2(calloc(numBytes2,1), numBytes2);
  ASSERT_TRUE(scratch2.IsValid());

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, scratch0);

  Array<u8> image(blockImage50_HEIGHT, blockImage50_WIDTH, scratch0);
  image.Set(&blockImage50[0], blockImage50_HEIGHT*blockImage50_WIDTH);

  const Result result = SimpleDetector_Steps123(
    image,
    scaleImage_numPyramidLevels,
    component1d_minComponentWidth, component1d_maxSkipDistance,
    component_minimumNumPixels, component_maximumNumPixels,
    component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
    extractedComponents,
    scratch1,
    scratch2);

  ASSERT_TRUE(result == RESULT_OK);

  Array<u8> drawnComponents(blockImage50_HEIGHT, blockImage50_WIDTH, scratch0);
  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

  matlab.PutArray(drawnComponents, "drawnComponents0");
  drawnComponents.Show("drawnComponents0", false, false);

  extractedComponents.InvalidateFilledCenterComponents(component_percentHorizontal, component_percentVertical, scratch1);
  drawnComponents.SetZero();
  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
  matlab.PutArray(drawnComponents, "drawnComponents1");
  drawnComponents.Show("drawnComponents1", true, false);

  free(scratch0.get_buffer());
  free(scratch1.get_buffer());
  free(scratch2.get_buffer());

  GTEST_RETURN_HERE;
}

// The test is if it can run without crashing
IN_DDR GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123)
{
  const s32 width = 640;
  const s32 height = 480;

  const s32 scaleImage_numPyramidLevels = 6;

  const s16 component1d_minComponentWidth = 0;
  const s16 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(height,width);
  const f32 maxSideLength = 0.9f*MIN(height,width);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const u32 numBytes0 = 10000000;
  MemoryStack scratch0(calloc(numBytes0,1), numBytes0);
  ASSERT_TRUE(scratch0.IsValid());

  const u32 numBytes1 = 10000000;
  MemoryStack scratch1(calloc(numBytes1,1), numBytes0);
  ASSERT_TRUE(scratch1.IsValid());

  const u32 numBytes2 = 10000000;
  MemoryStack scratch2(calloc(numBytes2,1), numBytes2);
  ASSERT_TRUE(scratch2.IsValid());

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, scratch0);

  Array<u8> image(height, width, scratch0);

  {
    Result result = DrawExampleSquaresImage(image);
    ASSERT_TRUE(result == RESULT_OK);
  }

  const Result result = SimpleDetector_Steps123(
    image,
    scaleImage_numPyramidLevels,
    component1d_minComponentWidth, component1d_maxSkipDistance,
    component_minimumNumPixels, component_maximumNumPixels,
    component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
    extractedComponents,
    scratch1,
    scratch2);

  ASSERT_TRUE(result == RESULT_OK);

  Array<u8> drawnComponents(height, width, scratch0);
  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

  matlab.PutArray(drawnComponents, "drawnComponents");
  //drawnComponents.Show("drawnComponents", true, false);

  free(scratch0.get_buffer());
  free(scratch1.get_buffer());
  free(scratch2.get_buffer());

  GTEST_RETURN_HERE;
} // IN_DDR GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123)

IN_DDR GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents)
{
  const s32 numComponents = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  const s32 sparseMultiplyThreshold = 10 << 5;
  const s32 solidMultiplyThreshold = 2 << 5;

  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, ms);

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
} // IN_DDR GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents)

IN_DDR GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents)
{
  const s32 numComponents = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  const s32 minimumNumPixels = 6;
  const s32 maximumNumPixels = 1000;

  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, ms);

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

    const u16 maximumId = components.get_maximumId();
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
} // IN_DDR GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents)

IN_DDR GTEST_TEST(CoreTech_Vision, CompressComponentIds)
{
  const s32 numComponents = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);

  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, ms);

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

    const u16 maximumId = components.get_maximumId();
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
} // IN_DDR GTEST_TEST(CoreTech_Vision, CompressComponentIds)

// Not really a test, but computes the size of a list of ComponentSegments, to ensure that c++ isn't adding junk
IN_DDR GTEST_TEST(CoreTech_Vision, ComponentsSize)
{
  const s32 numComponents = 500;
  const s32 numBytes = MIN(MAX_BYTES, 10000);

  MemoryStack ms(&buffer[0], numBytes);
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
} // IN_DDR GTEST_TEST(CoreTech_Vision, ComponentsSize)

IN_DDR GTEST_TEST(CoreTech_Vision, SortComponents)
{
  const s32 numComponents = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);

  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, ms);

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
} // IN_DDR GTEST_TEST(CoreTech_Vision, SortComponents)

IN_DDR GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d)
{
  const s32 width = 18;
  const s32 height = 5;
  const s32 numBytes = MIN(MAX_BYTES, 10000);

  const s16 minComponentWidth = 2;
  const s16 maxSkipDistance = 0;
  const u16 maxComponentSegments = 100;

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

#define ApproximateConnectedComponents2d_binaryImageDataLength (18*5)
  const u8 binaryImageData[ApproximateConnectedComponents2d_binaryImageDataLength] = {
    0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0};

  const u16 numComponents_groundTruth = 13;

  //#define UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
#ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
  const s16 xStart_groundTruth[] = {4, 3, 10, 13, 5, 9, 13, 6, 12, 16, 7, 11, 14};
  const s16 xEnd_groundTruth[]   = {11, 5, 11, 15, 6, 11, 14, 9, 14, 17, 8, 12, 16};
  const s16 y_groundTruth[]      = {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4};
  const u16 id_groundTruth[]     = {1, 1, 1, 2, 1, 1, 2, 1, 2, 2, 1, 2, 2};
#else // #ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
  const s16 xStart_groundTruth[] = {4, 3, 10, 5, 9, 6, 7, 13, 13, 12, 16, 11, 14};
  const s16 xEnd_groundTruth[]   = {11, 5, 11, 6, 11, 9, 8, 15, 14, 14, 17, 12, 16};
  const s16 y_groundTruth[]      = {0, 1, 1, 2, 2, 3, 4, 1, 2, 3, 3, 4, 4};
  const u16 id_groundTruth[]     = {1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
#endif // #ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d ... #else

  Array<u8> binaryImage(height, width, ms);
  ASSERT_TRUE(binaryImage.IsValid());
  ASSERT_TRUE(binaryImage.Set(binaryImageData, ApproximateConnectedComponents2d_binaryImageDataLength) == width*height);

  //FixedLengthList<ConnectedComponentSegment> extractedComponents(maxComponentSegments, ms);
  ConnectedComponents components(maxComponentSegments, ms);
  ASSERT_TRUE(components.IsValid());

  const Result result = components.Extract2dComponents(binaryImage, minComponentWidth, maxSkipDistance, ms);
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(components.get_size() == 13);

  // extractedComponents.Print();

  for(u16 i=0; i<numComponents_groundTruth; i++) {
    ASSERT_TRUE(components.Pointer(i)->xStart == xStart_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->xEnd == xEnd_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->y == y_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->id == id_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // IN_DDR GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d)

IN_DDR GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d)
{
  const s32 width = 50;
  const s32 numBytes = MIN(MAX_BYTES, 5000);

  const s16 minComponentWidth = 3;
  const s32 maxComponents = 10;
  const s16 maxSkipDistance = 1;

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&buffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  u8 * binaryImageRow = reinterpret_cast<u8*>(ms.Allocate(width));
  memset(binaryImageRow, 0, width);

  FixedLengthList<ConnectedComponentSegment> extractedComponentSegments(maxComponents, ms);

  for(s32 i=10; i<=15; i++) binaryImageRow[i] = 1;
  for(s32 i=25; i<=35; i++) binaryImageRow[i] = 1;
  for(s32 i=38; i<=38; i++) binaryImageRow[i] = 1;
  for(s32 i=43; i<=45; i++) binaryImageRow[i] = 1;
  for(s32 i=47; i<=49; i++) binaryImageRow[i] = 1;

  const Result result = ConnectedComponents::Extract1dComponents(binaryImageRow, width, minComponentWidth, maxSkipDistance, extractedComponentSegments);

  ASSERT_TRUE(result == RESULT_OK);

  //extractedComponentSegments.Print("extractedComponentSegments");

  ASSERT_TRUE(extractedComponentSegments.get_size() == 3);

  ASSERT_TRUE(extractedComponentSegments.Pointer(0)->xStart == 10 && extractedComponentSegments.Pointer(0)->xEnd == 15);
  ASSERT_TRUE(extractedComponentSegments.Pointer(1)->xStart == 25 && extractedComponentSegments.Pointer(1)->xEnd == 35);
  ASSERT_TRUE(extractedComponentSegments.Pointer(2)->xStart == 43 && extractedComponentSegments.Pointer(2)->xEnd == 49);

  GTEST_RETURN_HERE;
} // IN_DDR GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d)

IN_DDR GTEST_TEST(CoreTech_Vision, BinomialFilter)
{
  const s32 width = 10;
  const s32 height = 5;
  const s32 numBytes = MIN(MAX_BYTES, 5000);

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&buffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(height, width, ms, false);
  image.Set(static_cast<u8>(0));

  Array<u8> imageFiltered(height, width, ms);

  ASSERT_TRUE(image.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imageFiltered.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<width; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  const Result result = BinomialFilter(image, imageFiltered, ms);

  //printf("image:\n");
  //image.Print();

  //printf("imageFiltered:\n");
  //imageFiltered.Print();

  ASSERT_TRUE(result == RESULT_OK);

  const u8 correctResults[5][10] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 1, 1, 1, 2, 2, 2, 3}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  for(s32 y=0; y<height; y++) {
    for(s32 x=0; x<width; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageFiltered.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageFiltered.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
} // IN_DDR GTEST_TEST(CoreTech_Vision, BinomialFilter)

IN_DDR GTEST_TEST(CoreTech_Vision, DownsampleByFactor)
{
  const s32 width = 10;
  const s32 height = 4;
  const s32 downsampleFactor = 2;

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 1000);

  MemoryStack ms(&buffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(height, width, ms);
  Array<u8> imageDownsampled(height/downsampleFactor, width/downsampleFactor, ms);

  ASSERT_TRUE(image.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imageDownsampled.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<width; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  Result result = DownsampleByFactor(image, downsampleFactor, imageDownsampled);

  ASSERT_TRUE(result == RESULT_OK);

  const u8 correctResults[2][5] = {{0, 0, 0, 0, 0}, {0, 1, 2, 3, 4}};

  for(s32 y=0; y<imageDownsampled.get_size(0); y++) {
    for(s32 x=0; x<imageDownsampled.get_size(1); x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageDownsampled.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
} // IN_DDR GTEST_TEST(CoreTech_Vision, DownsampleByFactor)

IN_DDR GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale)
{
  const s32 width = 16;
  const s32 height = 16;
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

  const u32 correctResults[16][16] = {
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
  const s32 numBytes = MIN(MAX_BYTES, 10000);

  MemoryStack ms(&buffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(height, width, ms);
  ASSERT_TRUE(image.IsValid());
  ASSERT_TRUE(image.Set(imageData, ComputeCharacteristicScale_imageDataLength) == width*height);

  FixedPointArray<u32> scaleImage(height, width, 16, ms);
  ASSERT_TRUE(scaleImage.IsValid());

  ASSERT_TRUE(ComputeCharacteristicScaleImage(image, numPyramidLevels, scaleImage, ms) == RESULT_OK);

  // TODO: manually compute results, and check
  //scaleImage.Print();

  for(s32 y=0; y<height; y++) {
    for(s32 x=0; x<width; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *scaleImage.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
} // IN_DDR GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale)

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB) && defined(RUN_MATLAB_IMAGE_TEST)
IN_DDR GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale2)
{
  const s32 width = 640;
  const s32 height = 480;
  const s32 numPyramidLevels = 6;

  /*const s32 width = 320;
  const s32 height = 240;
  const s32 numPyramidLevels = 5;*/

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = 10000000;
  void *buffer = calloc(numBytes, 1);
  //ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);

  ASSERT_TRUE(ms.IsValid());

  Matlab matlab(false);

  matlab.EvalStringEcho("image = rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png'));");
  //matlab.EvalStringEcho("image = imresize(rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png')), [240,320]);");
  Array<u8> image = matlab.GetArray<u8>("image");
  ASSERT_TRUE(image.get_rawDataPointer() != NULL);

  Array<u32> scaleImage(height, width, ms);
  ASSERT_TRUE(scaleImage.get_rawDataPointer() != NULL);

  ASSERT_TRUE(ComputeCharacteristicScaleImage(image, numPyramidLevels, scaleImage, ms) == RESULT_OK);

  matlab.PutArray<u32>(scaleImage, "scaleImage6_c");

  free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
} // IN_DDR GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale2)
#endif // #if defined(ANKICORETECHEMBEDDED_USE_MATLAB)

IN_DDR GTEST_TEST(CoreTech_Vision, TraceInteriorBoundary)
{
  const s32 width = 16;
  const s32 height = 16;

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
  const Point<s16> groundTruth[9] = {Point<s16>(8,6), Point<s16>(8,5), Point<s16>(7,4), Point<s16>(7,3), Point<s16>(8,3), Point<s16>(9,3), Point<s16>(9,4), Point<s16>(9,5), Point<s16>(9,6)};

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 10000);

  MemoryStack ms(&buffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> binaryImage(height, width, ms);
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
} // IN_DDR GTEST_TEST(CoreTech_Vision, TraceInteriorBoundary)

#if !defined(ANKICORETECHEMBEDDED_USE_GTEST)
IN_DDR void RUN_ALL_TESTS()
{
  s32 numPassedTests = 0;
  s32 numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary);
  CALL_GTEST_TEST(CoreTech_Vision, SimpleDetector_Steps123_realImage);
  CALL_GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents);
  CALL_GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents);
  CALL_GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d);
  CALL_GTEST_TEST(CoreTech_Vision, SortComponents);
  CALL_GTEST_TEST(CoreTech_Vision, CompressComponentIds);
  CALL_GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d);
  CALL_GTEST_TEST(CoreTech_Vision, BinomialFilter);
  CALL_GTEST_TEST(CoreTech_Vision, DownsampleByFactor);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale);
  CALL_GTEST_TEST(CoreTech_Vision, TraceInteriorBoundary);

  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);
} // void RUN_ALL_TESTS()
#endif // #if !defined(ANKICORETECHEMBEDDED_USE_GTEST)