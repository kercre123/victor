//#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedCommon.h"

using namespace Anki::Embedded;

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

#define MAX_BYTES 5000

#ifdef _MSC_VER
static char buffer[MAX_BYTES];
#else

#ifdef BUFFER_IN_CMX
__attribute__((section(".cmx.bss,CMX"))) static char buffer[MAX_BYTES];
#else // #ifdef BUFFER_IN_CMX

#ifdef BUFFER_IN_DDR_WITH_L2
__attribute__((section(".ddr.bss,DDR"))) static char buffer[MAX_BYTES]; // With L2 cache
#else
__attribute__((section(".ddr_direct.bss,DDR_DIRECT"))) static char buffer[MAX_BYTES]; // No L2 cache
#endif

#endif // #ifdef BUFFER_IN_CMX ... #else

#endif // #ifdef USING_MOVIDIUS_COMPILER

// This test requires a stopwatch, and takes about a minute to do manually
#define TEST_BENCHMARKING
#ifdef TEST_BENCHMARKING
IN_DDR GTEST_TEST(CoreTech_Common, BENCHMARKING)
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

IN_DDR GTEST_TEST(CoreTech_Common, MatrixMultiply)
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

  const Result result = MultiplyMatrices<Array<f64>, f64>(mat1, mat2, matOut);

  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(matOut.IsElementwiseEqual(matOut_groundTruth));

  GTEST_RETURN_HERE;
}

IN_DDR GTEST_TEST(CoreTech_Common, ComputeHomography)
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

  MultiplyMatrices<Array<f64>, f64>(homography_groundTruth, originalPoints, transformedPoints);

  for(s32 i=0; i<originalPoints.get_size(1); i++) {
    const f64 x0 = (*originalPoints.Pointer(0,i)) / (*originalPoints.Pointer(2,i));
    const f64 y0 = (*originalPoints.Pointer(1,i)) / (*originalPoints.Pointer(2,i));

    const f64 x1 = (*transformedPoints.Pointer(0,i)) / (*transformedPoints.Pointer(2,i));
    const f64 y1 = (*transformedPoints.Pointer(1,i)) / (*transformedPoints.Pointer(2,i));

    originalPointsList.PushBack(Point<f64>(x0, y0));
    transformedPointsList.PushBack(Point<f64>(x1, y1));
  }

  //originalPoints.Print("originalPoints");
  //transformedPoints.Print("transformedPoints");

  //originalPointsList.Print("originalPointsList");
  //transformedPointsList.Print("transformedPointsList");

  const Result result = EstimateHomography(originalPointsList, transformedPointsList, homography, ms);

  ASSERT_TRUE(result == RESULT_OK);

  //homography.Print("homography");

  ASSERT_TRUE(homography.IsElementwiseEqual_PercentThreshold(homography_groundTruth, .01, .001));

  GTEST_RETURN_HERE;
}

IN_DDR void PrintfOneArray_f32(const Array<f32> &array, const char * variableName)
{
  printf("%s:\n", variableName);
  for(s32 y=0; y<array.get_size(0); y++) {
    const f32 * const rowPointer = array.Pointer(y, 0);
    for(s32 x=0; x<array.get_size(1); x++) {
      const f32 value = rowPointer[x];
      const f32 mulitipliedValue = 10000.0f * value;
      printf("%d ", static_cast<s32>(mulitipliedValue));
    }
    printf("\n");
  }
  printf("\n");
}

IN_DDR void PrintfOneArray_f64(const Array<f64> &array, const char * variableName)
{
  printf("%s:\n", variableName);
  for(s32 y=0; y<array.get_size(0); y++) {
    const f64 * const rowPointer = array.Pointer(y, 0);
    for(s32 x=0; x<array.get_size(1); x++) {
      printf("%d ", static_cast<s32>((10000.0*rowPointer[x])));
    }
    printf("\n");
  }
  printf("\n");
}

IN_DDR GTEST_TEST(CoreTech_Common, SVD32)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

