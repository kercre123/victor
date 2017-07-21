/**
 * File: testPtreeTools.cpp
 *
 * Author: damjan
 * Created: 5/22/2013
 *
 * Description: Unit tests for the ptree tools
 * --gtest_filter=PtreeTools*
 * Copyright: Anki, Inc. 2013
 *
 **/

#include "util/helpers/includeGTest.h"
#include "util/ptree/ptreeTools.h"
#include "util/ptree/ptreeKey.h"
#include "util/ptree/ptreeTraverser.h"
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include "util/ptree/includeJSONParser.h"
#include <string>

using namespace std;
using namespace boost;

TEST(PtreeTools, shallowMerge)
{
  ptree first;
  ptree second;

  first.put("1", 1);
  first.put("2", 2);
  first.put("3", 3);
  first.put("4", 4);
  first.put("5.1", 51);
  first.put("5.2", 52);
  first.put("6", 6);
  second.put("2", 10);
  second.put("7", 7);
  second.put("5.1", 71);

  Anki::Util::PtreeTools::ShallowMerge(first, second);
  ASSERT_EQ(first.get<int>("1"), 1);
  ASSERT_EQ(first.get<int>("2"), 10);
  ASSERT_EQ(first.get<int>("6"), 6);
  ASSERT_EQ(first.get<int>("7"), 7);
  ASSERT_EQ(first.get<int>("5.1"), 71);
  optional<int> optionalChild52 = first.get_optional<int>("5.2");
  ASSERT_FALSE(optionalChild52);
}

TEST(PtreeTools, shallowMergeWithCopy)
{
  ptree first;
  ptree second;

  first.put("1", 1);
  first.put("2", 2);
  first.put("3", 3);
  first.put("4", 4);
  first.put("5.1", 51);
  first.put("5.2", 52);
  first.put("6", 6);
  second.put("2", 10);
  second.put("7", 7);
  second.put("5.1", 71);

  ptree result = Anki::Util::PtreeTools::ShallowMergeWithCopy(first, second);
  ASSERT_EQ(result.get<int>("1"), 1);
  ASSERT_EQ(result.get<int>("2"), 10);
  ASSERT_EQ(result.get<int>("6"), 6);
  ASSERT_EQ(result.get<int>("7"), 7);
  ASSERT_EQ(result.get<int>("5.1"), 71);
  optional<int> optionalChild52 = result.get_optional<int>("5.2");
  ASSERT_FALSE(optionalChild52);
}


TEST(PtreeTools, deepMergeLimited)
{
  ptree first;
  ptree second;

  first.put("1", 1);
  first.put("2", 2);
  first.put("3", 3);
  first.put("4", 4);
  first.put("5.1", 51);
  first.put("5.2", 52);
  first.put("6", 6);
  second.put("2", 10);
  second.put("7", 7);
  second.put("5.1", 71);

  string key("5");
  ptree result = Anki::Util::PtreeTools::DeepMergeLimited(first, second, key);
  ASSERT_EQ(result.get<int>("1"), 1);
  ASSERT_EQ(result.get<int>("2"), 10);
  ASSERT_EQ(result.get<int>("6"), 6);
  ASSERT_EQ(result.get<int>("7"), 7);
  ASSERT_EQ(result.get<int>("5.1"), 71);
  optional<int> optionalChild52 = result.get_optional<int>("5.2");
  ASSERT_TRUE(optionalChild52);
  ASSERT_EQ(optionalChild52.get(), 52);
}



