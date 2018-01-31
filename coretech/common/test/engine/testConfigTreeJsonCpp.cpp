/**
 * File: testConfigTreeJsonCpp.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-01-06
 *
 * Description: unit tests for parsing a json file and the tools in jsonTools.h
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include <fstream>
#include "util/helpers/includeGTest.h"
#include "util/helpers/quoteMacro.h"
#include "json/json.h"
#include "coretech/common/engine/jsonTools.h"

using namespace std;

#ifndef TEST_DATA_PATH
#error No TEST_DATA_PATH defined. You may need to re-run cmake.
#endif

#define TEST_PRIM_FILE "/planning/matlab/test_mprim.json"
#define TEST_JSON_FILE "/common/engine/config/test.json"

GTEST_TEST(TestConfigTreeJsonCpp, JsonCppWorks)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  using namespace Anki;

  const std::string & test_json_path = std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_JSON_FILE);
  ifstream configFileStream(test_json_path);

  ASSERT_TRUE(reader.parse(configFileStream, root))
    << "Failed to parse configuration\n"
    << reader.getFormattedErrorMessages();

  EXPECT_EQ(root["key"].asString(), "value");
  EXPECT_EQ(root.get("key", "wrong"), "value");
  EXPECT_EQ(root.get("missing", "missing"), "missing");

  EXPECT_FLOAT_EQ(root["floatKey"].asFloat(), 3.1415);

  Json::Value list1 = root["list"];
  ASSERT_EQ(list1.size(), 4);
  EXPECT_EQ(list1[0].asInt(), 1);
  EXPECT_EQ(list1[1].asInt(), 2);
  EXPECT_EQ(list1[2].asInt(), 3);
  EXPECT_EQ(list1[3].asInt(), 4);

  EXPECT_EQ(root["dict"]["list"][1]["bv"].asBool(), true);

  Json::Value::Members members = root["dict"]["imporatantKeys"].getMemberNames();
  ASSERT_EQ(members.size(), 2);
  
  EXPECT_TRUE(members[0] == "key1" || members[1] == "key1");
  EXPECT_TRUE(members[0] == "key2" || members[1] == "key2");
}

GTEST_TEST(TestConfigTreeJsonCpp, OptionalValues)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  using namespace Anki;
  
  const std::string & test_json_path = std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_JSON_FILE);
  ifstream configFileStream(test_json_path);

  ASSERT_TRUE(reader.parse(configFileStream, root))
    << "Failed to parse configuration\n"
    << reader.getFormattedErrorMessages();

  using namespace Anki::JsonTools;

  string sval;
  EXPECT_TRUE(GetValueOptional(root, "key", sval));
  EXPECT_EQ(sval, "value");

  EXPECT_FALSE(GetValueOptional(root, "missingKey", sval));
}