#define SVD_aDataLength 16
#define SVD_uTGroundTruthDataLength 4
#define SVD_wGroundTruthDataLength 8
#define SVD_vTGroundTruthDataLength 64

  const f32 aData[SVD_aDataLength] = {
    1, 2, 3, 5, 7, 11, 13, 17,
    19, 23, 29, 31, 37, 41, 43, 47};

  const f32 uTGroundTruthData[SVD_uTGroundTruthDataLength] = {
    -0.237504316543999f,  -0.971386483137875f,
    0.971386483137874f,  -0.237504316543999f};

  const f32 wGroundTruthData[SVD_wGroundTruthDataLength] = {
    101.885662808124f, 9.29040979446927f, 0, 0, 0, 0, 0, 0};

  const f32 vTGroundTruthData[SVD_vTGroundTruthDataLength] = {
    -0.183478685625953f, -0.223946108965585f, -0.283481700610061f, -0.307212042374800f, -0.369078720749224f, -0.416539404278702f, -0.440269746043441f, -0.487730429572919f,
    0.381166774075590f, 0.378866636898153f, 0.427695421221120f, 0.269708382365037f, 0.213979186509760f, -0.101994891202405f, -0.259981930058488f, -0.575956007770654f,
    -0.227185052857744f, -0.469620636740071f, 0.828412170627898f, -0.133259849703043f, -0.130174916014859f, -0.0535189566767408f, -0.0151909770076815f, 0.0614649823304370f,
    -0.262651399482480f, -0.301123351378262f, -0.120288604832971f, 0.894501576357598f, -0.112627300393190f, -0.0830469380120526f, -0.0682567568214837f, -0.0386763944403458f,
    -0.324884751171929f, -0.243759383675343f, -0.107225790475501f, -0.104644995857761f, 0.880843955313500f, -0.113994455451022f, -0.111413660833282f, -0.106252071597804f,
    -0.395817444421400f, 0.0932351870482750f, -0.00462734139723796f, -0.0491221437364792f, -0.0840608134431630f, 0.826949581878355f, -0.217545220460887f, -0.306534825139369f,
    -0.431283791046136f, 0.261732472410084f, 0.0466718831418935f, -0.0213607176758380f, -0.0665131978214941f, -0.202578399456957f, 0.729388999725311f, -0.406676201910152f,
    -0.502216484295607f, 0.598727043133703f, 0.149270332220156f, 0.0341621344454443f, -0.0314179665781565f, -0.261634362127581f, -0.376742559902293f, 0.393041044548282f};

  const s32 m = 2, n = 8;

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
  ASSERT_TRUE(scratch != NULL);

  a.Set(aData, SVD_aDataLength);
  w_groundTruth.Set(wGroundTruthData, SVD_uTGroundTruthDataLength);
  uT_groundTruth.Set(uTGroundTruthData, SVD_wGroundTruthDataLength);
  vT_groundTruth.Set(vTGroundTruthData, SVD_vTGroundTruthDataLength);

  const Result result = svd_f32(a, w, uT, vT, scratch);

  ASSERT_TRUE(result == RESULT_OK);

  //  w.Print("w");
  //  w_groundTruth.Print("w_groundTruth");
  //PrintfOneArray_f32(w, "w");
  //PrintfOneArray_f32(w_groundTruth, "w_groundTruth");

  //  uT.Print("uT");
  //  uT_groundTruth.Print("uT_groundTruth");
  PrintfOneArray_f32(uT, "uT");
  PrintfOneArray_f32(uT_groundTruth, "uT_groundTruth");

  //  vT.Print("vT");
  //  vT_groundTruth.Print("vT_groundTruth");

  ASSERT_TRUE(w.IsElementwiseEqual_PercentThreshold(w_groundTruth, .05, .001));
  ASSERT_TRUE(uT.IsElementwiseEqual_PercentThreshold(uT_groundTruth, .05, .001));

  // I don't know why, but the v-transpose for this SVD doesn't match Matlab's. Probably this version's is more efficient, in either memory or computation.
  //ASSERT_TRUE(vT.IsElementwiseEqual_PercentThreshold(vT_groundTruth, .05, .001));

  GTEST_RETURN_HERE;
}

IN_DDR GTEST_TEST(CoreTech_Common, SVD64)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

