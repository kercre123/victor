/**
 * File: testConfigTreeJsonCpp.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-01-06
 *
 * Description: unit tests for parsing a json file, just making sure the library works reasonably
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include <fstream>
#include "gtest/gtest.h"

#include "json/json.h"

using namespace std;

GTEST_TEST(TestConfigTreeJsonCpp, JsonCppWorks)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  ifstream configFileStream("../coretech/common/basestation/config/test.json");

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
