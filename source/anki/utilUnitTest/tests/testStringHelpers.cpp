/**
 * File: testStringHelpers
 *
 * Author: Mark Wesley
 * Created: 05/11/16
 *
 * Description: Unit tests for stringHelpers
 *
 * Copyright: Anki, Inc. 2016
 *
 * --gtest_filter=StringHelpers*
 **/


#include "util/string/stringHelpers.h"
#include "util/helpers/includeGTest.h"
#include "util/string/stringHelpers.h"


static int ToMin1Plus1Range(int inVal)
{
  return (inVal < 0) ? -1:
         (inVal > 0) ?  1: 0;
}


TEST(StringHelpers, StringCompare_stricmp)
{
  const char* testString1upper = "TEST1";
  const char* testString1lower = "test1";
  const char* testString1mixed = "TeSt1";
  const char* testString2upper = "TEST2";
  const char* testString2lower = "test2";
  const char* testString2mixed = "TeSt2";
  const char* testString3upper = "3TST";
  const char* testString3lower = "3tst";
  const char* testString3mixed = "3TSt";
  const char* testEmptyString  = "";
  
  // Do strings match regardless of case
  EXPECT_EQ( Anki::Util::stricmp(testString1upper, testString1lower), 0);
  EXPECT_EQ( Anki::Util::stricmp(testString1mixed, testString1lower), 0);
  EXPECT_EQ( Anki::Util::stricmp(testString2upper, testString2lower), 0);
  EXPECT_EQ( Anki::Util::stricmp(testString2mixed, testString2lower), 0);
  EXPECT_EQ( Anki::Util::stricmp(testString3upper, testString3lower), 0);
  EXPECT_EQ( Anki::Util::stricmp(testString3mixed, testString3lower), 0);
  
  // Do different strings sort in correct order, regardless of paramater order
  {
    const int res1 = Anki::Util::stricmp(testString1upper, testString2upper);
    const int res2 = Anki::Util::stricmp(testString2upper, testString1upper);
    EXPECT_EQ(res1, -res2);
    EXPECT_EQ(res1, -1);
    EXPECT_EQ(res2,  1);
  }
  {
    const int res1 = Anki::Util::stricmp(testString1upper, testString2upper);
    const int res2 = Anki::Util::stricmp(testString2upper, testString1upper);
    EXPECT_EQ(res1, -res2);
    EXPECT_LT(res1, 0);
    EXPECT_GT(res2, 0);
  }
  
  // Compare with empty string
  EXPECT_GT( Anki::Util::stricmp(testString1upper, testEmptyString), 0);
  EXPECT_LT( Anki::Util::stricmp(testEmptyString, testString1upper), 0);
  
  // Should behave the same as strcmp for all-lowercase strings
  // NOTE: strcmp implementations vary between returning -1,0,1 or -diff,0,diff so cannot compare result directly
  //      we just want to ensure that they are -ve,0,+ve at the same times
  EXPECT_EQ( ToMin1Plus1Range(Anki::Util::stricmp(testString1lower, testString2lower)), ToMin1Plus1Range(strcmp(testString1lower, testString2lower)) );
  EXPECT_EQ( ToMin1Plus1Range(Anki::Util::stricmp(testString2lower, testString1lower)), ToMin1Plus1Range(strcmp(testString2lower, testString1lower)) );
  EXPECT_EQ( ToMin1Plus1Range(Anki::Util::stricmp(testString2lower, testString3lower)), ToMin1Plus1Range(strcmp(testString2lower, testString3lower)) );
  EXPECT_EQ( ToMin1Plus1Range(Anki::Util::stricmp(testString3lower, testString2lower)), ToMin1Plus1Range(strcmp(testString3lower, testString2lower)) );
  EXPECT_EQ( ToMin1Plus1Range(Anki::Util::stricmp(testString1lower, testString3lower)), ToMin1Plus1Range(strcmp(testString1lower, testString3lower)) );
  EXPECT_EQ( ToMin1Plus1Range(Anki::Util::stricmp(testString3lower, testString1lower)), ToMin1Plus1Range(strcmp(testString3lower, testString1lower)) );
}


