//#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedVision.h"

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
#include "gtest/gtest.h"
#endif

#if defined(ANKICORETECH_USE_MATLAB)
Anki::Embedded::Matlab matlab(false);
#endif

#define MAX_BYTES 5000
char buffer[MAX_BYTES];

GTEST_TEST(CoreTech_Vision, BinomialFilter)
{
  const s32 width = 10;
  const s32 height = 5;
  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  //void *buffer = calloc(numBytes, 1);
  //ASSERT_TRUE(buffer != NULL);

  //printf("1\n");

  //Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);
  Anki::Embedded::MemoryStack ms(buffer, numBytes);

  ASSERT_TRUE(ms.IsValid());

  //printf("1a\n");

  Anki::Embedded::Array_u8 img(height, width, ms, false);
  //Anki::Embedded::Array_u8 img(1, 1, ms, false);

  //printf("1b\n");

  Anki::Embedded::Array_u8 imgFiltered(height, width, ms);

  //printf("2\n");

  ASSERT_TRUE(img.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imgFiltered.get_rawDataPointer()!= NULL);

  //printf("3\n");

  for(s32 x=0; x<width; x++) {
    *img.Pointer(2,x) = static_cast<u8>(x);
  }

  //printf("4\n");

  Anki::Embedded::Result result = Anki::Embedded::BinomialFilter(img, imgFiltered, ms);

  //printf("5\n");

  //printf("img:\n");
  //img.Print();

  //printf("imgFiltered:\n");
  //imgFiltered.Print();

  ASSERT_TRUE(result == Anki::Embedded::RESULT_OK);

  const u8 correctResults[5][10] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 1, 1, 1, 2, 2, 2, 3}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  //printf("6\n");

  for(s32 y=0; y<height; y++) {
    for(s32 x=0; x<width; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imgFiltered.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imgFiltered.Pointer(y,x));
    }
  }

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, DownsampleByFactor)
{
  const s32 width = 10;
  const s32 height = 4;
  const s32 downsampleFactor = 2;

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 1000);
  //void *buffer = calloc(numBytes, 1);
  //ASSERT_TRUE(buffer != NULL);
  Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);

  Anki::Embedded::Array_u8 img(height, width, ms);
  Anki::Embedded::Array_u8 imgDownsampled(height/downsampleFactor, width/downsampleFactor, ms);

  ASSERT_TRUE(img.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imgDownsampled.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<width; x++) {
    *img.Pointer(2,x) = static_cast<u8>(x);
  }

  Anki::Embedded::Result result = Anki::Embedded::DownsampleByFactor(img, downsampleFactor, imgDownsampled);

  ASSERT_TRUE(result == Anki::Embedded::RESULT_OK);

  const u8 correctResults[2][5] = {{0, 0, 0, 0, 0}, {0, 1, 2, 3, 4}};

  for(s32 y=0; y<imgDownsampled.get_size(0); y++) {
    for(s32 x=0; x<imgDownsampled.get_size(1); x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imgDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imgDownsampled.Pointer(y,x));
    }
  }

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale)
{
  const s32 width = 16;
  const s32 height = 16;
  const s32 numLevels = 3;
  const char * imgData =
    "0	0	0	107	255	255	255	255	0	0	0	0	0	0	160	89 "
    "0	255	0	251	255	0	0	255	0	255	255	255	255	0	197	38 "
    "0	0	0	77	255	0	0	255	0	255	255	255	255	0	238	149 "
    "18	34	27	179	255	255	255	255	0	255	255	255	255	0	248	67 "
    "226	220	173	170	40	210	108	255	0	255	255	255	255	0	49	11 "
    "25	100	51	137	218	251	24	255	0	0	0	0	0	0	35	193 "
    "255	255	255	255	255	255	255	255	255	255	49	162	65	133	178	62 "
    "255	0	0	0	0	0	0	0	0	255	188	241	156	253	24	113 "
    "255	0	0	0	0	0	0	0	0	255	62	53	148	56	134	175 "
    "255	0	0	0	0	0	0	0	0	255	234	181	138	27	135	92 "
    "255	0	0	0	0	0	0	0	0	255	69	60	222	28	220	188 "
    "255	0	0	0	0	0	0	0	0	255	195	30	68	16	124	101 "
    "255	0	0	0	0	0	0	0	0	255	48	155	81	103	100	174 "
    "255	0	0	0	0	0	0	0	0	255	73	115	30	114	171	180 "
    "255	0	0	0	0	0	0	0	0	255	23	117	240	93	189	113 "
    "255	255	255	255	255	255	255	255	255	255	147	169	165	195	133	5";

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  //void *buffer = calloc(numBytes, 1);
  //ASSERT_TRUE(buffer != NULL);
  Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);

  Anki::Embedded::Array_u8 img(height, width, ms);
  ASSERT_TRUE(img.get_rawDataPointer() != NULL);
  ASSERT_TRUE(img.Set(imgData) == width*height);

  Anki::Embedded::Array_u32 scaleImage(height, width, ms);
  ASSERT_TRUE(scaleImage.get_rawDataPointer() != NULL);

  ASSERT_TRUE(ComputeCharacteristicScaleImage(img, numLevels, scaleImage, ms) == Anki::Embedded::RESULT_OK);

  // TODO: manually compute results, and check

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

