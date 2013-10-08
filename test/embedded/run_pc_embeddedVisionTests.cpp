//#define USING_MOVIDIUS_COMPILER
#include "anki/embeddedCommon.h"

#if ANKICORETECH_EMBEDDED_USE_GTEST
#include "gtest/gtest.h"
#endif

#if ANKICORETECH_EMBEDDED_USE_GTEST
int main(int argc, char ** argv)
#else
void RUN_ALL_TESTS();
int main()
#endif
{
#if ANKICORETECH_EMBEDDED_USE_GTEST
  ::testing::InitGoogleTest(&argc, argv);
#endif

  const int result = RUN_ALL_TESTS();

  return result;
}