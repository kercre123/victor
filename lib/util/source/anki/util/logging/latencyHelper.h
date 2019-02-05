/***********************************************************************************************************************
 *
 *  LatencyHelper
 *  Util
 *
 *  Created by Jarrod Hatfield on 01/28/19.
 *
 *  Description
 *  + Helper functions for timestamping and tracking down latency issues
 *
 **********************************************************************************************************************/


#ifndef __Util_Logging_LatencyHelper_H_
#define __Util_Logging_LatencyHelper_H_

#include "coretech/common/engine/utils/timer.h"
#include <cstddef>
#include <string>


#ifndef ALLOW_INTERVAL_TIMIING
  #define ALLOW_INTERVAL_TIMIING 1
#endif

namespace Anki {
namespace Util {


namespace TimeStampedIntervals {

  void InitLatencyIntervals( const std::string& filepath );

  void BeginInterval();
  void RecordTimeStep( const std::string& idString, size_t tick );
  void EndInterval();

  void ResetCurrentInterval();

} // namespace LatencyDebug
} // namespace Util
} // namespace Anki


#if ALLOW_INTERVAL_TIMIING

  #define INIT_LATENCY_INTERVALS( filename ) \
    Util::TimeStampedIntervals::InitLatencyIntervals( filename );

  #define BEGIN_INTERVAL_TIME() \
    Util::TimeStampedIntervals::BeginInterval();

  #define RECORD_INTERVAL_TIME(id) \
    Util::TimeStampedIntervals::RecordTimeStep(id, BaseStationTimer::getInstance()->GetTickCount());

  #define END_INTERVAL_TIME() \
    Util::TimeStampedIntervals::EndInterval();

  #define RESET_INTERVAL() \
    Util::TimeStampedIntervals::ResetCurrentInterval();

#else

  #define INIT_LATENCY_INTERVALS(filename)
  #define BEGIN_INTERVAL_TIME(id)
  #define RECORD_INTERVAL_TIME(id)
  #define END_INTERVAL_TIME(id)
  #define RESET_INTERVAL()

#endif // ALLOW_INTERVAL_TIMIING

#endif // __Util_Logging_LatencyHelper_H_
