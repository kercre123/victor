#include "anki/vision.h"

#include <iostream>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "gtest/gtest.h"

#if defined(ANKICORETECH_USE_MATLAB)
Anki::Matlab matlab(false);
#endif

TEST(AnkiMath, BinomialFilter)
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

TEST(AnkiMath, DownsampleByFactor)
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

  //printf("img:\n");
  //img.Print();

  //printf("imgDownsampled:\n");
  //imgDownsampled.Print();

  ASSERT_TRUE(result == Anki::RESULT_OK);

  const u8 correctResults[2][5] = {{0, 0, 0, 0, 0}, {0, 1, 2, 3, 4}};

  for(u32 y=0; y<imgDownsampled.get_size(0); y++) {
    for(u32 x=0; x<imgDownsampled.get_size(1); x++) {
      printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imgDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imgDownsampled.Pointer(y,x));
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