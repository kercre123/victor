#include "anki/vision.h"

#include <iostream>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "gtest/gtest.h"

#if defined(ANKICORETECH_USE_MATLAB)
Anki::Matlab matlab(false);
#endif

TEST(CoreTech_Vision, BinomialFilter)
{
  const u32 width = 10;
  const u32 height = 5;
  // Allocate memory from the heap, for the memory allocator
  const u32 numBytes = 10000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);

  Anki::Matrix<u8> img(height, width, ms);
  Anki::Matrix<u8> imgFiltered(height, width, ms);

  ASSERT_TRUE(img.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imgFiltered.get_rawDataPointer()!= NULL);

  for(u32 x=0; x<width; x++) {
    *img.Pointer(2,x) = static_cast<u32>(x);
  }

  Anki::Result result = Anki::BinomialFilter(img, imgFiltered, ms);

  //printf("img:\n");
  //img.Print();

  //printf("imgFiltered:\n");
  //imgFiltered.Print();

  ASSERT_TRUE(result == Anki::RESULT_OK);

  const u8 correctResults[5][10] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 1, 1, 1, 2, 2, 2, 3}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  for(u32 y=0; y<height; y++) {
    for(u32 x=0; x<width; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imgFiltered.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imgFiltered.Pointer(y,x));
    }
  }

  free(buffer); buffer = NULL;
}

TEST(CoreTech_Vision, DownsampleByFactor)
{
  const u32 width = 10;
  const u32 height = 4;
  const u32 downsampleFactor = 2;

  // Allocate memory from the heap, for the memory allocator
  const u32 numBytes = 10000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);

  Anki::Matrix<u8> img(height, width, ms);
  Anki::Matrix<u8> imgDownsampled(height/downsampleFactor, width/downsampleFactor, ms);

  ASSERT_TRUE(img.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imgDownsampled.get_rawDataPointer()!= NULL);

  for(u32 x=0; x<width; x++) {
    *img.Pointer(2,x) = static_cast<u32>(x);
  }

  Anki::Result result = Anki::DownsampleByFactor(img, downsampleFactor, imgDownsampled);

  ASSERT_TRUE(result == Anki::RESULT_OK);

  const u8 correctResults[2][5] = {{0, 0, 0, 0, 0}, {0, 1, 2, 3, 4}};

  for(u32 y=0; y<imgDownsampled.get_size(0); y++) {
    for(u32 x=0; x<imgDownsampled.get_size(1); x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imgDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imgDownsampled.Pointer(y,x));
    }
  }

  free(buffer); buffer = NULL;
}

TEST(CoreTech_Vision, ComputeCharacteristicScale)
{
  const u32 width = 16;
  const u32 height = 16;
  u32 numLevels = 3;
  const std::string imgData =
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
  const u32 numBytes = 10000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);

  Anki::Matrix<u8> img(height, width, ms);
  ASSERT_TRUE(img.get_rawDataPointer() != NULL);
  ASSERT_TRUE(img.Set(imgData) == width*height);

  Anki::FixedPointMatrix<u32> scaleImage(height, width, 16, ms);
  ASSERT_TRUE(scaleImage.get_rawDataPointer() != NULL);

  ASSERT_TRUE(ComputeCharacteristicScaleImage(img, numLevels, scaleImage, ms) == Anki::RESULT_OK);

  // TODO: manually compute results, and check

  free(buffer); buffer = NULL;
}

TEST(CoreTech_Vision, ComputeCharacteristicScale2)
{
  const u32 width = 640;
  const u32 height = 480;
  u32 numLevels = 6;

  /*const u32 width = 320;
  const u32 height = 240;
  u32 numLevels = 5;*/

  // Allocate memory from the heap, for the memory allocator
  const u32 numBytes = 10000000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);

  Anki::Matlab matlab(false);

  matlab.EvalStringEcho("img = rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png'));");
  //matlab.EvalStringEcho("img = imresize(rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png')), [240,320]);");
  Anki::Matrix<u8> img = matlab.GetMatrix<u8>("img");
  ASSERT_TRUE(img.get_rawDataPointer() != NULL);

  Anki::FixedPointMatrix<u32> scaleImage(height, width, 16, ms);
  ASSERT_TRUE(scaleImage.get_rawDataPointer() != NULL);

  ASSERT_TRUE(ComputeCharacteristicScaleImage(img, numLevels, scaleImage, ms) == Anki::RESULT_OK);

  matlab.PutMatrix(scaleImage, "scaleImage6_c");

  free(buffer); buffer = NULL;
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();

  return 0;
}