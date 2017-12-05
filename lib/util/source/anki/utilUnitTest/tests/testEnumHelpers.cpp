/**
 * File: testEnumHelpers
 *
 * Author: Mark Wesley
 * Created: 11/10/15
 *
 * Description: Unit tests for enum helpers
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=EnumHelpers*
 **/


#include "util/enums/enumOperators.h"
#include "util/enums/stringToEnumMapper.hpp"
#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"
#include "json/json.h"


// ================================================================================
// Enum declaration - i.e. everything that would normally be in a header file:


enum class TestEnum : uint8_t {
  TestVal1 = 0,
  TestVal2,
  TestVal3,
  TestVal4,
  TestVal5,
  TestVal6,
  TestVal7,
  TestVal8,
  Count
};


const char* EnumToString(const TestEnum m);

TestEnum TestEnumFromStringI(const char* inString);
TestEnum TestEnumFromStringC(const char* inString);

DECLARE_ENUM_INCREMENT_OPERATORS(TestEnum);


// ================================================================================
// Tests


TEST(EnumHelpers, TestPreIncrement)
{
  {
    int numLoops = 0;
    for (TestEnum i = TestEnum(0); i < TestEnum::Count; ++i)
    {
      ++numLoops;
    }
    EXPECT_EQ(numLoops, 8);
  }
  
  {
    // Pre Increment -> increment should occur before assignment
    TestEnum enumVal1 = TestEnum::TestVal1;
    TestEnum enumVal2 = ++enumVal1;
    EXPECT_EQ(enumVal1, TestEnum::TestVal2);
    EXPECT_EQ(enumVal2, TestEnum::TestVal2);
    EXPECT_EQ(enumVal1, enumVal2);
  }
}


TEST(EnumHelpers, TestPostIncrement)
{
  {
    int numLoops = 0;
    for (TestEnum i = TestEnum(0); i < TestEnum::Count; i++)
    {
      ++numLoops;
    }
    EXPECT_EQ(numLoops, 8);
  }
  
  {
    // Post Increment -> increment should occur after assignment
    TestEnum enumVal1 = TestEnum::TestVal1;
    TestEnum enumVal2 = enumVal1++;
    EXPECT_EQ(enumVal1, TestEnum::TestVal2);
    EXPECT_EQ(enumVal2, TestEnum::TestVal1);
    EXPECT_NE(enumVal1, enumVal2);
  }
}


TEST(EnumHelpers, TestStringToEnum)
{
  const TestEnum testVal1I    = TestEnumFromStringI("TestVal1");
  const TestEnum testVal1C    = TestEnumFromStringC("TestVal1");
  const TestEnum testVal2LCI  = TestEnumFromStringI("testval2");
  const TestEnum testVal2LCC  = TestEnumFromStringC("testval2");
  const TestEnum testVal3UCI  = TestEnumFromStringI("TESTVAL3");
  const TestEnum testVal3UCC  = TestEnumFromStringC("TESTVAL3");
  const TestEnum testInvalidI = TestEnumFromStringI("Blah");
  const TestEnum testInvalidC = TestEnumFromStringC("Blah");
  
  EXPECT_EQ(testVal1I,    TestEnum::TestVal1);
  EXPECT_EQ(testVal1C,    TestEnum::TestVal1);
  EXPECT_EQ(testVal2LCI,  TestEnum::TestVal2); // Case insenstive -> match
  EXPECT_EQ(testVal2LCC,  TestEnum::Count);    // Case sensitive  -> fail
  EXPECT_EQ(testVal3UCI,  TestEnum::TestVal3); // Case insenstive -> match
  EXPECT_EQ(testVal3UCC,  TestEnum::Count);    // Case sensitive  -> fail
  EXPECT_EQ(testInvalidI, TestEnum::Count);
  EXPECT_EQ(testInvalidC, TestEnum::Count);

  // Test that all values round-trip
  
  for (TestEnum i = TestEnum(0); i < TestEnum::Count; i++)
  {
    const char* enumAsString = EnumToString(i);
    TestEnum stringAsEnumI = TestEnumFromStringI(enumAsString);
    TestEnum stringAsEnumC = TestEnumFromStringC(enumAsString);
    EXPECT_EQ(i, stringAsEnumI);
    EXPECT_EQ(i, stringAsEnumC);
  }
}


// ================================================================================
// Enum implementations - i.e. everything that would normally be in a separate CPP file:


const char* EnumToString(const TestEnum m)
{
  switch (m)
  {
    case TestEnum::TestVal1: return "TestVal1";
    case TestEnum::TestVal2: return "TestVal2";
    case TestEnum::TestVal3: return "TestVal3";
    case TestEnum::TestVal4: return "TestVal4";
    case TestEnum::TestVal5: return "TestVal5";
    case TestEnum::TestVal6: return "TestVal6";
    case TestEnum::TestVal7: return "TestVal7";
    case TestEnum::TestVal8: return "TestVal8";
    case TestEnum::Count:    return "Count";
    default:                 return nullptr;
  }
}


IMPLEMENT_ENUM_INCREMENT_OPERATORS(TestEnum);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapperI<TestEnum> gStringToTestEnumMapperI;

TestEnum TestEnumFromStringI(const char* inString)
{
  return gStringToTestEnumMapperI.GetTypeFromString(inString);
}


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<TestEnum> gStringToTestEnumMapper;

TestEnum TestEnumFromStringC(const char* inString)
{
  return gStringToTestEnumMapper.GetTypeFromString(inString);
}
