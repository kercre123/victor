//#define USING_MOVIDIUS_COMPILER
#include "anki/embeddedCommon.h"

#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
#include "gtest/gtest.h"
#endif

using namespace Anki::Embedded;

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

  Array<u8> toast = AllocateArrayFromHeap<u8>(480,640);
  DrawRectangle<u8>(toast, Point<s16>(-20, 100), Point<s16>(100, 40), 5, 128, 255);
  toast.Show("toast", true);

  RUN_ALL_TESTS();

  return 0;
}