TEST(PtreeTools, deepMerge)
{
  ptree first;
  ptree second;

  first.put("1", 1);
  first.put("2", 2);
  first.put("3", 3);
  first.put("4", 4);
  first.put("5.1", 51);
  first.put("5.2", 52);
  first.put("6", 6);
  second.put("2", 10);
  second.put("7", 7);
  second.put("5.1", 71);

  first.put("8.1.1.1", 8111);
  first.put("8.1.1.2", 8112);
  second.put("8.1.2", 812);
  second.put("8.1.1.2", 8115);
  second.put("8.1.1.3", 8113);

  Anki::Util::PtreeTools::FastDeepOverride(first, second);
  EXPECT_EQ(first.get<int>("1"), 1);
  EXPECT_EQ(first.get<int>("2"), 10);
  EXPECT_EQ(first.get<int>("6"), 6);
  EXPECT_EQ(first.get<int>("7"), 7);
  EXPECT_EQ(first.get<int>("5.1"), 71);
  optional<int> optionalChild52 = first.get_optional<int>("5.2");
  EXPECT_TRUE(optionalChild52);
  EXPECT_EQ(optionalChild52.get(), 52);

/*
  write_json(cout, first, true);
  write_json(cout, second, true);
  write_json(cout, result, true);
*/

  ASSERT_EQ(first.get<int>("8.1.1.1"), 8111);
  ASSERT_EQ(first.get<int>("8.1.1.2"), 8115);
  ASSERT_EQ(first.get<int>("8.1.1.3"), 8113);
  ASSERT_EQ(first.get<int>("8.1.2"), 812);

}


TEST(PtreeTools, deepMergeEmpty)
{
  ptree first;
  ptree second;


  second.put("2", 10);
  second.put("7", 7);
  second.put("5.1", 71);

  second.put("8.1.2", 812);
  second.put("8.1.1.2", 8115);
  second.put("8.1.1.3", 8113);

  Anki::Util::PtreeTools::FastDeepOverride(first, second);
  EXPECT_EQ(first.get<int>("2"), 10);
  EXPECT_EQ(first.get<int>("7"), 7);
  EXPECT_EQ(first.get<int>("5.1"), 71);
  optional<int> optionalChild52 = first.get_optional<int>("5.2");
  EXPECT_FALSE(optionalChild52);

/*
  write_json(cout, first, true);
  write_json(cout, second, true);
  write_json(cout, result, true);
*/

  EXPECT_EQ(first.get<int>("8.1.1.2"), 8115);
  EXPECT_EQ(first.get<int>("8.1.1.3"), 8113);
  EXPECT_EQ(first.get<int>("8.1.2"), 812);

}


TEST(PtreeTools, shallowMergeList)
{
  string data("{     "
    "    \"game_state\": {"
    "        \"energy_system\": {"
    "            \"max_energy\" : 10.0"
    "         },"
    "        \"simple_list\": ["
    "        1,2,3,4"
    "        ],"
    "        \"objects_in_list\": ["
    " { \"a\" : \"a\"},"
    " { \"b\" : \"b\"}"
    "        ]"
    "    }"
    "}  ");
  istringstream dataStream;
  dataStream.str(data);
  ptree first;
  read_json(dataStream, first);


  data = "{     "
    "    \"game_state\": {"
    "        \"energy_system\": {"
    "            \"max_energy\" : 10.0"
    "         },"
    "        \"mess\": ["
    "        4,"
    " { \"d\" : \"d\"}"
    "        ],"
    "        \"simple_list\": ["
    "        4,6,7,8"
    "        ],"
    "        \"objects_in_list\": ["
    " { \"a\" : \"a\"},"
    " { \"c\" : \"c\"}"
    "        ]"
    "    }"
    "}  ";
  dataStream.str(data);
  ptree second;
  read_json(dataStream, second);



  ptree result = Anki::Util::PtreeTools::ShallowMergeWithCopy(first, second);

/*
  write_json(cout, first, true);
  write_json(cout, second, true);
  try {
    write_json(cout, result, true);
  }
  catch(...) {}
*/

  int itemsInMess = 0;
  boost::optional<ptree &> messList = result.get_child_optional("game_state.mess");
  if (messList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, messList.get())
    {
      itemsInMess++;
    }
  }
  int itemsInObjectList = 0;
  boost::optional<ptree &> objectListList = result.get_child_optional("game_state.objects_in_list");
  if (objectListList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, objectListList.get())
    {
      itemsInObjectList++;
    }
  }
  int itemsInSimpleList = 0;
  boost::optional<ptree &> simpleList = result.get_child_optional("game_state.simple_list");
  if (simpleList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, simpleList.get())
    {
      itemsInSimpleList++;
    }
  }

  EXPECT_EQ(itemsInMess, 2);
  EXPECT_EQ(itemsInObjectList, 2);
  EXPECT_EQ(itemsInSimpleList, 4);

}