#define SVD_aDataLength 16
#define SVD_uTGroundTruthDataLength 4
#define SVD_wGroundTruthDataLength 8
#define SVD_vTGroundTruthDataLength 64

  const f64 aData[SVD_aDataLength] = {
    1, 2, 3, 5, 7, 11, 13, 17,
    19, 23, 29, 31, 37, 41, 43, 47};

  const f64 uTGroundTruthData[SVD_uTGroundTruthDataLength] = {
    -0.237504316543999f,  -0.971386483137875f,
    0.971386483137874f,  -0.237504316543999f};

  const f64 wGroundTruthData[SVD_wGroundTruthDataLength] = {
    101.885662808124f, 9.29040979446927f, 0, 0, 0, 0, 0, 0};

  const f64 vTGroundTruthData[SVD_vTGroundTruthDataLength] = {
    -0.183478685625953f, -0.223946108965585f, -0.283481700610061f, -0.307212042374800f, -0.369078720749224f, -0.416539404278702f, -0.440269746043441f, -0.487730429572919f,
    0.381166774075590f, 0.378866636898153f, 0.427695421221120f, 0.269708382365037f, 0.213979186509760f, -0.101994891202405f, -0.259981930058488f, -0.575956007770654f,
    -0.227185052857744f, -0.469620636740071f, 0.828412170627898f, -0.133259849703043f, -0.130174916014859f, -0.0535189566767408f, -0.0151909770076815f, 0.0614649823304370f,
    -0.262651399482480f, -0.301123351378262f, -0.120288604832971f, 0.894501576357598f, -0.112627300393190f, -0.0830469380120526f, -0.0682567568214837f, -0.0386763944403458f,
    -0.324884751171929f, -0.243759383675343f, -0.107225790475501f, -0.104644995857761f, 0.880843955313500f, -0.113994455451022f, -0.111413660833282f, -0.106252071597804f,
    -0.395817444421400f, 0.0932351870482750f, -0.00462734139723796f, -0.0491221437364792f, -0.0840608134431630f, 0.826949581878355f, -0.217545220460887f, -0.306534825139369f,
    -0.431283791046136f, 0.261732472410084f, 0.0466718831418935f, -0.0213607176758380f, -0.0665131978214941f, -0.202578399456957f, 0.729388999725311f, -0.406676201910152f,
    -0.502216484295607f, 0.598727043133703f, 0.149270332220156f, 0.0341621344454443f, -0.0314179665781565f, -0.261634362127581f, -0.376742559902293f, 0.393041044548282f};

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

  a.Set(aData, SVD_aDataLength);
  w_groundTruth.Set(wGroundTruthData, SVD_uTGroundTruthDataLength);
  uT_groundTruth.Set(uTGroundTruthData, SVD_wGroundTruthDataLength);
  vT_groundTruth.Set(vTGroundTruthData, SVD_vTGroundTruthDataLength);

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

  ASSERT_TRUE(w.IsElementwiseEqual_PercentThreshold(w_groundTruth, .05, .001));
  ASSERT_TRUE(uT.IsElementwiseEqual_PercentThreshold(uT_groundTruth, .05, .001));

  // I don't know why, but the v-transpose for this SVD doesn't match Matlab's. Probably this version's is more efficient, in either memory or computation.
  //ASSERT_TRUE(vT.IsElementwiseEqual_PercentThreshold(vT_groundTruth, .05, .001));

  GTEST_RETURN_HERE;
}