#if defined(ANKICORETECH_USE_MATLAB)
GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale2)
{
  const s32 width = 640;
  const s32 height = 480;
  const s32 numLevels = 6;

  /*const s32 width = 320;
  const s32 height = 240;
  const s32 numLevels = 5;*/

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = 10000000;
  void *buffer = calloc(numBytes, 1);
  //ASSERT_TRUE(buffer != NULL);
  Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);

  Anki::Embedded::Matlab matlab(false);

  matlab.EvalStringEcho("img = rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png'));");
  //matlab.EvalStringEcho("img = imresize(rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png')), [240,320]);");
  Anki::Embedded::Array_u8 img = matlab.GetArray_u8("img");
  ASSERT_TRUE(img.get_rawDataPointer() != NULL);

  Anki::Embedded::Array_u32 scaleImage(height, width, 16, ms);
  ASSERT_TRUE(scaleImage.get_rawDataPointer() != NULL);

  ASSERT_TRUE(ComputeCharacteristicScaleImage(img, numLevels, scaleImage, ms) == Anki::Embedded::RESULT_OK);

  matlab.PutArray2d(scaleImage, "scaleImage6_c");

  free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}
#endif // #if defined(ANKICORETECH_USE_MATLAB)

GTEST_TEST(CoreTech_Vision, TraceBoundary)
{
  const s32 width = 16;
  const s32 height = 16;
  const char * imgData =
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 0 0 0 0 0 0 0 0 0 1 1 1 1 1 "
    " 1 1 0 1 1 1 0 1 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 1 0 1 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 0 0 0 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 0 0 0 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 0 0 0 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 1 1 1 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 1 1 1 1 1 0 1 1 1 1 1 "
    " 1 1 1 0 0 0 0 0 0 0 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 ";

  const s32 numPoints = 9;
  const Anki::Embedded::Point_s16 groundTruth[9] = {Anki::Embedded::Point_s16(8,6), Anki::Embedded::Point_s16(8,5), Anki::Embedded::Point_s16(7,4), Anki::Embedded::Point_s16(7,3), Anki::Embedded::Point_s16(8,3), Anki::Embedded::Point_s16(9,3), Anki::Embedded::Point_s16(9,4), Anki::Embedded::Point_s16(9,5), Anki::Embedded::Point_s16(9,6)};

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 5000);
  //void *buffer = calloc(numBytes, 1);
  //ASSERT_TRUE(buffer != NULL);
  Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);

  Anki::Embedded::Array_u8 binaryImg(height, width, ms);
  const Anki::Embedded::Point_s16 startPoint(8,6);
  const Anki::Embedded::BoundaryDirection initialDirection = Anki::Embedded::BOUNDARY_N;
  Anki::Embedded::FixedLengthList_Point_s16 boundary(Anki::Embedded::MAX_BOUNDARY_LENGTH, ms);

  ASSERT_TRUE(binaryImg.IsValid());
  ASSERT_TRUE(boundary.IsValid());

  binaryImg.Set(imgData);

  ASSERT_TRUE(Anki::Embedded::TraceBoundary(binaryImg, startPoint, initialDirection, boundary) == Anki::Embedded::RESULT_OK);

  ASSERT_TRUE(boundary.get_size() == numPoints);
  for(s32 iPoint=0; iPoint<numPoints; iPoint++) {
    ASSERT_TRUE(*boundary.Pointer(iPoint) == groundTruth[iPoint]);
  }

  //free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}

#if !defined(ANKICORETECHEMBEDDED_USE_GTEST)
void RUN_ALL_TESTS()
{
  s32 numPassedTests = 0;
  s32 numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Vision, BinomialFilter);
  CALL_GTEST_TEST(CoreTech_Vision, DownsampleByFactor);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale);
  CALL_GTEST_TEST(CoreTech_Vision, TraceBoundary);

  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);
} // void RUN_ALL_TESTS()
#endif // #if !defined(ANKICORETECHEMBEDDED_USE_GTEST)

#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
int main(int argc, char ** argv)
#else
int main()
#endif
{
#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
  ::testing::InitGoogleTest(&argc, argv);
#endif

  RUN_ALL_TESTS();

  return 0;
}