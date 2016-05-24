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

#ifndef BASESTATION_UTILS_STOPWATCH_H_
#define BASESTATION_UTILS_STOPWATCH_H_
#include "util/helpers/includeIostream.h"
#include "util/stats/runningStat.h"
#include <string>

namespace Anki {
namespace Util {
namespace Time {

/**
 * Utility class for measuring time and averaging recorded times and finding maximums.
 * 
 * @author   damjan
 */
class StopWatch {
public:

  // Constructor
  StopWatch(const std::string &id, double averageLimitMs = 0.0) : id_(id), averageLimit_(averageLimitMs) { ResetTrackers(); };

  // Constructor
  StopWatch(const char *id, double averageLimitMs = 0.0) : id_(id), averageLimit_(averageLimitMs) { ResetTrackers(); };

  // Destructor
  virtual ~StopWatch(){};

  // Starts measuring time
  void Start();

  // Stops measuring time and logs stats if needed. Returns time elapsed in milliseconds
  double Stop();

  // Resets all trackers
  void ResetTrackers();

  // Logs collected data
  void LogStats();

protected:

  // Encapsulates the time measurement code
  // returns number of nanoseconds elapsed since startTime
  uint64_t StopMeasuringTime();

  // id that will be used for logging
  std::string id_;

  // internals
  unsigned int count_;
  double accumulator_;
  double average_;
  unsigned int aboveAverageCount_;
  double aboveAverageAccumulator_;
  double aboveAverageAverage_;
  double max_;
  double averageLimit_;

  Anki::Util::Stats::RunningStat tickLength_;

  // time at witch start was called
  uint64_t startTime;
};

} // namespace Time
} // namespace Util
} // namespace Anki


#endif //BASESTATION_UTILS_STOPWATCH_H_
