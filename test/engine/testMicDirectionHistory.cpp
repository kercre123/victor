/**
 * File: testMicDirectionHistory.cpp
 *
 * Author:  Lee Crippen
 * Created: 11/15/2017
 *
 * Description: Unit/regression tests for Mic Direction History tracking
 *
 * Copyright: Anki, Inc. 2017
 *
 * --gtest_filter=MicDirectionHistory*
 **/

#include "engine/components/mics/micDirectionHistory.h"

#include "util/logging/logging.h"

#include "gtest/gtest.h"

#include <functional>

#define LOG_CHANNEL "MicDirectionHistory"

using namespace Anki;
using namespace Anki::Vector;

//
// Use this to enable/disable all tests in the suite
#define SUITE MicDirectionHistory

//----------------------------------------------------------------------------------------------------------------------
// Enter description here
//----------------------------------------------------------------------------------------------------------------------

TEST(SUITE, MicDirectionHistory_RecentDirection_simple)
{
  const auto directionUnknown = kMicDirectionUnknown;
  const auto directionFirst = kFirstMicDirectionIndex;
  const auto directionSecond = directionFirst + 1;
  const auto maxTimeLength_ms = std::numeric_limits<uint32_t>::max();

  // Some simple sets of data
  MicDirectionHistory newHistory{};

  // Verify empty history comes back as unknown
  ASSERT_EQ(directionUnknown, newHistory.GetRecentDirection(0));
  ASSERT_EQ(directionUnknown, newHistory.GetRecentDirection(maxTimeLength_ms));

  // Verify single entry comes back as expected
  newHistory.AddDirectionSample(10, directionFirst, 200, directionFirst);
  ASSERT_EQ(directionFirst, newHistory.GetRecentDirection(0));
  ASSERT_EQ(directionFirst, newHistory.GetRecentDirection(maxTimeLength_ms));

  // Verify multiple counts of single direction comes back as expected
  newHistory.AddDirectionSample(20, directionFirst, 200, directionFirst);
  ASSERT_EQ(directionFirst, newHistory.GetRecentDirection(0));
  ASSERT_EQ(directionFirst, newHistory.GetRecentDirection(maxTimeLength_ms));

  // Verify that a higher more recent string of results is detected 
  newHistory.AddDirectionSample(30, directionSecond, 200, directionSecond);
  newHistory.AddDirectionSample(40, directionSecond, 200, directionSecond);
  newHistory.AddDirectionSample(50, directionSecond, 200, directionSecond);
  ASSERT_EQ(directionSecond, newHistory.GetRecentDirection(50));
  ASSERT_EQ(directionSecond, newHistory.GetRecentDirection(maxTimeLength_ms));

  // Verify that alternating directions comes up with expected result
  newHistory.AddDirectionSample(60, directionFirst, 200, directionFirst);
  newHistory.AddDirectionSample(70, directionFirst, 200, directionFirst);
  ASSERT_EQ(directionFirst, newHistory.GetRecentDirection(70));
  ASSERT_EQ(directionFirst, newHistory.GetRecentDirection(maxTimeLength_ms));

  // Verify that looking at just some recent history (not all time) gets expected result
  ASSERT_EQ(directionSecond, newHistory.GetRecentDirection(50));
} // MicDirectionHistory_RecentDirection_simple

TEST(SUITE, MicDirectionHistory_RecentDirection_complex)
{
  const auto directionFirst = kFirstMicDirectionIndex;
  const auto directionSecond = directionFirst + 1;
  const auto directionThird = directionFirst + 2;
  const auto historyLen = MicDirectionHistory::kMicDirectionHistoryLen;
  const auto maxTimeLength_ms = std::numeric_limits<uint32_t>::max();

  // First fill up the data
  uint32_t t = 0;
  auto nextT = [&t] { t += 10; return t; };
  nextT();

  MicDirectionHistory newHistory{};
  for (uint32_t i = 0; i < historyLen; ++i)
  {
    if ((t % 20) == 0)
    {
      newHistory.AddDirectionSample(t, directionFirst, 200, directionFirst);
    }
    else
    {
      newHistory.AddDirectionSample(t, directionSecond, 200, directionFirst);
    }
    nextT();
  }

  static_assert((MicDirectionHistory::kMicDirectionHistoryLen % 2) == 0, "MicDirectionHistory length expected to be even");

  // Since we have an even number of slots, this should end with an equal count of the two input directions,
  // and the first of the two to be checked should be returned
  ASSERT_EQ(directionFirst, newHistory.GetRecentDirection(maxTimeLength_ms));

  // Now lets add in a new direction with a count just greater than the others, and verify it's picked
  for (uint32_t i=0; i < (historyLen / 2) + 1; ++i)
  {
    newHistory.AddDirectionSample(nextT(), directionThird, 200, directionThird);
  }
  ASSERT_EQ(directionThird, newHistory.GetRecentDirection(maxTimeLength_ms));
} // MicDirectionHistory_RecentDirection_complex

