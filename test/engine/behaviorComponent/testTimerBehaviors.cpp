/**
 * File: testTimerBehaviors.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-09-18
 *
 * Description: Unit tests for timer / clock utility behaviors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"

#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorWallTimeCoordinator.h"

#include <iomanip>
#include <sstream>

using namespace Anki;
using namespace Anki::Vector;


TEST(BehaviorWallTime, FormatTTSSpecific)
{
  auto GetTTS = [](int hours, int mins, bool is24Hour) {
    struct tm time;
    time.tm_hour = hours;
    time.tm_min = mins;

    return BehaviorWallTimeCoordinator::GetTTSStringForTime(time, is24Hour);
  };

  auto GetTTS12 = [&](int h, int m) { return GetTTS(h, m, false); };
  auto GetTTS24 = [&](int h, int m) { return GetTTS(h, m, true); };

  EXPECT_EQ("12:30", GetTTS12(12, 30));
  EXPECT_EQ("12:30", GetTTS24(12, 30));

  EXPECT_EQ("00:00", GetTTS24(0, 0));
  EXPECT_EQ("12:00", GetTTS12(0, 0));

  EXPECT_EQ("12:00", GetTTS24(12, 0));
  EXPECT_EQ("12:00", GetTTS12(12, 0));

  EXPECT_EQ("00:04", GetTTS24(0, 4));
  EXPECT_EQ("12:04", GetTTS12(0, 4));

  EXPECT_EQ("00:31", GetTTS24(0, 31));
  EXPECT_EQ("12:31", GetTTS12(0, 31));

  EXPECT_EQ("01:09", GetTTS24(1, 9));
  EXPECT_EQ("01:09", GetTTS12(1, 9));

  EXPECT_EQ("01:11", GetTTS24(1, 11));
  EXPECT_EQ("01:11", GetTTS12(1, 11));

  EXPECT_EQ("14:01", GetTTS24(14, 01));
  EXPECT_EQ("02:01", GetTTS12(14, 01));

}

TEST(BehaviorWallTime, FormatTTSAllRoundTrip)
{
  for( int h=0; h<24; h++ ) {
    for( int m=0; m<60; m++ ) {
      struct tm timeIn;
      timeIn.tm_hour = h;
      timeIn.tm_min = m;

      auto tts = BehaviorWallTimeCoordinator::GetTTSStringForTime(timeIn, true);

      // now convert back into a time (note that this doesn't ignores leading zeros on minutes)
      struct tm timeOut;
      std::istringstream ss(tts);
      ss >> std::get_time(&timeOut, "%H:%M");
      EXPECT_EQ(h, timeOut.tm_hour) << tts;
      EXPECT_EQ(m, timeOut.tm_min) << tts;
    }
  }
}
