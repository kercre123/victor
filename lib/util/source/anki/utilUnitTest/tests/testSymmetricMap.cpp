/**
 * File: testSymmetricMap.cpp
 *
 * Author: Andrew Stein
 * Created: 10/26/2018
 *
 * Description: Unit tests for SymmetricMap<A,B>
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=SymmetricMap*
 **/


#include "util/container/symmetricMap.h"
#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"


TEST(SymmetricMap, DifferentTypes)
{
  using namespace Anki;
  
  Util::SymmetricMap<int,std::string> LUT{
    {1, "one"},
    {2, "two"},
    {3, "three"},
    {2, "two_too"},
    {4, "three"},
    {33, "three"},
  };
  
  std::set<std::string> strs;
  LUT.Find(1, strs);
  ASSERT_EQ(1, strs.size());
  
  std::set<int> ints;
  LUT.Find("one", ints);
  ASSERT_EQ(1, ints.size());
  ASSERT_EQ(std::set<int>({1}), ints);
  
  strs.clear();
  LUT.Find(2, strs);
  ASSERT_EQ(2, strs.size());
  ASSERT_EQ(std::set<std::string>({"two", "two_too"}), strs);
  
  ints.clear();
  LUT.Find("three", ints);
  ASSERT_EQ(3, ints.size());
  ASSERT_EQ(std::set<int>({3,4,33}), ints);
  
  std::set<int> intKeys;
  LUT.GetKeys(intKeys);
  ASSERT_EQ(5, intKeys.size());
  
  std::set<std::string> strKeys;
  LUT.GetKeys(strKeys);
  ASSERT_EQ(4, strKeys.size());
  
  strs.clear();
  LUT.Find(5, strs);
  ASSERT_TRUE(strs.empty());
  
  ints.clear();
  LUT.Find("five", ints);
  ASSERT_TRUE(ints.empty());
  
  LUT.Erase(1);
  LUT.Find(1, strs);
  ASSERT_TRUE(strs.empty());
  LUT.Find("one", ints);
  ASSERT_TRUE(ints.empty());
  
  LUT.Erase("two");
  strKeys.clear();
  LUT.GetKeys(strKeys);
  ASSERT_EQ(2, strKeys.size()); 
  LUT.Find("two", ints);
  ASSERT_TRUE(ints.empty());
  LUT.Find(2, strs);
  ASSERT_EQ(std::set<std::string>({"two_too"}), strs);
  
  strs.clear();
  LUT.Insert(1, "one_new");
  LUT.Insert(1, "first");
  LUT.Find(1, strs);
  ASSERT_EQ(std::set<std::string>({"first", "one_new"}), strs);

  strs.clear();
  LUT.Erase(1);
  LUT.Find(1, strs);
  ASSERT_TRUE(strs.empty());
  
  LUT.Erase(33, "three");
  intKeys.clear();
  LUT.GetKeys(intKeys);
  ASSERT_EQ(0, intKeys.count(33));
  ASSERT_GT(intKeys.count(3), 0);
  
  strKeys.clear();
  LUT.GetKeys(strKeys);
  ASSERT_GT(strKeys.count("three"), 0);
  
}

TEST(SymmetricMap, SameTypes)
{
  using namespace Anki;

  Util::SymmetricMap<int,int> LUT{
    {1,1},
    {2,4},
    {3,8},
    {8,16},
    {3,32},
    {2,6},
  };
  
  std::set<int> keys;
  LUT.GetKeys(keys);
  ASSERT_EQ(8, keys.size());
  
  const std::set<int> expectedKeys{1,2,3,4,6,8,16,32};
  ASSERT_EQ(expectedKeys, keys);
  
  std::set<int> vals;
  LUT.Find(1,vals);
  ASSERT_EQ(std::set<int>({1}), vals);

  vals.clear();
  LUT.Find(2,vals);
  ASSERT_EQ(std::set<int>({4,6}), vals);
  
  vals.clear();
  LUT.Find(4,vals);
  ASSERT_EQ(std::set<int>({2}), vals);

  vals.clear();
  LUT.Find(3,vals);
  ASSERT_EQ(std::set<int>({8,32}), vals);
  
  vals.clear();
  LUT.Insert(9,81);
  LUT.Find(81, vals);
  ASSERT_EQ(std::set<int>({9}), vals);
  
  vals.clear();
  LUT.Erase(1);
  LUT.Find(1, vals);
  ASSERT_TRUE(vals.empty());
  
  LUT.Erase(2);
  LUT.Find(2, vals);
  ASSERT_TRUE(vals.empty());
  LUT.Find(4, vals);
  ASSERT_TRUE(vals.empty());
  
  LUT.Erase(8);
  LUT.Find(3, vals);
  ASSERT_EQ(1, vals.size());
  ASSERT_EQ(std::set<int>({32}), vals);
  vals.clear();
  LUT.Find(8, vals);
  ASSERT_TRUE(vals.empty());
  
  LUT.Erase(3,32);
  keys.clear();
  LUT.GetKeys(keys);
  ASSERT_EQ(std::set<int>({9,81}), keys);

  LUT.Insert(14,92);
  LUT.Insert(1,1);
  LUT.Erase(81,9);
  keys.clear();
  LUT.GetKeys(keys);
  ASSERT_EQ(std::set<int>({1,14,92}), keys);
  
}
