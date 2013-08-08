
#include "anki/vision.h"

#include <iostream>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "gtest/gtest.h"

#if defined(ANKICORETECH_USE_MATLAB)
AnkiMatlabInterface::Matlab matlab(false);
#endif

TEST(AnkiMath, SimpleAnkiVisionTest)
{
  // Allocate memory from the heap, for the memory allocator
  const u32 numBytes = 1000;
  void *buffer = calloc(numBytes, 1);
  ASSERT_TRUE(buffer != NULL);
  Anki::MemoryStack ms(buffer, numBytes);
  
  free(buffer); buffer = NULL;
}

int main(int argc, char ** argv)
{  
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();

  return 0;
}

