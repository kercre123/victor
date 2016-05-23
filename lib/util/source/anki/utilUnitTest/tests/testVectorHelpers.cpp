/**
 * File: testVectorHelpers
 *
 * Author: Mark Wesley
 * Created: 12/03/15
 *
 * Description: Unit tests for vector helpers
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=VectorHelpers*
 **/


#include "util/helpers/includeGTest.h"
#include "util/helpers/vectorHelpers.h"


TEST(VectorHelpers, RemoveSwapIndex)
{
  std::vector<int> testInts = {1, 2, 2, 3, 4};

  EXPECT_EQ(testInts.size(), 5);
  EXPECT_EQ(testInts, std::vector<int>({1, 2, 2, 3, 4}));

  // remove at start
  remove_swap(testInts, 0);
  
  EXPECT_EQ(testInts.size(), 4);
  EXPECT_EQ(testInts, std::vector<int>({4, 2, 2, 3}));
  
  // remove in middle
  remove_swap(testInts, 1);

  EXPECT_EQ(testInts.size(), 3);
  EXPECT_EQ(testInts, std::vector<int>({4, 3, 2}));

  // remove at end
  remove_swap(testInts, 2);
  
  EXPECT_EQ(testInts.size(), 2);
  EXPECT_EQ(testInts, std::vector<int>({4, 3}));

  // remove penultimate
  remove_swap(testInts, 0);
  
  EXPECT_EQ(testInts.size(), 1);
  EXPECT_EQ(testInts, std::vector<int>({3}));
  
  // remove last
  remove_swap(testInts, 0);
  
  EXPECT_EQ(testInts.size(), 0);
  EXPECT_EQ(testInts, std::vector<int>({}));
}


TEST(VectorHelpers, RemoveSwapItem)
{
  std::vector<int> testInts = {1, 2, 3, 4, 4, 4, 4, 5, 6, 7, 8};
  
  EXPECT_EQ(testInts.size(), 11);
  EXPECT_EQ(testInts, std::vector<int>({1, 2, 3, 4, 4, 4, 4, 5, 6, 7, 8}));
  
  // remove 1 in middle
  remove_swap_item(testInts, 3);
  
  EXPECT_EQ(testInts.size(), 10);
  EXPECT_EQ(testInts, std::vector<int>({1, 2, 8, 4, 4, 4, 4, 5, 6, 7}));
  
  // remove >1 in middle
  remove_swap_item(testInts, 4);
  
  EXPECT_EQ(testInts.size(), 6);
  EXPECT_EQ(testInts, std::vector<int>({1, 2, 8, 7, 5, 6}));
  
  testInts.push_back(6);
  
  EXPECT_EQ(testInts.size(), 7);
  EXPECT_EQ(testInts, std::vector<int>({1, 2, 8, 7, 5, 6, 6}));
  
  // remove >1 at end
  remove_swap_item(testInts, 6);
  
  EXPECT_EQ(testInts.size(), 5);
  EXPECT_EQ(testInts, std::vector<int>({1, 2, 8, 7, 5}));
  
  // remove 1 at end
  remove_swap_item(testInts, 5);
  
  EXPECT_EQ(testInts.size(), 4);
  EXPECT_EQ(testInts, std::vector<int>({1, 2, 8, 7}));
  
  // remove 1 at start
  remove_swap_item(testInts, 1);
  
  EXPECT_EQ(testInts.size(), 3);
  EXPECT_EQ(testInts, std::vector<int>({7, 2, 8}));
  
  // remove >1 at start
  testInts = std::vector<int>({1, 1, 1, 2, 3, 4});
  remove_swap_item(testInts, 1);
  
  EXPECT_EQ(testInts.size(), 3);
  EXPECT_EQ(testInts, std::vector<int>({2, 3, 4}));
  
  // remove not-in-container
  remove_swap_item(testInts, 5);
  
  EXPECT_EQ(testInts.size(), 3);
  EXPECT_EQ(testInts, std::vector<int>({2, 3, 4}));
}

