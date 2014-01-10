/**
 * File: testConfigTreeJsonCpp.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-01-06
 *
 * Description: unit tests for parsing a json file using the Anki ConfigTree interface
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include <fstream>
#include "gtest/gtest.h"

#include "anki/common/basestation/configTree.h"
#include "json/json.h"

using namespace std;
using namespace Anki;

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

GTEST_TEST(TestConfigTreeJsonCpp, ConfigTreeInterface)
{
  ConfigTree *tree = new ConfigTree;

  EXPECT_TRUE(tree->ReadJson("../coretech/common/basestation/config/test.json"));

  EXPECT_EQ(tree->GetNumberOfChildren(), 4);

  string str;
  EXPECT_TRUE(tree->GetValueOptional("key", str));
  EXPECT_EQ(str, "value");

  EXPECT_EQ((*tree)["key"].AsString(), "value");
  EXPECT_EQ(tree->GetValue("key", "wrong"), "value");
  EXPECT_EQ(tree->GetValue("missing", "missing"), "missing");

  float num;
  EXPECT_FLOAT_EQ((*tree)["floatKey"].AsFloat(), 3.1415);

  ConfigTree list1 = (*tree)["list"];
  EXPECT_EQ(list1.GetNumberOfChildren(), 4);
  EXPECT_EQ(list1[0].AsInt(), 1);
  EXPECT_EQ(list1[1].AsInt(), 2);
  EXPECT_EQ(list1[2].AsInt(), 3);
  EXPECT_EQ(list1[3].AsInt(), 4);

  EXPECT_EQ((*tree)["dict"]["list"][1]["bv"].AsBool(), true);

  vector<string> keys = (*tree)["dict"]["imporatantKeys"].GetKeys();
  ASSERT_EQ(keys.size(), 2);

  if(keys[0] == "key1") {
    EXPECT_EQ((*tree)["dict"]["imporatantKeys"][keys[0]].AsInt(), 5678);

    EXPECT_EQ(keys[1], "key2");
    EXPECT_EQ((*tree)["dict"]["imporatantKeys"][keys[1]]["thing"].AsString(), "stuff");
  }
  else if(keys[0] == "key2") {
    EXPECT_EQ((*tree)["dict"]["imporatantKeys"][keys[0]]["thing"].AsString(), "stuff");

    EXPECT_EQ(keys[1], "key1");
    EXPECT_EQ((*tree)["dict"]["imporatantKeys"][keys[1]].AsInt(), 5678);
  }
  else {
    FAIL() << "key 0 was "<<keys[0];
  }

  delete tree;
}
