#include "anki/common.h"

#include <iostream>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#if defined(ANKICORETECH_USE_MATLAB)
Anki::Matlab matlab(false);
#endif

#include "gtest/gtest.h"

TEST(CoreTech_Common, MemoryStack)
{
  const u32 numBytes = 100;
  void * buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);

  void * const buffer1 = ms.Allocate(5);
  void * const buffer2 = ms.Allocate(16);
  void * const buffer3 = ms.Allocate(1);
  void * const buffer4 = ms.Allocate(0);
  void * const buffer5 = ms.Allocate(100);
  void * const buffer6 = ms.Allocate(0x3FFFFFFF);
  void * const buffer7 = ms.Allocate(u32_MAX);
  void * const buffer8 = ms.Allocate(9);

  ASSERT_TRUE(buffer1 != NULL);
  ASSERT_TRUE(buffer2 != NULL);
  ASSERT_TRUE(buffer3 != NULL);
  ASSERT_TRUE(buffer4 == NULL);
  ASSERT_TRUE(buffer5 == NULL);
  ASSERT_TRUE(buffer6 == NULL);
  ASSERT_TRUE(buffer7 == NULL);
  ASSERT_TRUE(buffer8 == NULL);

  const size_t buffer1U32 = reinterpret_cast<size_t>(buffer1);
  const size_t buffer2U32 = reinterpret_cast<size_t>(buffer2);
  const size_t buffer3U32 = reinterpret_cast<size_t>(buffer3);

  ASSERT_TRUE((buffer2U32-buffer1U32) == 24);
  ASSERT_TRUE((buffer3U32-buffer2U32) == 32);

  ASSERT_TRUE(ms.IsConsistent());

#define NUM_EXPECTED_RESULTS 76
  const bool expectedResults[NUM_EXPECTED_RESULTS] = {
    // Allocation 1
    true,  false, false, false, // #0 just changes 5->6
    false, false, false, false,
    true,  true,  true,  true,
    true,  true,  true,  true,
    false, false, false, false,
    true,  true,  true,  true,
    // Allocation 2
    false, false, false, false,
    false, false, false, false,
    true,  true,  true,  true,
    true,  true,  true,  true,
    true,  true,  true,  true,
    true,  true,  true,  true,
    false, false, false, false,
    true,  true,  true,  true,
    // Allocation 3
    true,  false, false, false, // #56 just changes 1->2
    false, false, false, false,
    true,  true,  true,  true,
    true,  true,  true,  true,
    false, false, false, false};

  char * const alignedBufferBeginning = reinterpret_cast<char*>(buffer1) - 8;
  for(u32 i=0; i<NUM_EXPECTED_RESULTS; i++) {
    ASSERT_TRUE(ms.IsConsistent());
    (alignedBufferBeginning[i])++;
    ASSERT_TRUE(ms.IsConsistent() == expectedResults[i]);
    //std::cout << i << " " << ms.IsConsistent() << " " << expectedResults[i] << "\n";
    (alignedBufferBeginning[i])--;
  }

  std::cout << " ";

  free(buffer); buffer = NULL;
}

u32 CheckMemoryStackUsage(Anki::MemoryStack ms, u32 numBytes)
{
  ms.Allocate(numBytes);
  return ms.get_usedBytes();
}

u32 CheckConstCasting(const Anki::MemoryStack ms, u32 numBytes)
{
  // ms.Allocate(1); // Will not compile

  return CheckMemoryStackUsage(ms, numBytes);
}

TEST(CoreTech_Common, MemoryStack_call)
{
  const u32 numBytes = 100;
  void * buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);

  ASSERT_TRUE(ms.get_usedBytes() == 0);

  ms.Allocate(16);
  const u32 originalUsedBytes = ms.get_usedBytes();
  ASSERT_TRUE(originalUsedBytes >= 28);

  const u32 inFunctionUsedBytes = CheckMemoryStackUsage(ms, 32);
  ASSERT_TRUE(inFunctionUsedBytes == (originalUsedBytes+48));
  ASSERT_TRUE(ms.get_usedBytes() == originalUsedBytes);

  const u32 inFunctionUsedBytes2 = CheckConstCasting(ms, 32);
  ASSERT_TRUE(inFunctionUsedBytes2 == (originalUsedBytes+48));
  ASSERT_TRUE(ms.get_usedBytes() == originalUsedBytes);

  free(buffer); buffer = NULL;
}

TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1)
{
  const u32 numBytes = 104; // 12*9 = 104
  void * buffer = calloc(numBytes+8, 1);
  ASSERT_TRUE(buffer != NULL);

  ASSERT_TRUE(Anki::MEMORY_ALIGNMENT == 8);

  void * bufferAligned = reinterpret_cast<void*>(Anki::RoundUp<size_t>(reinterpret_cast<size_t>(buffer), Anki::MEMORY_ALIGNMENT));

  const size_t bufferShift = reinterpret_cast<size_t>(bufferAligned) - reinterpret_cast<size_t>(buffer);
  ASSERT_TRUE(bufferShift == 0 || bufferShift == static_cast<size_t>(Anki::MEMORY_ALIGNMENT));

  Anki::MemoryStack ms(bufferAligned, numBytes);
  const u32 largestPossibleAllocation1 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(largestPossibleAllocation1 == 88);

  const void * const allocatedBuffer1 = ms.Allocate(1);
  const u32 largestPossibleAllocation2 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer1 != NULL);
  ASSERT_TRUE(largestPossibleAllocation2 == 64);

  const void * const allocatedBuffer2 = ms.Allocate(65);
  const u32 largestPossibleAllocation3 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer2 == NULL);
  ASSERT_TRUE(largestPossibleAllocation3 == 64);

  const void * const allocatedBuffer3 = ms.Allocate(64);
  const u32 largestPossibleAllocation4 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer3 != NULL);
  ASSERT_TRUE(largestPossibleAllocation4 == 0);

  free(buffer); buffer = NULL;
}

TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation2)
{
  const u32 numBytes = 103; // 12*8 + 7 = 103
  void * buffer = calloc(numBytes+8, 1);
  ASSERT_TRUE(buffer != NULL);

  ASSERT_TRUE(Anki::MEMORY_ALIGNMENT == 8);

  void * bufferAligned = reinterpret_cast<void*>(Anki::RoundUp<size_t>(reinterpret_cast<size_t>(buffer), Anki::MEMORY_ALIGNMENT));

  const size_t bufferShift = reinterpret_cast<size_t>(bufferAligned) - reinterpret_cast<size_t>(buffer);
  ASSERT_TRUE(bufferShift == 0 || bufferShift == static_cast<size_t>(Anki::MEMORY_ALIGNMENT));

  Anki::MemoryStack ms(bufferAligned, numBytes);
  const u32 largestPossibleAllocation1 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(largestPossibleAllocation1 == 88);

  const void * const allocatedBuffer1 = ms.Allocate(1);
  const u32 largestPossibleAllocation2 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer1 != NULL);
  ASSERT_TRUE(largestPossibleAllocation2 == 64);

  const void * const allocatedBuffer2 = ms.Allocate(65);
  const u32 largestPossibleAllocation3 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer2 == NULL);
  ASSERT_TRUE(largestPossibleAllocation3 == 64);

  const void * const allocatedBuffer3 = ms.Allocate(64);
  const u32 largestPossibleAllocation4 = ms.LargestPossibleAllocation();
  ASSERT_TRUE(allocatedBuffer3 != NULL);
  ASSERT_TRUE(largestPossibleAllocation4 == 0);

  free(buffer); buffer = NULL;
}

#if defined(ANKICORETECH_USE_MATLAB)
TEST(CoreTech_Common, SimpleMatlabTest1)
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
#endif //#if defined(ANKICORETECH_USE_MATLAB)