TEST(PtreeTools, deepMergeLimitedList)
{
  string data("{     "
    "    \"game_state\": {"
    "        \"energy_system\": {"
    "            \"max_energy\" : 10.0"
    "         },"
    "        \"simple_list\": ["
    "        1,2,3"
    "        ],"
    "        \"objects_in_list\": ["
    " { \"a\" : \"a\"},"
    " { \"b\" : \"b\"}"
    "        ]"
    "    }"
    "}  ");
  istringstream dataStream;
  dataStream.str(data);
  ptree first;
  read_json(dataStream, first);


  data = "{     "
    "    \"game_state\": {"
    "        \"energy_system\": {"
    "            \"max_energy\" : 10.0"
    "         },"
    "        \"mess\": ["
    "        4,"
    " { \"d\" : \"d\"}"
    "        ],"
    "        \"simple_list\": ["
    "        4,6,7,8"
    "        ],"
    "        \"objects_in_list\": ["
    " { \"a\" : \"a\"},"
    " { \"c\" : \"c\"}"
    "        ]"
    "    }"
    "}  ";
  dataStream.str(data);
  ptree second;
  read_json(dataStream, second);



  ptree result = Anki::Util::PtreeTools::DeepMergeLimited(first, second, "simple_list");

/*
  write_json(cout, first, true);
  write_json(cout, second, true);
  try {
    write_json(cout, result, true);
  }
  catch(...) {}
*/

  int itemsInMess = 0;
  boost::optional<ptree &> messList = result.get_child_optional("game_state.mess");
  if (messList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, messList.get())
    {
      itemsInMess++;
    }
  }
  int itemsInObjectList = 0;
  boost::optional<ptree &> objectListList = result.get_child_optional("game_state.objects_in_list");
  if (objectListList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, objectListList.get())
    {
      itemsInObjectList++;
    }
  }
  int itemsInSimpleList = 0;
  boost::optional<ptree &> simpleList = result.get_child_optional("game_state.simple_list");
  if (simpleList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, simpleList.get())
    {
      itemsInSimpleList++;
    }
  }

  EXPECT_EQ(itemsInMess, 2);
  EXPECT_EQ(itemsInObjectList, 2);
  EXPECT_EQ(itemsInSimpleList, 4);

}


TEST(PtreeTools, deepMergeList)
{
  string data("{     "
    "    \"game_state\": {"
    "        \"energy_system\": {"
    "            \"max_energy\" : 10.0"
    "         },"
    "        \"simple_list\": ["
    "        1,2,3,4"
    "        ],"
    "        \"objects_in_list\": ["
    " { \"a\" : \"a\"},"
    " { \"b\" : \"b\"}"
    "        ]"
    "    }"
    "}  ");
  istringstream dataStream;
  dataStream.str(data);
  ptree first;
  read_json(dataStream, first);


  data = "{     "
    "    \"game_state\": {"
    "        \"energy_system\": {"
    "            \"max_energy\" : 10.0"
    "         },"
    "        \"mess\": ["
    "        4,"
    " { \"d\" : \"d\"}"
    "        ],"
    "        \"simple_list\": ["
    "        4,6,7,8"
    "        ],"
    "        \"objects_in_list\": ["
    " { \"a\" : \"a\"},"
    " { \"c\" : \"c\"}"
    "        ]"
    "    }"
    "}  ";
  dataStream.str(data);
  ptree second;
  read_json(dataStream, second);



  ptree result = first; // don't really need the copy
  Anki::Util::PtreeTools::FastDeepOverride(result, second);

/*
  write_json(cout, first, true);
  write_json(cout, second, true);
  try {
    write_json(cout, result, true);
  }
  catch(...) {}
*/

  int itemsInMess = 0;
  boost::optional<ptree &> messList = result.get_child_optional("game_state.mess");
  if (messList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, messList.get())
    {
      itemsInMess++;
    }
  }
  int itemsInObjectList = 0;
  boost::optional<ptree &> objectListList = result.get_child_optional("game_state.objects_in_list");
  if (objectListList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, objectListList.get())
    {
      itemsInObjectList++;
    }
  }
  int itemsInSimpleList = 0;
  boost::optional<ptree &> simpleList = result.get_child_optional("game_state.simple_list");
  if (simpleList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, simpleList.get())
    {
      itemsInSimpleList++;
    }
  }

  EXPECT_EQ(itemsInMess, 2);
  EXPECT_EQ(itemsInObjectList, 4);
  EXPECT_EQ(itemsInSimpleList, 8);

}


