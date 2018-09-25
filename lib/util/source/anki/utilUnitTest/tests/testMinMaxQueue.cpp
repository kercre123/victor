/**
 * File: testMinMaxQueue
 *
 * Author: Matt Michini
 * Created: 09/18/2018
 *
 * Description: Unit tests for MinMaxQueue
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=MinMaxQueue*
 **/

#define private public
#define protected public

#include "util/container/minMaxQueue.h"
#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"

using namespace Anki;
using namespace Util;

template <typename T>
void AssertValid(const MinMaxQueue<T> queue)
{
  if (queue.empty()) {
    ASSERT_TRUE(queue._data.empty());
    ASSERT_TRUE(queue._minTracker.empty());
    ASSERT_TRUE(queue._maxTracker.empty());
    return;
  }
  
  ASSERT_FALSE(queue._data.empty());
  ASSERT_FALSE(queue._minTracker.empty());
  ASSERT_FALSE(queue._maxTracker.empty());
  
  ASSERT_GE(queue.max(), queue.min());
  
  // 'manually' verify that the min and max are correct
  const auto actualMin = *std::min_element(queue._data.begin(), queue._data.end());
  ASSERT_EQ(actualMin, queue.min());
  
  const auto actualMax = *std::max_element(queue._data.begin(), queue._data.end());
  ASSERT_EQ(actualMax, queue.max());
  
  // Verify that maxTracker deque is monotonic decreasing
  ASSERT_TRUE(std::is_sorted(queue._maxTracker.rbegin(), queue._maxTracker.rend()));
  
  // Verify that minTracker deque is monotonic increasing
  ASSERT_TRUE(std::is_sorted(queue._minTracker.begin(), queue._minTracker.end()));
}


TEST(MinMaxQueue, SingleElement)
{
  MinMaxQueue<int> queue;
  const int testVal = 99;
  queue.push(testVal);
  ASSERT_FALSE(queue.empty());
  ASSERT_EQ(queue.back(), testVal);
  ASSERT_EQ(queue.front(), testVal);
  ASSERT_EQ(queue.min(), testVal);
  ASSERT_EQ(queue.max(), testVal);
  AssertValid(queue);
  
  queue.pop();
  ASSERT_TRUE(queue.empty());
  AssertValid(queue);
  
  queue.push(testVal);
  AssertValid(queue);
  
  queue.clear();
  ASSERT_TRUE(queue.empty());
  AssertValid(queue);
}


TEST(MinMaxQueue, Constant)
{
  // Add the same element a bunch of times
  MinMaxQueue<int> queue;
  
  const int testVal = 99;
  const int nElements = 10;
  for (int i=0 ; i < nElements ; i++) {
    queue.push(testVal);
    AssertValid(queue);
    ASSERT_EQ(testVal, queue.min());
    ASSERT_EQ(testVal, queue.max());
  }
  
  // Pop all the elements
  while(!queue.empty()) {
    ASSERT_EQ(testVal, queue.min());
    ASSERT_EQ(testVal, queue.max());
    queue.pop();
    AssertValid(queue);
  }
}


TEST(MinMaxQueue, Ascending)
{
  MinMaxQueue<int> queue;
  
  std::vector<int> testVals = {-3, -2, -1, 0, 1, 2, 3};

  for (const auto& val : testVals) {
    queue.push(val);
    AssertValid(queue);
    ASSERT_EQ(queue.max(), val);
    ASSERT_EQ(queue.min(), testVals.front());
  }
  
  while(!queue.empty()) {
    ASSERT_EQ(queue.max(), testVals.back());
    ASSERT_EQ(queue.min(), queue.front());
    queue.pop();
    AssertValid(queue);
  }
}


TEST(MinMaxQueue, Descending)
{
  MinMaxQueue<int> queue;
  
  std::vector<int> testVals = {3, 2, 1, 0, -1, -2, -3};
  
  for (const auto& val : testVals) {
    queue.push(val);
    AssertValid(queue);
    ASSERT_EQ(queue.min(), val);
    ASSERT_EQ(queue.max(), testVals.front());
  }
  
  while(!queue.empty()) {
    ASSERT_EQ(queue.min(), testVals.back());
    ASSERT_EQ(queue.max(), queue.front());
    queue.pop();
    AssertValid(queue);
  }
}


TEST(MinMaxQueue, RandomElements)
{
  MinMaxQueue<int> queue;
  
  RandomGenerator rng(123);
  const int maxVal = 1000;
  const int nElements = 25;
  for (int i=0 ; i < nElements ; i++) {
    queue.push(rng.RandInt(maxVal));
    AssertValid(queue);
  }

  // Pop roughly half the elements
  while(queue.size() > nElements/2) {
    queue.pop();
    AssertValid(queue);
  }
  
  // Add another nElements elements
  for (int i=0 ; i < nElements ; i++) {
    queue.push(rng.RandInt(maxVal));
    AssertValid(queue);
  }
  
  // Pop all the elements
  while(!queue.empty()) {
    queue.pop();
    AssertValid(queue);
  }
}


TEST(MinMaxQueue, RepeatedElements)
{
  MinMaxQueue<int> queue;
  
  // Add random elements, but add a random number of repeated elements as well
  RandomGenerator rng(123);
  const int maxVal = 1000;
  const int nElements = 25;
  for (int i=0 ; i < nElements ; i++) {
    const int nRepeatedElements = rng.RandInt(4);
    for (int j=0 ; j < nRepeatedElements ; j++) {
      queue.push(rng.RandInt(maxVal));
      AssertValid(queue);
    }
  }
  
  // Pop all the elements
  while(!queue.empty()) {
    queue.pop();
    AssertValid(queue);
  }
}