TEST(SUITE, MicDirectionHistory_GetDirectionAtTime)
{
  const auto directionFirst = kFirstMicDirectionIndex;
  const auto directionSecond = directionFirst + 1;
  const auto directionThird = directionFirst + 2;
  const auto maxTimeLength_ms = std::numeric_limits<uint32_t>::max();

  uint32_t t = 0;
  auto nextT = [&t] { t += 10; return t; };

  MicDirectionHistory newHistory{};
  for (uint32_t i=0; i<10; ++i) { newHistory.AddDirectionSample(nextT(), directionFirst, 200, directionFirst); } // ts 10 : 100
  for (uint32_t i=0; i<20; ++i) { newHistory.AddDirectionSample(nextT(), directionSecond, 200, directionSecond); } // ts 110 : 300
  for (uint32_t i=0; i<31; ++i) { newHistory.AddDirectionSample(nextT(), directionFirst, 200, directionFirst); } // ts 310 : 600

  ASSERT_EQ(directionFirst, newHistory.GetDirectionAtTime(0, 0));
  ASSERT_EQ(directionFirst, newHistory.GetDirectionAtTime(maxTimeLength_ms, 0));
  ASSERT_EQ(directionSecond, newHistory.GetDirectionAtTime(330, 200));
  ASSERT_EQ(directionSecond, newHistory.GetDirectionAtTime(330, 250));
  ASSERT_EQ(directionFirst, newHistory.GetDirectionAtTime(maxTimeLength_ms, maxTimeLength_ms));

  for (uint32_t i=0; i<100; ++i) { newHistory.AddDirectionSample(nextT(), directionThird, 200, directionThird); } // ts 610 : 1600
  ASSERT_EQ(directionThird, newHistory.GetDirectionAtTime(maxTimeLength_ms, maxTimeLength_ms));
} // MicDirectionHistory_GetDirectionAtTime

TEST(SUITE, MicDirectionHistory_GetRecentHistory)
{
  // const auto directionUnknown = kMicDirectionUnknown;
  const auto directionFirst = kFirstMicDirectionIndex;
  const auto directionSecond = directionFirst + 1;
  const auto maxTimeLength_ms = std::numeric_limits<uint32_t>::max();

  // Some simple sets of data
  MicDirectionHistory newHistory{};

  // Verify empty history comes back as empty
  MicDirectionNodeList history;
  history = newHistory.GetRecentHistory(0);
  ASSERT_TRUE(history.empty());
  history = newHistory.GetRecentHistory(maxTimeLength_ms);
  ASSERT_TRUE(history.empty());

  // Verify single entry comes back as expected
  newHistory.AddDirectionSample(10, directionFirst, 200, directionFirst);
  history = newHistory.GetRecentHistory(0);
  ASSERT_EQ(history.size(), 1);
  ASSERT_EQ(history[0].directionIndex, directionFirst);
  history = newHistory.GetRecentHistory(maxTimeLength_ms);
  ASSERT_EQ(history.size(), 1);
  ASSERT_EQ(history[0].directionIndex, directionFirst);

  // Verify multiple counts of single direction comes back as expected
  newHistory.AddDirectionSample(20, directionFirst, 200, directionFirst);
  history = newHistory.GetRecentHistory(0);
  ASSERT_EQ(history.size(), 1);
  ASSERT_EQ(history[0].directionIndex, directionFirst);
  history = newHistory.GetRecentHistory(maxTimeLength_ms);
  ASSERT_EQ(history.size(), 1);
  ASSERT_EQ(history[0].directionIndex, directionFirst);

  // Verify that a higher more recent string of results is detected 
  newHistory.AddDirectionSample(30, directionSecond, 200, directionSecond);
  newHistory.AddDirectionSample(40, directionSecond, 200, directionSecond);
  newHistory.AddDirectionSample(50, directionSecond, 200, directionSecond);

  // Try using the exact time that's been recorded
  history = newHistory.GetRecentHistory(50);
  ASSERT_EQ(history.size(), 2);
  ASSERT_EQ(history[0].directionIndex, directionFirst);
  ASSERT_EQ(history[0].count, 2);
  ASSERT_EQ(history[0].timestampEnd, 20);
  ASSERT_EQ(history[1].directionIndex, directionSecond);
  ASSERT_EQ(history[1].count, 3);
  ASSERT_EQ(history[1].timestampEnd, 50);
  // Try using a query for max possible history
  history = newHistory.GetRecentHistory(maxTimeLength_ms);
  ASSERT_EQ(history.size(), 2);
  ASSERT_EQ(history[0].directionIndex, directionFirst);
  ASSERT_EQ(history[0].count, 2);
  ASSERT_EQ(history[0].timestampEnd, 20);
  ASSERT_EQ(history[1].directionIndex, directionSecond);
  ASSERT_EQ(history[1].count, 3);
  ASSERT_EQ(history[1].timestampEnd, 50);

  // Verify that looking at just some recent history (not all time) gets expected result
  newHistory.AddDirectionSample(60, directionFirst, 200, directionFirst);
  newHistory.AddDirectionSample(70, directionFirst, 200, directionFirst);
  history = newHistory.GetRecentHistory(50);
  ASSERT_EQ(history.size(), 2);
  ASSERT_EQ(history[0].directionIndex, directionSecond);
  ASSERT_EQ(history[0].count, 3);
  ASSERT_EQ(history[0].timestampEnd, 50);
  ASSERT_EQ(history[1].directionIndex, directionFirst);
  ASSERT_EQ(history[1].count, 2);
  ASSERT_EQ(history[1].timestampEnd, 70);
} // MicDirectionHistory_GetRecentHistory

