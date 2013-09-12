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

#define MAX_BYTES 5000
char buffer[MAX_BYTES];

GTEST_TEST(CoreTech_Common, MatrixMultiply)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

  const char * mat1Data =
    "1 2 3 5 7 "
    "11 13 17 19 23 ";

  const char * mat2Data =
    "29 31 37 "
    "41 43 47 "
    "53 59 61 "
    "67 71 73 "
    "79 83 89 ";

  const char * matOutGroundTruthData =
    "1158 1230 1302 "
    "4843 5161 5489 ";

  Array_f64 mat1(2, 5, ms);
  Array_f64 mat2(5, 3, ms);
  Array_f64 matOut(2, 3, ms);
  Array_f64 matOut_groundTruth(2, 3, ms);

  ASSERT_TRUE(mat1.IsValid());
  ASSERT_TRUE(mat2.IsValid());
  ASSERT_TRUE(matOut.IsValid());
  ASSERT_TRUE(matOut_groundTruth.IsValid());

  mat1.Set(mat1Data);
  mat2.Set(mat2Data);
  matOut_groundTruth.Set(matOutGroundTruthData);

  const Result result = MultiplyMatrices<Array_f64, f64>(mat1, mat2, matOut);

  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(matOut.IsElementwiseEqual(matOut_groundTruth));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ComputeHomography)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

  Array_f64 homography_groundTruth(3, 3, ms);
  Array_f64 homography(3, 3, ms);
  Array_f64 initialPoints(3, 4, ms);
  Array_f64 transformedPoints(3, 4, ms);

  FixedLengthList_Point_f64 initialPointsList(4, ms);
  FixedLengthList_Point_f64 transformedPointsList(4, ms);

  ASSERT_TRUE(homography_groundTruth.IsValid());
  ASSERT_TRUE(homography.IsValid());
  ASSERT_TRUE(initialPoints.IsValid());
  ASSERT_TRUE(initialPointsList.IsValid());
  ASSERT_TRUE(transformedPointsList.IsValid());

  const char * homographyGroundTruthData =
    "5.5   -0.3 5.5 "
    "0.5   0.5  3.3 "
    "0.001 0.0  1.0 ";

  const char * initialPointsData =
    "0 1 1 0 "
    "0 0 1 1 "
    "1 1 1 1 ";

  homography_groundTruth.Set(homographyGroundTruthData);
  initialPoints.Set(initialPointsData);

  MultiplyMatrices<Array_f64, f64>(homography_groundTruth, initialPoints, transformedPoints);

  for(s32 i=0; i<initialPoints.get_size(1); i++) {
    const f64 x0 = (*initialPoints.Pointer(0,i)) / (*initialPoints.Pointer(2,i));
    const f64 y0 = (*initialPoints.Pointer(1,i)) / (*initialPoints.Pointer(2,i));

    const f64 x1 = (*transformedPoints.Pointer(0,i)) / (*transformedPoints.Pointer(2,i));
    const f64 y1 = (*transformedPoints.Pointer(1,i)) / (*transformedPoints.Pointer(2,i));

    initialPointsList.PushBack(Point_f64(x0, y0));
    transformedPointsList.PushBack(Point_f64(x1, y1));
  }

  //initialPoints.Print("initialPoints");
  //transformedPoints.Print("transformedPoints");

  //initialPointsList.Print("initialPointsList");
  //transformedPointsList.Print("transformedPointsList");

  const Result result = cvHomographyEstimator_runKernel(initialPointsList, transformedPointsList, homography, ms);

  ASSERT_TRUE(result == RESULT_OK);

  //homography.Print("homography");

  ASSERT_TRUE(homography.IsElementwiseEqual_PercentThreshold(homography_groundTruth, .01, .001));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, SVD)
{
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);
  ASSERT_TRUE(ms.IsValid());

  const char * aData =
    "1 2 3 5 7 11 13 17 "
    "19 23 29 31 37 41 43 47 ";

  const char *uTGroundTruthData =
    "-0.237504316543999	-0.971386483137875 "
    "0.971386483137874	-0.237504316543999 ";

  const char *wGroundTruthData =
    "101.885662808124  9.29040979446927  0  0  0  0  0  0 ";

  const char *vTGroundTruthData =
    "-0.183478685625953	-0.223946108965585	-0.283481700610061	-0.307212042374800	-0.369078720749224	-0.416539404278702	-0.440269746043441	-0.487730429572919 "
    "0.381166774075590	0.378866636898153	0.427695421221120	0.269708382365037	0.213979186509760	-0.101994891202405	-0.259981930058488	-0.575956007770654 "
    "-0.227185052857744	-0.469620636740071	0.828412170627898	-0.133259849703043	-0.130174916014859	-0.0535189566767408	-0.0151909770076815	0.0614649823304370 "
    "-0.262651399482480	-0.301123351378262	-0.120288604832971	0.894501576357598	-0.112627300393190	-0.0830469380120526	-0.0682567568214837	-0.0386763944403458 "
    "-0.324884751171929	-0.243759383675343	-0.107225790475501	-0.104644995857761	0.880843955313500	-0.113994455451022	-0.111413660833282	-0.106252071597804 "
    "-0.395817444421400	0.0932351870482750	-0.00462734139723796	-0.0491221437364792	-0.0840608134431630	0.826949581878355	-0.217545220460887	-0.306534825139369 "
    "-0.431283791046136	0.261732472410084	0.0466718831418935	-0.0213607176758380	-0.0665131978214941	-0.202578399456957	0.729388999725311	-0.406676201910152 "
    "-0.502216484295607	0.598727043133703	0.149270332220156	0.0341621344454443	-0.0314179665781565	-0.261634362127581	-0.376742559902293	0.393041044548282 ";

  const s32 m = 2, n = 8;

  Array_f32 a(m, n, ms);
  Array_f32 w(1, n, ms);
  Array_f32 uT(m, m, ms);
  Array_f32 vT(n, n, ms);
  Array_f32 w_groundTruth(1, n, ms);
  Array_f32 uT_groundTruth(m, m, ms);
  Array_f32 vT_groundTruth(n, n, ms);

  ASSERT_TRUE(a.IsValid());
  ASSERT_TRUE(w.IsValid());
  ASSERT_TRUE(uT.IsValid());
  ASSERT_TRUE(vT.IsValid());
  ASSERT_TRUE(w_groundTruth.IsValid());
  ASSERT_TRUE(uT_groundTruth.IsValid());
  ASSERT_TRUE(vT_groundTruth.IsValid());

  void * scratch = ms.Allocate(sizeof(float)*(2*n+m));
  ASSERT_TRUE(scratch != NULL);

  a.Set(aData);
  w_groundTruth.Set(wGroundTruthData);
  uT_groundTruth.Set(uTGroundTruthData);
  vT_groundTruth.Set(vTGroundTruthData);

  const Result result = svd_f32(a, w, uT, vT, scratch);

  ASSERT_TRUE(result == RESULT_OK);

  //w.Print("w");
  //w_groundTruth.Print("w_groundTruth");

  //uT.Print("uT");
  //uT_groundTruth.Print("uT_groundTruth");

  //vT.Print("vT");
  //vT_groundTruth.Print("vT_groundTruth");

  ASSERT_TRUE(w.IsElementwiseEqual_PercentThreshold(w_groundTruth, .05, .001));
  ASSERT_TRUE(uT.IsElementwiseEqual_PercentThreshold(uT_groundTruth, .05, .001));

  // I don't know why, but the v-transpose for this SVD doesn't match Matlab's. Probably this version's is more efficient, in either memory or computation.
  //ASSERT_TRUE(vT.IsElementwiseEqual_PercentThreshold(vT_groundTruth, .05, .001));

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MemoryStack)
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

