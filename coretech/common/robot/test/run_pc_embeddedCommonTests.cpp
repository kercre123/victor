/**
File: run_pc_embeddedCommonTests.cpp
Author: Peter Barnum
Created: 2013

Code to run the unit tests in embeddedCommonTests.cpp on a PC.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef ROBOT_HARDWARE

#include "anki/common/robot/config.h"

#if ANKICORETECH_EMBEDDED_USE_GTEST
#include "gtest/gtest.h"
#endif

#if ANKICORETECH_EMBEDDED_USE_GTEST
int main(int argc, char ** argv)
#else
int RUN_ALL_TESTS();
int main()
#endif
{
#if ANKICORETECH_EMBEDDED_USE_GTEST
  ::testing::InitGoogleTest(&argc, argv);
#endif

  const int result = RUN_ALL_TESTS();

  return result;
}
#endif