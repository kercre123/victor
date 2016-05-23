//
//  testPTreeToJSON.cpp
//  util
//
//  Created by Mark Pauley on 6/17/15.
//
//

#include "util/helpers/includeGTest.h"
#include "util/ptree/ptreeToJSON.h"
#include "util/ptree/includePtree.h"
#include <json/json.h>
#include <string>


using ptree = boost::property_tree::ptree;

TEST(PTreeToJSON, simpleObject)
{
  ptree test_ptree;
  Json::Value test_json;
  bool result = false;
  
  test_ptree.put("1", 1);
  test_ptree.put("2", 2);
  test_ptree.put("3", 3);
  test_ptree.put("4", "a");
  test_ptree.put("5_1", 51.2);
  test_ptree.put("5_2", -51.2);
  test_ptree.put("5_3", -52);
  test_ptree.put("6", true);
  
  result = Anki::Util::PTreeToJSON(test_ptree, test_json);
  ASSERT_TRUE(result);
  
  ASSERT_EQ(test_json.type(), Json::objectValue);
  
  ASSERT_EQ(Json::uintValue, test_json["1"].type());
  ASSERT_EQ(test_json["1"].asUInt(), 1);
  
  ASSERT_EQ(Json::uintValue, test_json["2"].type());
  ASSERT_EQ(test_json["2"].asUInt(), 2);

  ASSERT_EQ(Json::uintValue, test_json["3"].type());
  ASSERT_EQ(test_json["3"].asUInt(), 3);
  
  ASSERT_EQ(Json::stringValue, test_json["4"].type());
  ASSERT_EQ(test_json["4"].asString(), "a");
  
  ASSERT_EQ(Json::realValue, test_json["5_1"].type());
  ASSERT_DOUBLE_EQ(test_json["5_1"].asDouble(), 51.2);
  
  ASSERT_EQ(Json::realValue, test_json["5_2"].type());
  ASSERT_DOUBLE_EQ(test_json["5_2"].asDouble(), -51.2);
  
  ASSERT_EQ(Json::intValue, test_json["5_3"].type());
  ASSERT_EQ(test_json["5_3"].asInt(), -52);
  
  ASSERT_EQ(Json::booleanValue, test_json["6"].type());
  ASSERT_EQ(test_json["6"].asBool(), true);
}

TEST(PTreeToJSON, simpleArray)
{
  ptree test_ptree;
  Json::Value test_json;
  ptree child_ptree;
  bool result = false;
  
  child_ptree.put_value(1);
  test_ptree.push_back(std::make_pair("", child_ptree));
  child_ptree.put_value(2);
  test_ptree.push_back(std::make_pair("", child_ptree));
  child_ptree.put_value(3);
  test_ptree.push_back(std::make_pair("", child_ptree));
  child_ptree.put_value("a");
  test_ptree.push_back(std::make_pair("", child_ptree));
  child_ptree.put_value(51.2);
  test_ptree.push_back(std::make_pair("", child_ptree));
  child_ptree.put_value(-51.2);
  test_ptree.push_back(std::make_pair("", child_ptree));
  child_ptree.put_value(-52);
  test_ptree.push_back(std::make_pair("", child_ptree));
  child_ptree.put_value(true);
  test_ptree.push_back(std::make_pair("", child_ptree));
  
  result = Anki::Util::PTreeToJSON(test_ptree, test_json);
  ASSERT_TRUE(result);
  
  ASSERT_EQ(Json::arrayValue, test_json.type());
  ASSERT_EQ(8, test_json.size());
  
  ASSERT_EQ(Json::uintValue, test_json.get((int)0, Json::Value::null).type());
  ASSERT_EQ(1, test_json.get((int)0, Json::Value::null).asUInt());
  
  ASSERT_EQ(Json::uintValue, test_json.get((int)1, Json::Value::null).type());
  ASSERT_EQ(2, test_json.get((int)1, Json::Value::null).asUInt());
  
  ASSERT_EQ(Json::uintValue, test_json.get((int)2, Json::Value::null).type());
  ASSERT_EQ(3, test_json.get((int)2, Json::Value::null).asUInt());
  
  ASSERT_EQ(Json::stringValue, test_json.get((int)3, Json::Value::null).type());
  ASSERT_EQ("a", test_json.get((int)3, Json::Value::null).asString());
  
  ASSERT_EQ(Json::realValue, test_json.get((int)4, Json::Value::null).type());
  ASSERT_DOUBLE_EQ(51.2, test_json.get((int)4, Json::Value::null).asDouble());
  
  ASSERT_EQ(Json::realValue, test_json.get((int)5, Json::Value::null).type());
  ASSERT_DOUBLE_EQ(-51.2, test_json.get((int)5, Json::Value::null).asDouble());
  
  ASSERT_EQ(Json::intValue, test_json.get((int)6, Json::Value::null).type());
  ASSERT_EQ(-52, test_json.get((int)6, Json::Value::null).asInt());
  
  ASSERT_EQ(Json::booleanValue, test_json.get((int)7, Json::Value::null).type());
  ASSERT_EQ(true, test_json.get((int)7, Json::Value::null).asBool());
}