TEST(SUITE, MicDirectionHistory_GetHistoryAtTime)
{
  const auto directionFirst = kFirstMicDirectionIndex;
  const auto directionSecond = directionFirst + 1;
  const auto directionThird = directionFirst + 2;
  const auto historyLen = MicDirectionHistory::kMicDirectionHistoryLen;
  const auto maxTimeLength_ms = std::numeric_limits<uint32_t>::max();

  uint32_t t = 0;
  auto nextT = [&t] { t += 10; return t; };

  MicDirectionHistory newHistory{};
  for (uint32_t i=0; i<10; ++i) { newHistory.AddDirectionSample(nextT(), directionFirst, 200, directionFirst); } // ts 10 : 100
  for (uint32_t i=0; i<20; ++i) { newHistory.AddDirectionSample(nextT(), directionSecond, 200, directionSecond); } // ts 110 : 300
  for (uint32_t i=0; i<31; ++i) { newHistory.AddDirectionSample(nextT(), directionFirst, 200, directionFirst); } // ts 310 : 600

  MicDirectionNodeList history;
  // History before beginning of time is empty
  history = newHistory.GetHistoryAtTime(0, 0);
  ASSERT_TRUE(history.empty());

  // History ending at a particular point with a duration included in that node returns only that node
  history = newHistory.GetHistoryAtTime(100, 50);
  ASSERT_EQ(history.size(), 1);
  ASSERT_EQ(history[0].directionIndex, directionFirst);
  ASSERT_EQ(history[0].count, 10);
  ASSERT_EQ(history[0].timestampEnd, 100);

  // History ending in a node with duration continuing into previous node returns both
  history = newHistory.GetHistoryAtTime(110, 100);
  ASSERT_EQ(history.size(), 2);
  ASSERT_EQ(history[0].directionIndex, directionFirst);
  ASSERT_EQ(history[0].count, 10);
  ASSERT_EQ(history[0].timestampEnd, 100);
  ASSERT_EQ(history[1].directionIndex, directionSecond);
  ASSERT_EQ(history[1].count, 20);
  ASSERT_EQ(history[1].timestampEnd, 300);

  for (uint32_t i=0; i<100; ++i) { newHistory.AddDirectionSample(nextT(), directionThird, 200, directionThird); } // ts 610 : 1600
  history = newHistory.GetHistoryAtTime(620, 1000);
  ASSERT_EQ(history.size(), 4);
  history = newHistory.GetHistoryAtTime(620, maxTimeLength_ms);
  ASSERT_EQ(history.size(), 4);
  history = newHistory.GetHistoryAtTime(610, maxTimeLength_ms);
  ASSERT_EQ(history.size(), 3);

  // Requesting history that is past what's recorded gives the last node
  history = newHistory.GetHistoryAtTime(2000, 10);
  ASSERT_EQ(history.size(), 1);

  // Requesting history that previous to what's recorded gives first node
  for (uint32_t i = 0; i < historyLen; ++i) // 1610 to 11610
  {
    if ((t % 20) == 0)
    {
      newHistory.AddDirectionSample(t, directionFirst, 200, directionFirst);
    }
    else
    {
      newHistory.AddDirectionSample(t, directionSecond, 200, directionSecond);
    }
    nextT();
  }
  history = newHistory.GetHistoryAtTime(50, 10);
  ASSERT_EQ(history.size(), 1);
  ASSERT_EQ(history[0].directionIndex, directionSecond);
  ASSERT_EQ(history[0].count, 1);
  ASSERT_EQ(history[0].timestampEnd, 1610);
} // MicDirectionHistory_GetHistoryAtTime
