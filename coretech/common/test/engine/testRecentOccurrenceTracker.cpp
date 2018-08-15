#include "util/helpers/includeGTest.h"

#include "coretech/common/engine/utils/recentOccurrenceTracker.h"
#include "json/json.h"

using namespace Anki;

GTEST_TEST(RecentOccurrenceTracker, JsonDefaults)
{
  RecentOccurrenceTracker tracker("test");

  RecentOccurrenceTracker::Handle handle1, handle2, handle3;

  {
    Json::Value empty;
    empty["recentOccurrence"] = Json::objectValue;
    int n;
    float s;
    ASSERT_TRUE( RecentOccurrenceTracker::ParseConfig(empty, n, s) );
    handle1 = tracker.GetHandle(n, s);
  }

  {
    Json::Value noNumber;
    noNumber["recentOccurrence"] = Json::objectValue;
    noNumber["recentOccurrence"]["amountOfSeconds"] = 10.0f;
    int n;
    float s;
    ASSERT_TRUE( RecentOccurrenceTracker::ParseConfig(noNumber, n, s) );
    handle2 = tracker.GetHandle(n, s);
  }

  {
    Json::Value noTime;
    noTime["recentOccurrence"] = Json::objectValue;
    noTime["recentOccurrence"]["numberOfTimes"] = 2;
    int n;
    float s;
    ASSERT_TRUE( RecentOccurrenceTracker::ParseConfig(noTime, n, s) );
    handle3 = tracker.GetHandle(n, s);
  }

  ASSERT_TRUE(handle1 != nullptr);
  ASSERT_TRUE(handle2 != nullptr);
  ASSERT_TRUE(handle3 != nullptr);

  float t = 0.0f;
  tracker.SetTimeProvider( [&t](){ return t; } );
  
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_FALSE(handle2->AreConditionsMet());
  EXPECT_FALSE(handle3->AreConditionsMet());

  t = 1.5f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_FALSE(handle3->AreConditionsMet());

  t = 12.5f;
  EXPECT_FALSE(handle2->AreConditionsMet());
  EXPECT_FALSE(handle3->AreConditionsMet());

  t = 20.5f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());

  t = 100.0f;
  EXPECT_FALSE(handle2->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());

}

GTEST_TEST(RecentOccurrenceTracker, SingleHandle)
{
  RecentOccurrenceTracker tracker("test");

  auto handle = tracker.GetHandle(3, 10.0);

  float t = 0.0f;
  tracker.SetTimeProvider( [&t](){ return t; } );

  EXPECT_FALSE(handle->AreConditionsMet());

  t = 0.1f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 1);

  t = 0.2f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 2);

  t = 10.4f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 1);

  t = 12.6f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 2);

  t = 18.9f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 19.1f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 22.5f;
  EXPECT_TRUE(handle->AreConditionsMet());

  t = 24.0f;
  EXPECT_FALSE(handle->AreConditionsMet());

  t = 100.0f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 1);

}


GTEST_TEST(RecentOccurrenceTracker, MultiHandle)
{
  RecentOccurrenceTracker tracker("test");

  float t = 0.0f;
  tracker.SetTimeProvider( [&t](){ return t; } );

  auto handle1 = tracker.GetHandle(3, 10.0);
  auto handle2 = tracker.GetHandle(2, 15.0);

  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_FALSE(handle2->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 0);

  t = 0.1f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_FALSE(handle2->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 1);

  t = 0.2f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 2);

  t = 10.4f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 12.6f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 18.9f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 19.1f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  auto handle3 = tracker.GetHandle(1, 30.0f);

  t = 22.5f;
  EXPECT_TRUE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 24.0f;
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 33.8f;
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle2->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 34.6f;
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_FALSE(handle2->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 80.0f;
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_FALSE(handle2->AreConditionsMet());
  EXPECT_FALSE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 81.0f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_FALSE(handle2->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 1);

  // release handle 2
  handle2.reset();

  t = 82.0f;
  tracker.AddOccurrence();
  EXPECT_FALSE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 2);

  t = 83.0f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  t = 84.0f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle1->AreConditionsMet());
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 3);

  handle1.reset();

  t = 97.5f;
  tracker.AddOccurrence();
  EXPECT_TRUE(handle3->AreConditionsMet());
  EXPECT_EQ(tracker.GetCurrentSize(), 1);

  handle3.reset();

  {
    auto handle4 = tracker.GetHandle(2, 0.5f);
    t = 105.0f;
    tracker.AddOccurrence();
    EXPECT_FALSE(handle4->AreConditionsMet());
    EXPECT_EQ(tracker.GetCurrentSize(), 1);

    t = 105.4f;
    tracker.AddOccurrence();
    EXPECT_TRUE(handle4->AreConditionsMet());
    EXPECT_EQ(tracker.GetCurrentSize(), 2);

    t = 115.0f;
    tracker.AddOccurrence();
    EXPECT_FALSE(handle4->AreConditionsMet());
    EXPECT_EQ(tracker.GetCurrentSize(), 1);

  }

}


GTEST_TEST(RecentOccurrenceTracker, Destructor)
{
  {
    RecentOccurrenceTracker::Handle handle;

    {
      RecentOccurrenceTracker tracker("test");

      float t = 0.0f;
      tracker.SetTimeProvider( [&t](){ return t; } );

      handle = tracker.GetHandle(2, 5.0);

      t = 0.0f;
      tracker.AddOccurrence();

      t = 4.0f;
      tracker.AddOccurrence();

      t = 4.1f;
      EXPECT_TRUE(handle->AreConditionsMet());
    }

    // tracker is out of scope now, but handle isn't, make sure things don't explode once the handle goes away
    EXPECT_TRUE(true);
  }

  EXPECT_TRUE(true);
}
