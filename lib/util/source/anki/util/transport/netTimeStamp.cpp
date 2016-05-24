/**
 * File: netTimeStamp
 *
 * Author: Mark Wesley
 * Created: 01/06/16
 *
 * Description: TimeStamp for messages and connection info (milliseconds since 1st timestamp)
 *              Only relevant on local device - each device will have a completely different start
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "util/transport/netTimeStamp.h"
#include "util/logging/logging.h"
#include <assert.h>
#include <chrono>


namespace Anki {
namespace Util {
  

  using NetTimeStampClock = std::chrono::steady_clock;

  
  const NetTimeStampClock::time_point& GetEpochStartTime()
  {
    static NetTimeStampClock::time_point sEpochStartTime = NetTimeStampClock::now();
    return sEpochStartTime;
  }
  
  
  NetTimeStamp GetCurrentNetTimeStamp() // Milliseconds since kEpochStartTime (i.e. since
  {
    using namespace std::chrono;

    const NetTimeStampClock::time_point now = NetTimeStampClock::now();

    const microseconds numMicroSecondsSinceStart = duration_cast<microseconds>(now - GetEpochStartTime());
    const NetTimeStamp numMilliSecondsSinceStart = 0.001 * numMicroSecondsSinceStart.count();
    
    if (numMilliSecondsSinceStart < 0.0)
    {
      PRINT_NAMED_ERROR("NetTimeStamp.NegativeTimeStamp", "Unexpected Timestamp = %f", numMilliSecondsSinceStart);
    }
    
    return numMilliSecondsSinceStart;
  }

  
  static NetTimeStamp kFirstTimeStamp = GetCurrentNetTimeStamp(); // force GetEpochStartTime() to initialize in main app static init to ensure threadsafety)


} // end namespace Util
} // end namespace Anki