TEST(PtreeTools, deepMergeEmptyList)
{
  string data("{     "
    "    \"game_state\": {"
    "        \"energy_system\": {"
    "            \"max_energy\" : 10.0"
    "         },"
    "        \"mess\": ["
    "        4"
    "        ],"
    "        \"simple_list\": ["
    "        ],"
    "        \"objects_in_list\": ["
    " { \"a\" : \"a\"},"
    " { \"b\" : \"b\"}"
    "        ]"
    "    }"
    "}  ");
  istringstream dataStream;
  dataStream.str(data);
  ptree first;
  read_json(dataStream, first);


  data = "{     "
    "    \"game_state\": {"
    "        \"energy_system\": {"
    "            \"max_energy\" : 10.0"
    "         },"
    "        \"mess\": ["
    " { \"d\" : \"d\"}"
    "        ],"
    "        \"simple_list\": ["
    "        4,6,7,8"
    "        ]"
    "    }"
    "}  ";
  dataStream.str(data);
  ptree second;
  read_json(dataStream, second);



  ptree result = first;
  Anki::Util::PtreeTools::FastDeepOverride(result, second);

/*
  write_json(cout, first, true);
  write_json(cout, second, true);
  try {
    write_json(cout, result, true);
  }
  catch(...) {}
*/

  int itemsInMess = 0;
  boost::optional<ptree &> messList = result.get_child_optional("game_state.mess");
  if (messList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, messList.get())
    {
      itemsInMess++;
    }
  }
  int itemsInObjectList = 0;
  boost::optional<ptree &> objectListList = result.get_child_optional("game_state.objects_in_list");
  if (objectListList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, objectListList.get())
    {
      itemsInObjectList++;
    }
  }
  int itemsInSimpleList = 0;
  boost::optional<ptree &> simpleList = result.get_child_optional("game_state.simple_list");
  if (simpleList) {
    BOOST_FOREACH(__attribute__((unused)) const ptree::value_type &v, simpleList.get())
    {
      itemsInSimpleList++;
    }
  }

  EXPECT_EQ(itemsInMess, 2);
  EXPECT_EQ(itemsInObjectList, 2);
  EXPECT_EQ(itemsInSimpleList, 4);

}

TEST(PtreeTools, ListKeys)
{
  string data(
    "{\n"
    "  \"a\": \"asdf\",\n"
    "  \"lst1\": [\n"
    "    {\"a\": \"2.5\"},\n"
    "    {\"list1_1\": [\"1.0\", \"2.0\", \"3.0\"]}\n"
    "  ],\n"
    "  \"grp\": {\n"
    "    \"lst\": [\"0.0\"]\n"
    "  }\n"
    "}\n"
    );

  istringstream dataStream;
  dataStream.str(data);
  ptree input;
  read_json(dataStream, input);

  Anki::Util::PtreeTools::PtreeKey key;

  key.push_back(pair<string, int>("lst1", 1));
  key.push_back(pair<string, int>("list1_1", 2));

  ptree t = Anki::Util::PtreeTools::Deprecated::GetChild_Deprecated(input, key);
  EXPECT_EQ(3.0, t.get<double>(""));

  Anki::Util::PtreeTools::PtreeKey key2;
  key2.push_back(pair<string, int>("a", -1));
  ptree t2 = Anki::Util::PtreeTools::Deprecated::GetChild_Deprecated(input, key2);
  EXPECT_EQ("asdf", t2.get<string>(""));

  Anki::Util::PtreeTools::PtreeKey key3;
  key3.push_back(pair<string, int>("grp", -1));
  key3.push_back(pair<string, int>("lst", 0));
  ptree t3 = Anki::Util::PtreeTools::Deprecated::GetChild_Deprecated(input, key3);
  EXPECT_EQ(0.0, t3.get<double>(""));
}

