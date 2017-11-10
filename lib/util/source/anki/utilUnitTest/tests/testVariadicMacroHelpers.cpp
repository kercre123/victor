/**
 * File: testVariadicMacroHelpers.cpp
 *
 * Author: Matt Michini
 * Created: 11/7/17
 *
 * Description: Unit tests for variadic macro helpers
 *
 * Copyright: Anki, Inc. 2017
 *
 * --gtest_filter=VariadicMacroHelpers*
 **/


#include "util/helpers/includeGTest.h"
#include "util/helpers/variadicMacroHelpers.h"


TEST(VariadicMacroHelpers, nArgs)
{
  #define TEST_VA_ARGS(input1, input2, ...) PP_NARG(__VA_ARGS__)
  
  ASSERT_EQ(1, TEST_VA_ARGS(dummy, "dummy", va1));
  ASSERT_EQ(2, TEST_VA_ARGS(dummy, "dummy", va1, va2));
  ASSERT_EQ(3, TEST_VA_ARGS(dummy, "dummy", va1, va2, va3));
}

TEST(VariadicMacroHelpers, Stringize)
{
  ASSERT_EQ(0, strcmp("testString", PP_STRINGIZE_X(testString)));
  
  const std::vector<std::string>& strVec = {PP_STRINGIZE_X(str0, str1, str2)};
  
  ASSERT_EQ(3, strVec.size());
  
  ASSERT_EQ(0, strcmp("str0", strVec[0].c_str()));
  ASSERT_EQ(0, strcmp("str1", strVec[1].c_str()));
  ASSERT_EQ(0, strcmp("str2", strVec[2].c_str()));
}

