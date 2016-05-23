/**
 * File: testBitFlags
 *
 * Author: Mark Wesley
 * Created: 11/20/14
 *
 * Description: Unit tests for BitFlags
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "util/helpers/includeGTest.h"
#include "util/bitFlags/bitFlags.h"
#include <algorithm>


enum ETestBitFlagEnum
{
  eTF0 = 0,
   eTF1,  eTF2,  eTF3,  eTF4,  eTF5,  eTF6,  eTF7,  eTF8,  eTF9, eTF10,
  eTF11, eTF12, eTF13, eTF14, eTF15, eTF16, eTF17, eTF18, eTF19, eTF20,
  eTF21, eTF22, eTF23, eTF24, eTF25, eTF26, eTF27, eTF28, eTF29, eTF30,
  eTF31, eTF32, eTF33, eTF34, eTF35, eTF36, eTF37, eTF38, eTF39, eTF40,
  eTF41, eTF42, eTF43, eTF44, eTF45, eTF46, eTF47, eTF48, eTF49, eTF50,
  eTF51, eTF52, eTF53, eTF54, eTF55, eTF56, eTF57, eTF58, eTF59, eTF60,
  eTF61, eTF62, eTF63, eTF64,
  eTF115 = 115,
  eTF127 = 127,
  eTF253 = 253,
  eTF255 = 255
};


enum class ETestBitFlagEnumClassI8 : int8_t
{
  e0 = 0,
   e1,  e2,  e3,  e4,  e5,  e6,  e7,  e8,  e9, e10,
  e11, e12, e13, e14, e15, e16, e17, e18, e19, e20,
  e21, e22, e23, e24, e25, e26, e27, e28, e29, e30,
  e31, e32, e33, e34, e35, e36, e37, e38, e39, e40,
  e41, e42, e43, e44, e45, e46, e47, e48, e49, e50,
  e51, e52, e53, e54, e55, e56, e57, e58, e59, e60,
  e61, e62, e63, e64,
  e115 = 115,
  e127 = 127 // i8 can't go above 127
};


enum class ETestBitFlagEnumClassU64 : uint64_t
{
  e0 = 0,
  e1,  e2,  e3,  e4,  e5,  e6,  e7,  e8,  e9, e10,
  e11, e12, e13, e14, e15, e16, e17, e18, e19, e20,
  e21, e22, e23, e24, e25, e26, e27, e28, e29, e30,
  e31, e32, e33, e34, e35, e36, e37, e38, e39, e40,
  e41, e42, e43, e44, e45, e46, e47, e48, e49, e50,
  e51, e52, e53, e54, e55, e56, e57, e58, e59, e60,
  e61, e62, e63, e64,
  e115 = 115,
  e127 = 127,
  e253 = 253,
  e255 = 255
};


template<typename T, typename ET>
void TestBitFlagsForType(const std::vector<ET>& eFlagsToSet,
                         const std::vector<ET>& eFlagsToUnset,
                         const std::vector<int>& expectedTrue)
{
  using namespace Anki::Util;

  BitFlags<T, ET> bitFlags;
  const int kMaxBits = sizeof(T) * 8;
  
  for (int i=0; i < kMaxBits; ++i)
  {
    const ET eVal = (ET)i;
    EXPECT_FALSE(bitFlags.IsBitFlagSet(eVal));
  }
  
  // Set flags
  
  for (ET flag : eFlagsToSet)
  {
    bitFlags.SetBitFlag(flag, true);
  }
  
  // Unset flags
  
  for (ET flag : eFlagsToUnset)
  {
    bitFlags.SetBitFlag(flag, false);
  }
  
  // Verify which flags are set

  for (int i=0; i < kMaxBits; ++i)
  {
    const ET eVal = (ET)i;
    
    const bool shouldBeSet = std::find(expectedTrue.begin(), expectedTrue.end(), i) != expectedTrue.end();
    
    EXPECT_EQ(bitFlags.IsBitFlagSet(eVal),    shouldBeSet);
  }
  

  BitFlags<T, ET> bitFlags2;
  EXPECT_FALSE(bitFlags2.AreAnyBitsInMaskSet(bitFlags.GetFlags()));

  for (int i : expectedTrue)
  {
    const ET eVal = (ET)i;
    BitFlags<T, ET> bitFlagsJustOne;
    bitFlagsJustOne.SetBitFlag(eVal, true);
    bitFlags2.SetBitFlag(eVal, true);
    
    EXPECT_TRUE(bitFlags2.AreAnyBitsInMaskSet(bitFlags.GetFlags()));
    EXPECT_TRUE(bitFlagsJustOne.AreAnyBitsInMaskSet(bitFlags.GetFlags()));
  }
  
  EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
}


// --gtest_filter=TestBitFlags.TestBitFlagsU8
TEST(TestBitFlags, TestBitFlagsU8)
{
  std::vector<ETestBitFlagEnum> eFlagsToSet   = {eTF0, eTF3, eTF6, eTF7};
  std::vector<ETestBitFlagEnum> eFlagsToUnset = {eTF6};
  
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToSet = {ETestBitFlagEnumClassI8::e0, ETestBitFlagEnumClassI8::e3,
    ETestBitFlagEnumClassI8::e6, ETestBitFlagEnumClassI8::e7};
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToUnset = {ETestBitFlagEnumClassI8::e6};
  
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToSet = {ETestBitFlagEnumClassU64::e0, ETestBitFlagEnumClassU64::e3,
    ETestBitFlagEnumClassU64::e6, ETestBitFlagEnumClassU64::e7};
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToUnset = {ETestBitFlagEnumClassU64::e6};
  
  std::vector<int> iFlagsToSet = {0, 3, 6, 7};
  std::vector<int> iFlagsToUnset = {6};
  
  std::vector<int> expectedTrue = {0, 3, 7};
  
  TestBitFlagsForType<uint8_t, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint8_t, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint8_t, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint8_t, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
}


// --gtest_filter=TestBitFlags.TestBitFlagsU16
TEST(TestBitFlags, TestBitFlagsU16)
{
  std::vector<ETestBitFlagEnum> eFlagsToSet   = {eTF1, eTF3, eTF8, eTF13, eTF15};
  std::vector<ETestBitFlagEnum> eFlagsToUnset = {eTF13};
  
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToSet = {ETestBitFlagEnumClassI8::e1, ETestBitFlagEnumClassI8::e3,
    ETestBitFlagEnumClassI8::e8, ETestBitFlagEnumClassI8::e13, ETestBitFlagEnumClassI8::e15};
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToUnset = {ETestBitFlagEnumClassI8::e13};
  
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToSet = {ETestBitFlagEnumClassU64::e1, ETestBitFlagEnumClassU64::e3,
    ETestBitFlagEnumClassU64::e8, ETestBitFlagEnumClassU64::e13, ETestBitFlagEnumClassU64::e15};
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToUnset = {ETestBitFlagEnumClassU64::e13};
  
  std::vector<int> iFlagsToSet = {1,3,8,13,15};
  std::vector<int> iFlagsToUnset = {13};
  
  std::vector<int> expectedTrue = {1, 3, 8, 15};
  
  TestBitFlagsForType<uint16_t, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint16_t, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint16_t, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint16_t, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
}


// --gtest_filter=TestBitFlags.TestBitFlagsU32
TEST(TestBitFlags, TestBitFlagsU32)
{
  std::vector<ETestBitFlagEnum> eFlagsToSet   = {eTF1, eTF4, eTF10, eTF15, eTF25, eTF31};
  std::vector<ETestBitFlagEnum> eFlagsToUnset = {eTF4};
  
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToSet = {ETestBitFlagEnumClassI8::e1, ETestBitFlagEnumClassI8::e4,
    ETestBitFlagEnumClassI8::e10, ETestBitFlagEnumClassI8::e15, ETestBitFlagEnumClassI8::e25, ETestBitFlagEnumClassI8::e31};
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToUnset = {ETestBitFlagEnumClassI8::e4};
  
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToSet = {ETestBitFlagEnumClassU64::e1, ETestBitFlagEnumClassU64::e4,
    ETestBitFlagEnumClassU64::e10, ETestBitFlagEnumClassU64::e15, ETestBitFlagEnumClassU64::e25, ETestBitFlagEnumClassU64::e31};
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToUnset = {ETestBitFlagEnumClassU64::e4};
  
  std::vector<int> iFlagsToSet = {1,4,10,15,25,31};
  std::vector<int> iFlagsToUnset = {4};
  
  std::vector<int> expectedTrue = {1, 10, 15, 25, 31};
  
  TestBitFlagsForType<uint32_t, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint32_t, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint32_t, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint32_t, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
}


// --gtest_filter=TestBitFlags.TestBitFlagsU64
TEST(TestBitFlags, TestBitFlagsU64)
{
  std::vector<ETestBitFlagEnum> eFlagsToSet   = {eTF3, eTF7, eTF12, eTF17, eTF26, eTF33, eTF52, eTF63};
  std::vector<ETestBitFlagEnum> eFlagsToUnset = {eTF3, eTF26};
  
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToSet = {ETestBitFlagEnumClassI8::e3, ETestBitFlagEnumClassI8::e7,
    ETestBitFlagEnumClassI8::e12, ETestBitFlagEnumClassI8::e17, ETestBitFlagEnumClassI8::e26,
    ETestBitFlagEnumClassI8::e33, ETestBitFlagEnumClassI8::e52, ETestBitFlagEnumClassI8::e63};
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToUnset = {ETestBitFlagEnumClassI8::e3, ETestBitFlagEnumClassI8::e26};
  
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToSet = {ETestBitFlagEnumClassU64::e3, ETestBitFlagEnumClassU64::e7,
    ETestBitFlagEnumClassU64::e12, ETestBitFlagEnumClassU64::e17, ETestBitFlagEnumClassU64::e26,
    ETestBitFlagEnumClassU64::e33, ETestBitFlagEnumClassU64::e52, ETestBitFlagEnumClassU64::e63};
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToUnset = {ETestBitFlagEnumClassU64::e3, ETestBitFlagEnumClassU64::e26};
  
  std::vector<int> iFlagsToSet = {3,7,12,17,26,33,52,63};
  std::vector<int> iFlagsToUnset = {3, 26};
  
  std::vector<int> expectedTrue = {7, 12, 17, 33, 52, 63};
  
  TestBitFlagsForType<uint64_t, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint64_t, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint64_t, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<uint64_t, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
  
  // Verify that BitFlagStorage<> with N smaller types would work identically to the larger builtin type
  using namespace Anki::Util;
  TestBitFlagsForType<BitFlagStorage<uint32_t, 2>, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint32_t, 2>, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint32_t, 2>, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint32_t, 2>, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
  
  TestBitFlagsForType<BitFlagStorage<uint16_t, 4>, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 4>, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 4>, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 4>, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
  
  TestBitFlagsForType<BitFlagStorage<uint16_t, 8>, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 8>, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 8>, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 8>, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
}


// --gtest_filter=TestBitFlags.TestBitFlagStorage
TEST(TestBitFlags, TestBitFlagStorage)
{
  using namespace Anki::Util;
  
  BitFlagStorage<uint64_t, 4> testStorage(0);
  EXPECT_EQ(testStorage, 0);
  EXPECT_EQ(testStorage.GetVar(0), 0);
  EXPECT_EQ(testStorage.GetVar(1), 0);
  EXPECT_EQ(testStorage.GetVar(2), 0);
  EXPECT_EQ(testStorage.GetVar(3), 0);
  
  testStorage = BitFlagStorage<uint64_t, 4>(4);
  EXPECT_EQ(testStorage, 4);
  EXPECT_EQ(testStorage.GetVar(0), 4);
  EXPECT_EQ(testStorage.GetVar(1), 0);
  EXPECT_EQ(testStorage.GetVar(2), 0);
  EXPECT_EQ(testStorage.GetVar(3), 0);
  
  testStorage = testStorage << 1;
  EXPECT_EQ(testStorage, 8);
  EXPECT_EQ(testStorage, (1<<3));
  EXPECT_EQ(testStorage.GetVar(0), 8);
  EXPECT_EQ(testStorage.GetVar(0), (1<<3));
  EXPECT_EQ(testStorage.GetVar(1), 0);
  EXPECT_EQ(testStorage.GetVar(2), 0);
  EXPECT_EQ(testStorage.GetVar(3), 0);
  
  testStorage = testStorage << 8;
  EXPECT_EQ(testStorage, (1<<11));
  EXPECT_EQ(testStorage.GetVar(0), (1<<11));
  EXPECT_EQ(testStorage.GetVar(1), 0);
  EXPECT_EQ(testStorage.GetVar(2), 0);
  EXPECT_EQ(testStorage.GetVar(3), 0);
  
  testStorage = testStorage << 40;
  EXPECT_EQ(testStorage, (1ull<<51ull));
  EXPECT_EQ(testStorage.GetVar(0), (1ull<<51ull));
  EXPECT_EQ(testStorage.GetVar(1), 0);
  EXPECT_EQ(testStorage.GetVar(2), 0);
  EXPECT_EQ(testStorage.GetVar(3), 0);
  
  testStorage = testStorage << 10;
  EXPECT_EQ(testStorage, (1ull<<61ull));
  EXPECT_EQ(testStorage.GetVar(0), (1ull<<61ull));
  EXPECT_EQ(testStorage.GetVar(1), 0);
  EXPECT_EQ(testStorage.GetVar(2), 0);
  EXPECT_EQ(testStorage.GetVar(3), 0);
  
  testStorage = testStorage << 2;
  EXPECT_EQ(testStorage, (1ull<<63ull));
  EXPECT_EQ(testStorage.GetVar(0), (1ull<<63ull));
  EXPECT_EQ(testStorage.GetVar(1), 0);
  EXPECT_EQ(testStorage.GetVar(2), 0);
  EXPECT_EQ(testStorage.GetVar(3), 0);
  
  testStorage = testStorage << 1;
  EXPECT_EQ(testStorage.GetVar(0), 0);
  EXPECT_EQ(testStorage.GetVar(1), 1);
  EXPECT_EQ(testStorage.GetVar(2), 0);
  EXPECT_EQ(testStorage.GetVar(3), 0);
  
  testStorage |= BitFlagStorage<uint64_t, 4>(8);
  testStorage = testStorage << 130;
  EXPECT_EQ(testStorage.GetVar(0), 0);
  EXPECT_EQ(testStorage.GetVar(1), 0);
  EXPECT_EQ(testStorage.GetVar(2), 32);
  EXPECT_EQ(testStorage.GetVar(3), 4);
  
  BitFlagStorage<uint64_t, 4> testStorage2(0);
  
  EXPECT_FALSE(testStorage  == testStorage2);
  EXPECT_TRUE( testStorage  >  testStorage2);
  EXPECT_TRUE( testStorage  >= testStorage2);
  EXPECT_TRUE( testStorage2 <  testStorage);
  EXPECT_TRUE( testStorage2 <= testStorage);
  
  testStorage2.SetVar(2, 32);
  
  EXPECT_FALSE(testStorage  == testStorage2);
  EXPECT_TRUE( testStorage  >  testStorage2);
  EXPECT_TRUE( testStorage  >= testStorage2);
  EXPECT_TRUE( testStorage2 <  testStorage);
  EXPECT_TRUE( testStorage2 <= testStorage);
  
  testStorage2.SetVar(3,  4);
  
  EXPECT_TRUE( testStorage  == testStorage2);
  EXPECT_FALSE(testStorage  >  testStorage2);
  EXPECT_TRUE( testStorage  >= testStorage2);
  EXPECT_FALSE(testStorage2 <  testStorage);
  EXPECT_TRUE( testStorage2 <= testStorage);
  
  testStorage2.SetVar(3,  5);
  
  EXPECT_FALSE(testStorage  == testStorage2);
  EXPECT_FALSE(testStorage  >  testStorage2);
  EXPECT_FALSE(testStorage  >= testStorage2);
  EXPECT_FALSE(testStorage2 <  testStorage);
  EXPECT_FALSE(testStorage2 <= testStorage);
  
  testStorage2.SetVar(2, 31);
  
  EXPECT_FALSE(testStorage  == testStorage2);
  EXPECT_FALSE(testStorage  >  testStorage2);
  EXPECT_FALSE(testStorage  >= testStorage2);
  EXPECT_FALSE(testStorage2 <  testStorage);
  EXPECT_FALSE(testStorage2 <= testStorage);
  
  testStorage2.SetVar(3,  4);
  
  EXPECT_FALSE(testStorage  == testStorage2);
  EXPECT_TRUE( testStorage  >  testStorage2);
  EXPECT_TRUE( testStorage  >= testStorage2);
  EXPECT_TRUE( testStorage2 <  testStorage);
  EXPECT_TRUE( testStorage2 <= testStorage);
}


// --gtest_filter=TestBitFlags.TestBitFlagsU128
TEST(TestBitFlags, TestBitFlagsU128)
{
  std::vector<ETestBitFlagEnum> eFlagsToSet   = {eTF3, eTF7, eTF12, eTF17, eTF26, eTF33, eTF52, eTF63, eTF64, eTF115, eTF127};
  std::vector<ETestBitFlagEnum> eFlagsToUnset = {eTF3, eTF26, eTF115};
  
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToSet = {ETestBitFlagEnumClassI8::e3, ETestBitFlagEnumClassI8::e7,
    ETestBitFlagEnumClassI8::e12, ETestBitFlagEnumClassI8::e17, ETestBitFlagEnumClassI8::e26,
    ETestBitFlagEnumClassI8::e33, ETestBitFlagEnumClassI8::e52, ETestBitFlagEnumClassI8::e63,
    ETestBitFlagEnumClassI8::e64, ETestBitFlagEnumClassI8::e115, ETestBitFlagEnumClassI8::e127};
  std::vector<ETestBitFlagEnumClassI8>  eI8FlagsToUnset = {ETestBitFlagEnumClassI8::e3, ETestBitFlagEnumClassI8::e26, ETestBitFlagEnumClassI8::e115};
  
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToSet = {ETestBitFlagEnumClassU64::e3, ETestBitFlagEnumClassU64::e7,
    ETestBitFlagEnumClassU64::e12, ETestBitFlagEnumClassU64::e17, ETestBitFlagEnumClassU64::e26,
    ETestBitFlagEnumClassU64::e33, ETestBitFlagEnumClassU64::e52, ETestBitFlagEnumClassU64::e63,
    ETestBitFlagEnumClassU64::e64, ETestBitFlagEnumClassU64::e115, ETestBitFlagEnumClassU64::e127};
  std::vector<ETestBitFlagEnumClassU64> eU64FlagsToUnset = {ETestBitFlagEnumClassU64::e3, ETestBitFlagEnumClassU64::e26, ETestBitFlagEnumClassU64::e115};
  
  std::vector<int> iFlagsToSet = {3,7,12,17,26,33,52,63,64,115,127};
  std::vector<int> iFlagsToUnset = {3, 26, 115};
  
  std::vector<int> expectedTrue = {7, 12, 17, 33, 52, 63, 64, 127};
  
  // We Test the "U128" with every combination of backing with N UintX vars
  
  using namespace Anki::Util;
  TestBitFlagsForType<BitFlagStorage<uint64_t, 2>, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint32_t, 4>, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 8>, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint8_t, 16>, ETestBitFlagEnum>(eFlagsToSet, eFlagsToUnset, expectedTrue);
  
  TestBitFlagsForType<BitFlagStorage<uint64_t, 2>, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint32_t, 4>, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 8>, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint8_t, 16>, ETestBitFlagEnumClassI8>(eI8FlagsToSet, eI8FlagsToUnset, expectedTrue);
  
  TestBitFlagsForType<BitFlagStorage<uint64_t, 2>, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint32_t, 4>, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 8>, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint8_t, 16>, ETestBitFlagEnumClassU64>(eU64FlagsToSet, eU64FlagsToUnset, expectedTrue);
  
  TestBitFlagsForType<BitFlagStorage<uint64_t, 2>, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint32_t, 4>, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint16_t, 8>, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
  TestBitFlagsForType<BitFlagStorage<uint8_t, 16>, int>(iFlagsToSet, iFlagsToUnset, expectedTrue);
}


// --gtest_filter=TestBitFlags.TestTypedefs
TEST(TestBitFlags, TestTypedefs)
{
  // Verify that helper "using typedefs" (BitFlags8/16/32/64) work identically to specifiying the entire type
  
  using namespace Anki::Util;
  
  {
    BitFlags<uint8_t, ETestBitFlagEnumClassU64> bitFlags;
    BitFlags8<ETestBitFlagEnumClassU64> bitFlags2;

    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());

    bitFlags.SetBitFlag(ETestBitFlagEnumClassU64::e7, true);
    EXPECT_NE(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags2.SetBitFlag(ETestBitFlagEnumClassU64::e7, true);
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
  }

  {
    BitFlags<uint16_t, ETestBitFlagEnumClassU64> bitFlags;
    BitFlags16<ETestBitFlagEnumClassU64> bitFlags2;
    
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags.SetBitFlag(ETestBitFlagEnumClassU64::e14, true);
    EXPECT_NE(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags2.SetBitFlag(ETestBitFlagEnumClassU64::e14, true);
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
  }
  
  {
    BitFlags<uint32_t, ETestBitFlagEnumClassI8> bitFlags;
    BitFlags32<ETestBitFlagEnumClassI8> bitFlags2;
    
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags.SetBitFlag(ETestBitFlagEnumClassI8::e27, true);
    EXPECT_NE(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags2.SetBitFlag(ETestBitFlagEnumClassI8::e27, true);
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
  }
  
  {
    BitFlags<uint64_t, ETestBitFlagEnumClassU64> bitFlags;
    BitFlags64<ETestBitFlagEnumClassU64> bitFlags2;
    
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags.SetBitFlag(ETestBitFlagEnumClassU64::e52, true);
    EXPECT_NE(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags2.SetBitFlag(ETestBitFlagEnumClassU64::e52, true);
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
  }
  
  {
    BitFlags<BitFlagStorage<uint64_t, 2>, ETestBitFlagEnumClassU64> bitFlags;
    BitFlags128<ETestBitFlagEnumClassU64> bitFlags2;
    
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags.SetBitFlag(ETestBitFlagEnumClassU64::e115, true);
    EXPECT_NE(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags2.SetBitFlag(ETestBitFlagEnumClassU64::e115, true);
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
  }
  
  {
    BitFlags<BitFlagStorage<uint64_t, 4>, ETestBitFlagEnumClassU64> bitFlags;
    BitFlags256<ETestBitFlagEnumClassU64> bitFlags2;
    
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags.SetBitFlag(ETestBitFlagEnumClassU64::e253, true);
    EXPECT_NE(bitFlags.GetFlags(), bitFlags2.GetFlags());
    
    bitFlags2.SetBitFlag(ETestBitFlagEnumClassU64::e253, true);
    EXPECT_EQ(bitFlags.GetFlags(), bitFlags2.GetFlags());
  }
}