TEST(PtreeTools, ListKeysRef)
{
  string data(
    "{\n"
    "  \"a\": \"asdf\",\n"
    "  \"lst1\": [\n"
    "    {\"a\": \"2.5\"},\n"
    "    {\"list1_1\": [\"1.0\", \"2.0\", \"3.0\"]}\n"
    "  ],\n"
    "  \"grp\": {\n"
    "    \"lst\": [\"0.0\"]\n"
    "  }\n"
    "}\n"
    );

  istringstream dataStream;
  dataStream.str(data);
  ptree input;
  read_json(dataStream, input);

  Anki::Util::PtreeTools::PtreeKey key;
  key.push_back(pair<string, int>("lst1", 1));
  key.push_back(pair<string, int>("list1_1", 2));
  
  ptree insert;
  insert.put("newval", "asdf");
  insert.put("newval2", "asdf");

  Anki::Util::PtreeTools::Deprecated::GetChild_Deprecated(input, key) = insert;

  Anki::Util::PtreeTools::PtreeKey key2;
  key2.push_back(pair<string, int>("lst1", 0));

  Anki::Util::PtreeTools::Deprecated::GetChild_Deprecated(input, key2).put("a", 3.5);

  string data2(
    "{\n"
    "  \"a\": \"asdf\",\n"
    "  \"lst1\": [\n"
    "    {\"a\": \"3.5\"},\n"
    "    {\"list1_1\": [\"1.0\", \"2.0\", {\n"
    "      \"newval\": \"asdf\",\n"
    "      \"newval2\": \"asdf\"\n"
    "    }]}\n"
    "  ],\n"
    "  \"grp\": {\n"
    "    \"lst\": [\"0.0\"]\n"
    "  }\n"
    "}\n"
    );
  istringstream dataStream2;
  dataStream2.str(data2);
  ptree output;
  read_json(dataStream2, output);
  
  EXPECT_EQ(input, output);
}

TEST(PtreeTools, ParseKey)
{
  using namespace Anki::Util::PtreeTools;

  string data(
    "{\n"
    "  \"a\": \"asdf\",\n"
    "  \"x\": {\n"
    "    \"lst1\": [\n"
    "      {\"a\": \"2\"},\n"
    "      {\"list1_1\": [\"1\", \"3\", \"4\"]}\n"
    "    ]\n"
    "  },\n"
    "  \"grp\": {\n"
    "    \"lst\": [\"0.0\"]\n"
    "  },\n"
    "  \"b\": {\n"
    "    \"c\": 22\n"
    "  }\n"
    "}\n"
    );

  istringstream dataStream;
  dataStream.str(data);
  ptree tree;
  read_json(dataStream, tree);

  PtreeKey key0("b.c");
  EXPECT_EQ(Deprecated::GetChild_Deprecated(tree, key0).get<int>(""), 22);

  PtreeKey key1("x.lst1[0]");
  EXPECT_EQ(Deprecated::GetChild_Deprecated(tree, key1).get<int>("a"), 2);

  PtreeKey key2("x.lst1[0].a");
  EXPECT_EQ(Deprecated::GetChild_Deprecated(tree, key2).get<int>(""), 2);

  PtreeKey key3("x.lst1[1].list1_1[2]");
  EXPECT_EQ(Deprecated::GetChild_Deprecated(tree, key3).get<int>(""), 4);

}

