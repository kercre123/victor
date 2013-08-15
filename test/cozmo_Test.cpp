#include "cozmo.h"

#include <iostream>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "gtest/gtest.h"

#if defined(ANKICORETECH_USE_MATLAB)
Anki::Matlab matlab(false);
#endif

TEST(Cozmo, SimpleCozmoTest)
{
  testFunc();

  ASSERT_TRUE(true);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();

  return 0;
}