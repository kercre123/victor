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
  ASSERT_TRUE(Anki::MEMORY_ALIGNMENT == 16);

  const s32 numBytes = 200;
  void * buffer = calloc(numBytes+Anki::MEMORY_ALIGNMENT, 1);
  void * alignedBuffer = reinterpret_cast<void*>(Anki::RoundUp(reinterpret_cast<size_t>(buffer), Anki::MEMORY_ALIGNMENT));
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(alignedBuffer, numBytes);

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

  free(buffer); buffer = NULL;
}

s32 CheckMemoryStackUsage(Anki::MemoryStack ms, s32 numBytes)
{
  ms.Allocate(numBytes);
  return ms.get_usedBytes();
}

s32 CheckConstCasting(const Anki::MemoryStack ms, s32 numBytes)
{
  // ms.Allocate(1); // Will not compile

  return CheckMemoryStackUsage(ms, numBytes);
}

TEST(CoreTech_Common, MemoryStack_call)
{
  const s32 numBytes = 100;
  void * buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);

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

  free(buffer); buffer = NULL;
}

TEST(CoreTech_Common, MemoryStack_largestPossibleAllocation1)
{
  ASSERT_TRUE(Anki::MEMORY_ALIGNMENT == 16);

  const s32 numBytes = 104; // 12*9 = 104
  void * buffer = calloc(numBytes+Anki::MEMORY_ALIGNMENT, 1);
  ASSERT_TRUE(buffer != NULL);

  void * alignedBuffer = reinterpret_cast<void*>(Anki::RoundUp<size_t>(reinterpret_cast<size_t>(buffer), Anki::MEMORY_ALIGNMENT));

  const size_t bufferShift = reinterpret_cast<size_t>(alignedBuffer) - reinterpret_cast<size_t>(buffer);
  ASSERT_TRUE(bufferShift < static_cast<size_t>(Anki::MEMORY_ALIGNMENT));

  Anki::MemoryStack ms(alignedBuffer, numBytes);
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
  const s32 numBytes = 1000;
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
}

TEST(CoreTech_Common, MatrixAlignment1)
{
  const s32 numBytes = 1000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( Anki::RoundUp(reinterpret_cast<size_t>(buffer), Anki::MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    Anki::Matrix<s16> simpleMatrix(10, 6, alignedBufferAndOffset, numBytes-offset-8);

    const size_t trueLocation = reinterpret_cast<size_t>(simpleMatrix.Pointer(0,0));
    const size_t expectedLocation = Anki::RoundUp(reinterpret_cast<size_t>(alignedBufferAndOffset), Anki::MEMORY_ALIGNMENT);;

    ASSERT_TRUE(trueLocation ==  expectedLocation);
  }

  free(buffer); buffer = NULL;
}

TEST(CoreTech_Common, MemoryStackAlignment)
{
  const s32 numBytes = 1000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( Anki::RoundUp(reinterpret_cast<size_t>(buffer), Anki::MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    Anki::MemoryStack simpleMemoryStack(alignedBufferAndOffset, numBytes-offset-8);
    Anki::Matrix<s16> simpleMatrix(10, 6, simpleMemoryStack);

    const size_t matrixStart = reinterpret_cast<size_t>(simpleMatrix.Pointer(0,0));
    ASSERT_TRUE(matrixStart == Anki::RoundUp(matrixStart, Anki::MEMORY_ALIGNMENT));
  }

  free(buffer); buffer = NULL;
}

TEST(CoreTech_Common, MatrixFillPattern)
{
  const s32 width = 6, height = 10;
  const s32 numBytes = 1000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);

  void *alignedBuffer = reinterpret_cast<void*>( Anki::RoundUp(reinterpret_cast<size_t>(buffer), Anki::MEMORY_ALIGNMENT) );

  Anki::MemoryStack ms(alignedBuffer, numBytes-Anki::MEMORY_ALIGNMENT);

  // Create a matrix, and manually set a few values
  Anki::Matrix<s16> simpleMatrix(height, width, ms, true);
  ASSERT_TRUE(simpleMatrix.get_rawDataPointer() != NULL);

  ASSERT_TRUE(simpleMatrix.IsValid());

  char * curDataPointer = reinterpret_cast<char*>(simpleMatrix.get_rawDataPointer());

  // Unused space
  for(s32 i=0; i<8; i++) {
    ASSERT_TRUE(simpleMatrix.IsValid());
    (*curDataPointer)++;
    ASSERT_TRUE(simpleMatrix.IsValid());
    (*curDataPointer)--;

    curDataPointer++;
  }

  for(s32 y=0; y<height; y++) {
    // Header
    for(s32 x=0; x<8; x++) {
      ASSERT_TRUE(simpleMatrix.IsValid());
      (*curDataPointer)++;
      ASSERT_FALSE(simpleMatrix.IsValid());
      (*curDataPointer)--;

      curDataPointer++;
    }

    // Data
    for(s32 x=8; x<24; x++) {
      ASSERT_TRUE(simpleMatrix.IsValid());
      (*curDataPointer)++;
      ASSERT_TRUE(simpleMatrix.IsValid());
      (*curDataPointer)--;

      curDataPointer++;
    }

    // Footer
    for(s32 x=24; x<32; x++) {
      ASSERT_TRUE(simpleMatrix.IsValid());
      (*curDataPointer)++;
      ASSERT_FALSE(simpleMatrix.IsValid());
      (*curDataPointer)--;

      curDataPointer++;
    }
  }

  free(buffer); buffer = NULL;
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();

  return 0;
}