TEST(PtreeTools, Traversal)
{
  string data(
    "{\n"
    "  \"val\": \"0\",\n"
    "  \"lst\": [\n"
    "    { \"lst\": [{\"val\": \"1\"},{\"val\": \"2\"}]},\n"
    "    {\"val\": \"3\"}\n"
    "  ],\n"
    "  \"sub\": {\n"
    "    \"val\": \"4\",\n"
    "    \"lst\":[\n"
    "      {\"val\": \"5\"},\n"
    "      {\"val\": \"6\"},\n"
    "      {\"notval\": \"33\", \"val\": \"7\"}\n"
    "    ]\n"
    "  }\n"
    "}\n"
    );
  istringstream dataStream;
  dataStream.str(data);
  ptree input;
  read_json(dataStream, input);

  // we should see every number in 0:8
  bool seen[8];
  for(unsigned int i=0; i<8; ++i)
    seen[i] = false;

  Anki::Util::PtreeTools::PtreeTraverser trav(input);
  EXPECT_FALSE(trav.done());

  unsigned int numSteps = 0;
  while(!trav.done()) {
    numSteps++;

    // cout<<"traversal "<<numSteps<<"\n\n";
    // Anki::Util::PtreeTools::PrintJson((trav.topTree());
 
    ASSERT_LT(numSteps, 30);

    EXPECT_EQ(trav.topTree(), Anki::Util::PtreeTools::Deprecated::GetChild_Deprecated(input,trav.topKey())) << "tree mismatch!";

    boost::optional<unsigned int> idx = trav.topTree().get_optional<unsigned int>("val");
    if(idx) {
      ASSERT_LT(*idx, 8);
      EXPECT_FALSE(seen[*idx]);
      seen[*idx] = true;
    }

    trav.next();
  }

  for(unsigned int i=0; i<8; ++i)
    EXPECT_TRUE(seen[i]);

  EXPECT_EQ(numSteps, 12);
}

TEST(PtreeTools, PreprocessSimpleTree)
{
  string inData(
    "{\n"
    "  \"grp1\": {\n"
    "    \"val\": \"3.7\",\n"
    "    \"id\": \"g1\",\n"
    "    \"subgrp1\": {\n"
    "      \"val\": \"1.5\",\n"
    "      \"val2\": \"asdf\"\n"
    "    }\n"
    "  },\n"
    "  \"outer_grp\": {\n"
    "    \"sub_group\": {\n"
    "      \"extends\": \"g1\",\n"
    "      \"val\": \"1.0\"\n"
    "    }\n"
    "  }\n"
    "}\n"
    );
  istringstream inDataStream;
  inDataStream.str(inData);
  ptree input;
  read_json(inDataStream, input);

  string outData(
    "{\n"
    "  \"grp1\": {\n"
    "    \"val\": \"3.7\",\n"
    "    \"id\": \"g1\",\n"
    "    \"subgrp1\": {\n"
    "      \"val\": \"1.5\",\n"
    "      \"val2\": \"asdf\"\n"
    "    }\n"
    "  },\n"
    "  \"outer_grp\": {\n"
    "    \"sub_group\": {\n"
    "      \"val\": \"1.0\",\n"
    "      \"subgrp1\": {\n"
    "        \"val\": \"1.5\",\n"
    "        \"val2\": \"asdf\"\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n"
    );
  istringstream outDataStream;
  outDataStream.str(outData);
  ptree output;
  read_json(outDataStream, output);

  // cout<<"input before processing:\n";
  // Anki::Util::PtreeTools::PrintJson((input);

  Anki::Util::PtreeTools::FastPreprocess(input);
  input.erase("preprocessed");

  // cout<<"result:\n";
  // Anki::Util::PtreeTools::PrintJson((processed);

  EXPECT_EQ(input, output);
}