//TODO: aggregate Object (object with objects / arrays as children)

//TODO: aggregate Array (array with objects / arrays as children)


TEST(JSONToPTree, simpleObject)
{
  std::string data(
    "{  "
    "  \"1\": 1,"
    "  \"2\": -51,"
    "  \"3\": 51.2,"
    "  \"4\": \"a\","
    "  \"5\": true,"
    "}  ");
  
  Json::Value test_json;
  boost::property_tree::ptree test_ptree;
  
  Json::Reader::Reader().parse(data, test_json);

  ASSERT_EQ(5, test_json.size());
  
  bool result = Anki::Util::JSONToPTree(test_json, test_ptree);
  ASSERT_TRUE(result);
  
  ASSERT_EQ(5, test_ptree.size());
  ASSERT_EQ(1, test_ptree.get<unsigned int>("1"));
  ASSERT_EQ(-51, test_ptree.get<int>("2"));
  ASSERT_EQ(51.2, test_ptree.get<double>("3"));
  ASSERT_EQ(std::string("a"), test_ptree.get<std::string>("4"));
  ASSERT_EQ(true, test_ptree.get<bool>("5"));
}

TEST(JSONToPTree, simpleArray)
{
  std::string data(
    "[ 1, -51, 51.2, \"a\", true ]"
  );

  Json::Value test_json;
  boost::property_tree::ptree test_ptree;
  
  Json::Reader::Reader().parse(data, test_json);
  
  ASSERT_EQ(5, test_json.size());
  
  bool result = Anki::Util::JSONToPTree(test_json, test_ptree);
  ASSERT_TRUE(result);
  
  ASSERT_EQ(5, test_ptree.size());
  
  // ptree is stupid: it translates array elements into children of empty nodes.
  //  so there is no simple way to get the kth element.
  auto array_iter = test_ptree.begin();
  ASSERT_NE(test_ptree.end(), array_iter);
  // [0]
  ASSERT_EQ("", array_iter->first);
  ASSERT_EQ(1, array_iter->second.get_value<unsigned int>());
  ++array_iter;
  // [1]
  ASSERT_EQ("", array_iter->first);
  ASSERT_EQ(-51, array_iter->second.get_value<int>());
  ++array_iter;
  // [2]
  ASSERT_EQ("", array_iter->first);
  ASSERT_EQ(51.2, array_iter->second.get_value<double>());
  ++array_iter;
  // [3]
  ASSERT_EQ("", array_iter->first);
  ASSERT_EQ(std::string("a"), array_iter->second.get_value<std::string>());
  ++array_iter;
  // [4]
  ASSERT_EQ("", array_iter->first);
  ASSERT_EQ(true, array_iter->second.get_value<bool>());
  ++array_iter;
  // [end]
  ASSERT_EQ(test_ptree.end(), array_iter);
}

// TODO: aggregate object ( with objects / arrays as children )
// TODO: aggregate array ( with objects / arrays as children )