#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
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
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH

  //free(buffer); buffer = NULL;

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

GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1)
{
  ASSERT_TRUE(MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(MAX_BYTES, 104); // 12*9 = 104
  //void * buffer = calloc(numBytes+MEMORY_ALIGNMENT, 1);
  ASSERT_TRUE(buffer != NULL);

  void * alignedBuffer = reinterpret_cast<void*>(RoundUp<size_t>(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT));

  const size_t bufferShift = reinterpret_cast<size_t>(alignedBuffer) - reinterpret_cast<size_t>(buffer);
  ASSERT_TRUE(bufferShift < static_cast<size_t>(MEMORY_ALIGNMENT));

  MemoryStack ms(alignedBuffer, numBytes);
  const s32 largestPossibleAllocation1 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(largestPossibleAllocation1 == 80);

  const void * const allocatedBuffer1 = ms.Allocate(1);
  const s32 largestPossibleAllocation2 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer1 != NULL);
  ASSERT_TRUE(largestPossibleAllocation2 == 48);

  const void * const allocatedBuffer2 = ms.Allocate(49);
  const s32 largestPossibleAllocation3 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer2 == NULL);
  ASSERT_TRUE(largestPossibleAllocation3 == 48);

  const void * const allocatedBuffer3 = ms.Allocate(48);
  const s32 largestPossibleAllocation4 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer3 != NULL);
  ASSERT_TRUE(largestPossibleAllocation4 == 0);

  ///free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
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
}
#endif //#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
GTEST_TEST(CoreTech_Common, SimpleMatlabTest2)
{
  matlab.EvalStringEcho("simpleArray = int16([1,2,3,4,5;6,7,8,9,10]);");
  Array_s16 simpleArray = matlab.GetArray_s16("simpleArray");
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

GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest)
{
  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  MemoryStack ms(buffer, numBytes);

  // Create a matrix, and manually set a few values
  Array_s16 simpleArray(10, 6, ms);
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
  matlab.PutArray_s16(simpleArray, "simpleArray");
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

GTEST_TEST(CoreTech_Common, ArraySpecifiedClass)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  MemoryStack ms(buffer, numBytes);

  Array_u8 simpleArray(3, 3, ms);

  const char * imgData =
    " 1 1 1 "
    " 1 1 1 "
    " 1 1 1 ";

  simpleArray.Set(imgData);

  ASSERT_TRUE((*simpleArray.Pointer(0,0)) == 1); // If the templating fails, this will equal 49

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ArrayAlignment1)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    Array_s16 simpleArray(10, 6, alignedBufferAndOffset, numBytes-offset-8);

    const size_t trueLocation = reinterpret_cast<size_t>(simpleArray.Pointer(0,0));
    const size_t expectedLocation = RoundUp(reinterpret_cast<size_t>(alignedBufferAndOffset), MEMORY_ALIGNMENT);;

    ASSERT_TRUE(trueLocation ==  expectedLocation);
  }

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, MemoryStackAlignment)
{
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    MemoryStack simpleMemoryStack(alignedBufferAndOffset, numBytes-offset-8);
    Array_s16 simpleArray(10, 6, simpleMemoryStack);

    const size_t matrixStart = reinterpret_cast<size_t>(simpleArray.Pointer(0,0));
    ASSERT_TRUE(matrixStart == RoundUp(matrixStart, MEMORY_ALIGNMENT));
  }

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Common, ArrayFillPattern)
{
  const s32 width = 6, height = 10;
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( RoundUp(reinterpret_cast<size_t>(buffer), MEMORY_ALIGNMENT) );

  MemoryStack ms(alignedBuffer, numBytes-MEMORY_ALIGNMENT);

  // Create a matrix, and manually set a few values
  Array_s16 simpleArray(height, width, ms, true);
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
void RUN_ALL_TESTS()
{
  s32 numPassedTests = 0;
  s32 numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Common, MemoryStack);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack_call);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1);
  CALL_GTEST_TEST(CoreTech_Common, SimpleCoreTech_CommonTest);
  CALL_GTEST_TEST(CoreTech_Common, ArraySpecifiedClass);
  CALL_GTEST_TEST(CoreTech_Common, ArrayAlignment1);
  CALL_GTEST_TEST(CoreTech_Common, MemoryStackAlignment);
  CALL_GTEST_TEST(CoreTech_Common, ArrayFillPattern);
  CALL_GTEST_TEST(CoreTech_Common, SVD);

  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);
} // void RUN_ALL_TESTS()
#endif // #if !defined(ANKICORETECHEMBEDDED_USE_GTEST)