#if defined(ANKICORETECH_USE_MATLAB)
TEST(CoreTech_Common, SimpleMatlabTest2)
{
  matlab.EvalStringEcho("simpleMatrix = int16([1,2,3,4,5;6,7,8,9,10]);");
  Anki::Matrix<s16> simpleMatrix = matlab.GetMatrix<s16>("simpleMatrix");
  printf("simple matrix:\n");
  simpleMatrix.Print();

  ASSERT_EQ(1, *simpleMatrix.Pointer(0,0));
  ASSERT_EQ(2, *simpleMatrix.Pointer(0,1));
  ASSERT_EQ(3, *simpleMatrix.Pointer(0,2));
  ASSERT_EQ(4, *simpleMatrix.Pointer(0,3));
  ASSERT_EQ(5, *simpleMatrix.Pointer(0,4));
  ASSERT_EQ(6, *simpleMatrix.Pointer(1,0));
  ASSERT_EQ(7, *simpleMatrix.Pointer(1,1));
  ASSERT_EQ(8, *simpleMatrix.Pointer(1,2));
  ASSERT_EQ(9, *simpleMatrix.Pointer(1,3));
  ASSERT_EQ(10, *simpleMatrix.Pointer(1,4));

  free(simpleMatrix.get_rawDataPointer());
}
#endif //#if defined(ANKICORETECH_USE_MATLAB)

#if defined(ANKICORETECH_USE_OPENCV)
TEST(CoreTech_Common, SimpleOpenCVTest)
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
#endif // #if defined(ANKICORETECH_USE_OPENCV)

TEST(CoreTech_Common, SimpleCoreTech_CommonTest)
{
  // Allocate memory from the heap, for the memory allocator
  const u32 numBytes = 1000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);

  // Create a matrix, and manually set a few values
  Anki::Matrix<s16> simpleMatrix(10, 6, ms);
  ASSERT_TRUE(simpleMatrix.get_rawDataPointer() != NULL);
  *simpleMatrix.Pointer(0,0) = 1;
  *simpleMatrix.Pointer(0,1) = 2;
  *simpleMatrix.Pointer(0,2) = 3;
  *simpleMatrix.Pointer(0,3) = 4;
  *simpleMatrix.Pointer(0,4) = 5;
  *simpleMatrix.Pointer(0,5) = 6;
  *simpleMatrix.Pointer(2,0) = 7;
  *simpleMatrix.Pointer(2,1) = 8;
  *simpleMatrix.Pointer(2,2) = 9;
  *simpleMatrix.Pointer(2,3) = 10;
  *simpleMatrix.Pointer(2,4) = 11;
  *simpleMatrix.Pointer(2,5) = 12;

#if defined(ANKICORETECH_USE_MATLAB)
  // Check that the Matlab transfer works (you need to check the Matlab window to verify that this works)
  matlab.PutMatrix(simpleMatrix, "simpleMatrix");
#endif //#if defined(ANKICORETECH_USE_MATLAB)