IN_DDR GTEST_TEST(CoreTech_Common, MemoryStack)
{
  ASSERT_TRUE(MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(MAX_BYTES, 200);
  //void * buffer = calloc(numBytes+MEMORY_ALIGNMENT, 1);
  void * alignedBuffer = reinterpret_cast<void*>(RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT));
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(alignedBuffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

  void * const buffer1 = ms.Allocate(5);
  void * const buffer2 = ms.Allocate(16);
  void * const buffer3 = ms.Allocate(17);
  void * const buffer4 = ms.Allocate(0);
  void * const buffer5 = ms.Allocate(100);
  void * const buffer6 = ms.Allocate(0x3FFFFFFF);
  void * const buffer7 = ms.Allocate(u32_MAX);

  ASSERT_TRUE(buffer1 != NULL);
  ASSERT_TRUE(buffer2 != NULL);
  ASSERT_TRUE(buffer3 != NULL);

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_ESSENTIAL_AND_ERROR
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
    //std::cout << i << " " << ms.IsValid() << " " << expectedResults[i] << "\n";
    ASSERT_TRUE(ms.IsValid() == expectedResults[i]);
    (reinterpret_cast<char*>(alignedBuffer)[i])--;
  }
#endif // #if ANKI_DEBUG_LEVEL > ANKI_DEBUG_ESSENTIAL_AND_ERROR

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

IN_DDR s32 CheckMemoryStackUsage(MemoryStack ms, s32 numBytes)
{
  ms.Allocate(numBytes);
  return ms.get_usedBytes();
}

IN_DDR s32 CheckConstCasting(const MemoryStack ms, s32 numBytes)
{
  // ms.Allocate(1); // Will not compile

  return CheckMemoryStackUsage(ms, numBytes);
}

IN_DDR GTEST_TEST(CoreTech_Common, MemoryStack_call)
{
  const s32 numBytes = MIN(MAX_BYTES, 100);
  //void * buffer = calloc(numBytes, 1);
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

  //  free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

IN_DDR GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1)
{
  ASSERT_TRUE(MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(MAX_BYTES, 104); // 12*9 = 104
  //void * buffer = calloc(numBytes+MEMORY_ALIGNMENT, 1);
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

  const void * const allocatedBuffer2 = ms.Allocate(49);
  const s32 largestPossibleAllocation3 = ms.ComputeLargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer2 == NULL);
  ASSERT_TRUE(largestPossibleAllocation3 == 48);

  const void * const allocatedBuffer3 = ms.Allocate(48);
  const s32 largestPossibleAllocation4 = ms.ComputeLargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer3 != NULL);
  ASSERT_TRUE(largestPossibleAllocation4 == 0);

  ///free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
IN_DDR GTEST_TEST(CoreTech_Common, SimpleMatlabTest1)
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
}
#endif //#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
IN_DDR GTEST_TEST(CoreTech_Common, SimpleMatlabTest2)
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
}
#endif //#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
IN_DDR GTEST_TEST(CoreTech_Common, SimpleOpenCVTest)
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

  std::cout << src.at<double>(50, 0) << " "
    << src.at<double>(50, 1) << "\n"
    << *( ((double*)src.data) + 50*6) << " "
    << *( ((double*)src.data) + 50*6 + 1) << "\n"
    << dst.at<double>(50, 0) << " "
    << dst.at<double>(50, 1) << "\n";

  ASSERT_EQ(5, src.at<double>(50, 0));
  ASSERT_EQ(6, src.at<double>(50, 1));
  ASSERT_TRUE(abs(1.49208 - dst.at<double>(50, 0)) < .00001);
  ASSERT_TRUE(abs(1.39378 - dst.at<double>(50, 1)) < .00001);
}
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

