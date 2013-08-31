#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedCommon.h"

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
Anki::Embedded::Matlab matlab(false);
#endif

#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
#include "gtest/gtest.h"
#endif

#define MAX_BYTES 1000
char buffer[MAX_BYTES];

GTEST_TEST(CoreTech_Common, MemoryStack)
{
  ASSERT_TRUE(Anki::Embedded::MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(MAX_BYTES, 200);
  //void * buffer = calloc(numBytes+Anki::Embedded::MEMORY_ALIGNMENT, 1);
  void * alignedBuffer = reinterpret_cast<void*>(Anki::Embedded::RoundUp(reinterpret_cast<size_t>(buffer), Anki::Embedded::MEMORY_ALIGNMENT));
  ASSERT_TRUE(buffer != NULL);
  Anki::Embedded::MemoryStack ms(alignedBuffer, numBytes);

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

s32 CheckMemoryStackUsage(Anki::Embedded::MemoryStack ms, s32 numBytes)
{
  ms.Allocate(numBytes);
  return ms.get_usedBytes();
}

s32 CheckConstCasting(const Anki::Embedded::MemoryStack ms, s32 numBytes)
{
  // ms.Allocate(1); // Will not compile

  return CheckMemoryStackUsage(ms, numBytes);
}

GTEST_TEST(CoreTech_Common, MemoryStack_call)
{
  const s32 numBytes = MIN(MAX_BYTES, 100);
  //void * buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::Embedded::MemoryStack ms(buffer, numBytes);

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
  ASSERT_TRUE(Anki::Embedded::MEMORY_ALIGNMENT == 16);

  const s32 numBytes = MIN(MAX_BYTES, 104); // 12*9 = 104
  //void * buffer = calloc(numBytes+Anki::Embedded::MEMORY_ALIGNMENT, 1);
  ASSERT_TRUE(buffer != NULL);

  void * alignedBuffer = reinterpret_cast<void*>(Anki::Embedded::RoundUp<size_t>(reinterpret_cast<size_t>(buffer), Anki::Embedded::MEMORY_ALIGNMENT));

  const size_t bufferShift = reinterpret_cast<size_t>(alignedBuffer) - reinterpret_cast<size_t>(buffer);
  ASSERT_TRUE(bufferShift < static_cast<size_t>(Anki::Embedded::MEMORY_ALIGNMENT));

  Anki::Embedded::MemoryStack ms(alignedBuffer, numBytes);
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
  Anki::Embedded::Array_s16 simpleArray = matlab.GetArray_s16("simpleArray");
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
  Anki::Embedded::MemoryStack ms(buffer, numBytes);

  // Create a matrix, and manually set a few values
  Anki::Embedded::Array_s16 simpleArray(10, 6, ms);
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

  Anki::Embedded::MemoryStack ms(buffer, numBytes);

  Anki::Embedded::Array_u8 simpleArray(3, 3, ms);

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

  void *alignedBuffer = reinterpret_cast<void*>( Anki::Embedded::RoundUp(reinterpret_cast<size_t>(buffer), Anki::Embedded::MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    Anki::Embedded::Array_s16 simpleArray(10, 6, alignedBufferAndOffset, numBytes-offset-8);

    const size_t trueLocation = reinterpret_cast<size_t>(simpleArray.Pointer(0,0));
    const size_t expectedLocation = Anki::Embedded::RoundUp(reinterpret_cast<size_t>(alignedBufferAndOffset), Anki::Embedded::MEMORY_ALIGNMENT);;

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

  void *alignedBuffer = reinterpret_cast<void*>( Anki::Embedded::RoundUp(reinterpret_cast<size_t>(buffer), Anki::Embedded::MEMORY_ALIGNMENT) );

  // Check all offsets
  for(s32 offset=0; offset<8; offset++) {
    void * const alignedBufferAndOffset = reinterpret_cast<char*>(alignedBuffer) + offset;
    Anki::Embedded::MemoryStack simpleMemoryStack(alignedBufferAndOffset, numBytes-offset-8);
    Anki::Embedded::Array_s16 simpleArray(10, 6, simpleMemoryStack);

    const size_t matrixStart = reinterpret_cast<size_t>(simpleArray.Pointer(0,0));
    ASSERT_TRUE(matrixStart == Anki::Embedded::RoundUp(matrixStart, Anki::Embedded::MEMORY_ALIGNMENT));
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

  void *alignedBuffer = reinterpret_cast<void*>( Anki::Embedded::RoundUp(reinterpret_cast<size_t>(buffer), Anki::Embedded::MEMORY_ALIGNMENT) );

  Anki::Embedded::MemoryStack ms(alignedBuffer, numBytes-Anki::Embedded::MEMORY_ALIGNMENT);

  // Create a matrix, and manually set a few values
  Anki::Embedded::Array_s16 simpleArray(height, width, ms, true);
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

  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);
} // void RUN_ALL_TESTS()
#endif // #if !defined(ANKICORETECHEMBEDDED_USE_GTEST)