#if defined(ANKICORETECH_USE_OPENCV)
  // Check that the templated OpenCV matrix works
  {
    cv::Mat_<s16> &simpleMatrix_cvMat = simpleMatrix.get_CvMat_();
    std::cout << "simpleMatrix(2,0) = " << *simpleMatrix.Pointer(2,0) << "\nsimpleMatrix_cvMat(2,0) = " << simpleMatrix_cvMat(2,0) << "\n";
    ASSERT_EQ(7, *simpleMatrix.Pointer(2,0));
    ASSERT_EQ(7, simpleMatrix_cvMat(2,0));

    std::cout << "Setting OpenCV matrix\n";
    simpleMatrix_cvMat(2,0) = 100;
    std::cout << "simpleMatrix(2,0) = " << *simpleMatrix.Pointer(2,0) << "\nsimpleMatrix_cvMat(2,0) = " << simpleMatrix_cvMat(2,0) << "\n";
    ASSERT_EQ(100, *simpleMatrix.Pointer(2,0));
    ASSERT_EQ(100, simpleMatrix_cvMat(2,0));

    std::cout << "Setting CoreTech_Common matrix\n";
    *simpleMatrix.Pointer(2,0) = 42;
    std::cout << "simpleMatrix(2,0) = " << *simpleMatrix.Pointer(2,0) << "\nsimpleMatrix_cvMat(2,0) = " << simpleMatrix_cvMat(2,0) << "\n";
    ASSERT_EQ(42, *simpleMatrix.Pointer(2,0));
    ASSERT_EQ(42, simpleMatrix_cvMat(2,0));
  }

  std::cout << "\n\n";

  // Check that the non-templated OpenCV matrix works
  {
    cv::Mat &simpleMatrix_cvMat = simpleMatrix.get_CvMat_();
    std::cout << "simpleMatrix(2,0) = " << *simpleMatrix.Pointer(2,0) << "\nsimpleMatrix_cvMat(2,0) = " << simpleMatrix_cvMat.at<s16>(2,0) << "\n";
    ASSERT_EQ(42, *simpleMatrix.Pointer(2,0));
    ASSERT_EQ(42, simpleMatrix_cvMat.at<s16>(2,0));

    std::cout << "Setting OpenCV matrix\n";
    simpleMatrix_cvMat.at<s16>(2,0) = 300;
    std::cout << "simpleMatrix(2,0) = " << *simpleMatrix.Pointer(2,0) << "\nsimpleMatrix_cvMat(2,0) = " << simpleMatrix_cvMat.at<s16>(2,0) << "\n";
    ASSERT_EQ(300, *simpleMatrix.Pointer(2,0));
    ASSERT_EQ(300, simpleMatrix_cvMat.at<s16>(2,0));

    std::cout << "Setting CoreTech_Common matrix\n";
    *simpleMatrix.Pointer(2,0) = 90;
    std::cout << "simpleMatrix(2,0) = " << *simpleMatrix.Pointer(2,0) << "\nsimpleMatrix_cvMat(2,0) = " << simpleMatrix_cvMat.at<s16>(2,0) << "\n";
    ASSERT_EQ(90, *simpleMatrix.Pointer(2,0));
    ASSERT_EQ(90, simpleMatrix_cvMat.at<s16>(2,0));
  }
#endif //#if defined(ANKICORETECH_USE_OPENCV)

  free(buffer); buffer = NULL;

  std::cout << "\n";
}

TEST(CoreTech_Common, MatrixAlignment1)
{
  const u32 numBytes = 1000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *bufferAligned = reinterpret_cast<void*>( Anki::RoundUp(reinterpret_cast<u32>(buffer), Anki::MEMORY_ALIGNMENT) );

  // Check all offsets
  for(u32 offset=0; offset<8; offset++) {
    void * const bufferAlignedAndOffset = reinterpret_cast<char*>(bufferAligned) + offset;
    Anki::Matrix<s16> simpleMatrix(10, 6, bufferAlignedAndOffset, numBytes-offset-8);

    const u32 trueLocation = reinterpret_cast<u32>(simpleMatrix.Pointer(0,0));
    const u32 expectedLocation = Anki::RoundUp(reinterpret_cast<u32>(bufferAlignedAndOffset), Anki::MEMORY_ALIGNMENT);;

    ASSERT_TRUE(trueLocation ==  expectedLocation);
  }
}

TEST(CoreTech_Common, MemoryStackAlignment)
{
  const u32 numBytes = 1000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *bufferAligned = reinterpret_cast<void*>( Anki::RoundUp(reinterpret_cast<u32>(buffer), Anki::MEMORY_ALIGNMENT) );

  // Check all offsets
  for(u32 offset=0; offset<8; offset++) {
    void * const bufferAlignedAndOffset = reinterpret_cast<char*>(bufferAligned) + offset;
    Anki::MemoryStack simpleMemoryStack(bufferAlignedAndOffset, numBytes-offset-8);
    Anki::Matrix<s16> simpleMatrix(10, 6, simpleMemoryStack);

    const u32 matrixStart = reinterpret_cast<u32>(simpleMatrix.Pointer(0,0));
    ASSERT_TRUE(matrixStart == Anki::RoundUp(matrixStart, Anki::MEMORY_ALIGNMENT));
  }
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();

  return 0;
}