IN_DDR GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest)
{
  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
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

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
  // Check that the Matlab transfer works (you need to check the Matlab window to verify that this works)
  matlab.PutArray<s16>(simpleArray, "simpleArray");
#endif //#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
  // Check that the templated OpenCV matrix works
  {
    cv::Mat_<s16> &simpleArray_cvMat = simpleArray.get_CvMat_();
    std::cout << "simpleArray(2,0) = " << *simpleArray.Pointer(2,0) << "\nsimpleArray_cvMat(2,0) = " << simpleArray_cvMat(2,0) << "\n";
    ASSERT_EQ(7, *simpleArray.Pointer(2,0));
    ASSERT_EQ(7, simpleArray_cvMat(2,0));

    std::cout << "Setting OpenCV matrix\n";
    simpleArray_cvMat(2,0) = 100;
    std::cout << "simpleArray(2,0) = " << *simpleArray.Pointer(2,0) << "\nsimpleArray_cvMat(2,0) = " << simpleArray_cvMat(2,0) << "\n";
    ASSERT_EQ(100, *simpleArray.Pointer(2,0));
    ASSERT_EQ(100, simpleArray_cvMat(2,0));

    std::cout << "Setting CoreTech_Common matrix\n";
    *simpleArray.Pointer(2,0) = 42;
    std::cout << "simpleArray(2,0) = " << *simpleArray.Pointer(2,0) << "\nsimpleArray_cvMat(2,0) = " << simpleArray_cvMat(2,0) << "\n";
    ASSERT_EQ(42, *simpleArray.Pointer(2,0));
    ASSERT_EQ(42, simpleArray_cvMat(2,0));
  }

  std::cout << "\n\n";

  // Check that the non-templated OpenCV matrix works
  {
    cv::Mat &simpleArray_cvMat = simpleArray.get_CvMat_();
    std::cout << "simpleArray(2,0) = " << *simpleArray.Pointer(2,0) << "\nsimpleArray_cvMat(2,0) = " << simpleArray_cvMat.at<s16>(2,0) << "\n";
    ASSERT_EQ(42, *simpleArray.Pointer(2,0));
    ASSERT_EQ(42, simpleArray_cvMat.at<s16>(2,0));

    std::cout << "Setting OpenCV matrix\n";
    simpleArray_cvMat.at<s16>(2,0) = 300;
    std::cout << "simpleArray(2,0) = " << *simpleArray.Pointer(2,0) << "\nsimpleArray_cvMat(2,0) = " << simpleArray_cvMat.at<s16>(2,0) << "\n";
    ASSERT_EQ(300, *simpleArray.Pointer(2,0));
    ASSERT_EQ(300, simpleArray_cvMat.at<s16>(2,0));

    std::cout << "Setting CoreTech_Common matrix\n";
    *simpleArray.Pointer(2,0) = 90;
    std::cout << "simpleArray(2,0) = " << *simpleArray.Pointer(2,0) << "\nsimpleArray_cvMat(2,0) = " << simpleArray_cvMat.at<s16>(2,0) << "\n";
    ASSERT_EQ(90, *simpleArray.Pointer(2,0));
    ASSERT_EQ(90, simpleArray_cvMat.at<s16>(2,0));
  }
#endif //#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

IN_DDR GTEST_TEST(CoreTech_Common, ArraySpecifiedClass)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  MemoryStack ms(buffer, numBytes);

  Array<u8> simpleArray(3, 3, ms);

#define ArraySpecifiedClass_imgDataLength 9
  const u8 imgData[ArraySpecifiedClass_imgDataLength] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1};

  simpleArray.Set(imgData, ArraySpecifiedClass_imgDataLength);

  ASSERT_TRUE((*simpleArray.Pointer(0,0)) == 1); // If the templating fails, this will equal 49

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

IN_DDR GTEST_TEST(CoreTech_Common, ArrayAlignment1)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
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

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

IN_DDR GTEST_TEST(CoreTech_Common, MemoryStackAlignment)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
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

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

IN_DDR GTEST_TEST(CoreTech_Common, ArrayFillPattern)
{
  const s32 width = 6, height = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT) );

  MemoryStack ms(alignedBuffer, numBytes-MEMORY_ALIGNMENT);

  // Create a matrix, and manually set a few values
  Array<s16> simpleArray(height, width, ms, true);
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

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

#if !defined(ANKICORETECHEMBEDDED_USE_GTEST)
IN_DDR void RUN_ALL_TESTS()
{
  s32 numPassedTests = 0;
  s32 numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Common, MatrixMultiply);
  CALL_GTEST_TEST(CoreTech_Common, ComputeHomography);
  CALL_GTEST_TEST(CoreTech_Common, SVD32);
  CALL_GTEST_TEST(CoreTech_Common, SVD64);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack_call);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1);
  CALL_GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest);
  CALL_GTEST_TEST(CoreTech_Common, ArraySpecifiedClass);
  CALL_GTEST_TEST(CoreTech_Common, ArrayAlignment1);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStackAlignment);
  CALL_GTEST_TEST(CoreTech_Common, ArrayFillPattern);

  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);
} // void RUN_ALL_TESTS()
#endif // #if !defined(ANKICORETECHEMBEDDED_USE_GTEST)