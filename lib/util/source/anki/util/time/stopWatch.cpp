/**
 * File: stopWatch
 *
 * Author: damjan
 * Created: 11/20/12
 *
 * Description: Utility class for measuring time and averaging recorded times and finding maximums.
 *
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#include "util/time/stopWatch.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

using namespace std;

namespace Anki {
namespace Util {
namespace Time {

void StopWatch::ResetTrackers()
{
  count_ = 0;
  accumulator_ = 0;
  average_ = 0;
  aboveAverageCount_ = 0;
  aboveAverageAccumulator_ = 0;
  aboveAverageAverage_ = 0;
  max_ = 0;
}

void StopWatch::Start()
{
  startTime = Anki::Util::Time::UniversalTime::GetCurrentTimeValue();
}

// Encapsulates the time measurement code
// returns number of nanoseconds elapsed since startTime
uint64_t StopWatch::StopMeasuringTime()
{
  return Anki::Util::Time::UniversalTime::GetNanosecondsElapsedSince(startTime);
}

double StopWatch::Stop()
{
  uint64_t elapsedNano = StopMeasuringTime();

  double diffMs = (double)elapsedNano / 1000000.0;
  count_++;
  accumulator_ += diffMs;
  tickLength_.Push((float)diffMs);

  // make sure we have enough data for averaging
  if (count_ > 60)
  {
    average_ = accumulator_ / (double)count_;
    if ((averageLimit_ > 0.5 && diffMs > averageLimit_) ||
        (averageLimit_ < 0.5 && diffMs > average_ * 2.0))
    {
      aboveAverageAccumulator_ += diffMs;
      aboveAverageCount_++;
      aboveAverageAverage_ = aboveAverageAccumulator_ / (double)aboveAverageCount_;
      if (diffMs > max_)
        max_ = diffMs;
    }
  }

  return diffMs;
}

void StopWatch::LogStats()
{
  // log some data
  #define SEND_STOPWATCH_STATS(eventNameThird, stat) \
  { string eventName ("StopWatch."+ id_ + eventNameThird); PRINT_NAMED_INFO(eventName.c_str(), "%f", (double)stat); }

  SEND_STOPWATCH_STATS(".f_average", average_);
  SEND_STOPWATCH_STATS(".i_tickCount", count_);
  SEND_STOPWATCH_STATS(".f_aboveAveragePercent", (double)aboveAverageCount_ / (double)count_);
  SEND_STOPWATCH_STATS(".f_aboveAverageAverage", aboveAverageAverage_);
  SEND_STOPWATCH_STATS(".i_aboveAverageCount", aboveAverageCount_);
  SEND_STOPWATCH_STATS(".f_aboveAverageMax", max_);

  string eventName ("StopWatch."+ id_);
  tickLength_.LogStats(eventName.c_str());
}


} // namespace Time
} // namespace Util
} // namespace Anki