TEST(PtreeTools, PreprocessComplexTree)
{
  string inData(
    "{\n"
    "  \"grp1\": {\n"
    "    \"val\": \"3.7\",\n"
    "    \"id\": \"g1\",\n"
    "    \"subgrp1\": {\n"
    "      \"val\": \"1.5\",\n"
    "      \"extends\": \"g3\"\n"
    "    },\n"
    "    \"subgrp2\": {\n"
    "      \"val\": \"1.5\",\n"
    "      \"extends\": \"g5\"\n"
    "    }\n"
    "  },\n"
    "  \"outer_grp\": {\n"
    "    \"sub_group\": {\n"
    "      \"extends\": \"g1\",\n"
    "      \"val\": \"1.0\"\n"
    "    }\n"
    "  },\n"
    "  \"outer_2\": {\n"
    "    \"id\": \"g3\",\n"
    "    \"extends\": \"g4\",\n"
    "    \"val\": \"3.0\",\n"
    "    \"sub\": {\n"
    "      \"extends\": \"g5\"\n"
    "    }\n"
    "  },\n"
    "  \"defs\": [\n"
    "  {\n"
    "    \"id\": \"g4\",\n"
    "    \"val4\": \"4.0\",\n"
    "    \"val\": \"0.4\"\n"
    "  },\n"
    "  {\n"
    "    \"id\": \"g5\",\n"
    "    \"strval\": \"string\"\n"
    "  }\n"
    "  ],\n"
    "  \"dot\": {\n"
    "    \"extends\": \"g5\"\n"
    "  }\n"
    "}\n"
    );
  istringstream inDataStream;
  inDataStream.str(inData);
  ptree input;
  read_json(inDataStream, input);

  string outData(
    "{\n"
    "  \"grp1\": {\n"
    "    \"val\": \"3.7\",\n"
    "    \"id\": \"g1\",\n"
    "      \"subgrp1\": {\n"
    "      \"val4\": \"4.0\",\n"
    "      \"val\": \"1.5\",\n"
    "      \"sub\": {\n"
    "        \"strval\": \"string\"\n"
    "      }\n"
    "    },\n"
    "    \"subgrp2\": {\n"
    "      \"strval\": \"string\",\n"
    "      \"val\": \"1.5\"\n"
    "    }\n"
    "  },\n"
    "  \"outer_grp\": {\n"
    "    \"sub_group\": {\n"
    "      \"val\": \"1.0\",\n"
    "        \"subgrp1\": {\n"
    "        \"val4\": \"4.0\",\n"
    "        \"val\": \"1.5\",\n"
    "        \"sub\": {\n"
    "          \"strval\": \"string\"\n"
    "        }\n"
    "      },\n"
    "      \"subgrp2\": {\n"
    "        \"strval\": \"string\",\n"
    "        \"val\": \"1.5\"\n"
    "      }\n"
    "    }\n"
    "  },\n"
    "  \"outer_2\": {\n"
    "    \"val4\": \"4.0\",\n"
    "    \"val\": \"3.0\",\n"
    "    \"id\": \"g3\",\n"
    "    \"sub\": {\n"
    "      \"strval\": \"string\"\n"
    "    }\n"
    "  },\n"
    "  \"defs\": [\n"
    "  {\n"
    "    \"id\": \"g4\",\n"
    "    \"val4\": \"4.0\",\n"
    "    \"val\": \"0.4\"\n"
    "  },\n"
    "  {\n"
    "    \"id\": \"g5\",\n"
    "    \"strval\": \"string\"\n"
    "  }\n"
    "  ],\n"
    "  \"dot\": {\n"
    "    \"strval\": \"string\"\n"
    "  }\n"
    "}\n"
    );
  istringstream outDataStream;
  outDataStream.str(outData);
  ptree output;
  read_json(outDataStream, output);

  // cout<<"input before processing:\n";
  // Anki::Util::PtreeTools::PrintJson((input);

  Anki::Util::PtreeTools::FastPreprocess(input);
  input.erase("preprocessed");

  // cout<<"result:\n";
  // Anki::Util::PtreeTools::PrintJson((processed);

  // cout<<"\n\nexpected result:\n";
  // Anki::Util::PtreeTools::PrintJson((output);

  EXPECT_EQ(input, output);

  // Now make sure nothing breaks if we pre-process an extra time
  Anki::Util::PtreeTools::FastPreprocess(input);
  input.erase("preprocessed");
  EXPECT_EQ(input, output);
}
