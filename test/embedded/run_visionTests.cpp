#define USING_MOVIDIUS_COMPILER

#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
#include "gtest/gtest.h"
#endif

#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
int main(int argc, char ** argv)
#else
void RUN_ALL_TESTS();
int main()
#endif
{
#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
  ::testing::InitGoogleTest(&argc, argv);
#endif

  RUN_ALL_TESTS();

  return 0;
}