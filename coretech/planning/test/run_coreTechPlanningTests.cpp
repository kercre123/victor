#include "gtest/gtest.h"

// NOTE: I don't think this file is used! see run_coreTechCommonTests.cpp instead!!

// using namespace Anki;

int main(int argc, char ** argv)
{

  printf("Hello! This is coreTechPlanningTests.cpp running here`n");

  ::testing::InitGoogleTest(&argc, argv);
 
  return RUN_ALL_TESTS();
  
}
