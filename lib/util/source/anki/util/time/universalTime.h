/**
 * File: universalTime
 *
 * Author: damjan
 * Created: 4/14/14
 * 
 * Description: Helper functions for measuring time on different platforms
 * 
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef UTIL_LOGGING_UNIVERSALTIME_H_
#define UTIL_LOGGING_UNIVERSALTIME_H_

namespace Anki{ namespace Util {

namespace Time {

/**
 *  Helper functions for measuring time on different platforms
 * 
 * @author   damjan
 */
class UniversalTime {
public:
  // returns time in nanoseconds
  static unsigned long long int GetCurrentTimeInNanoseconds();

  // returns time in milliseconds
  static double GetCurrentTimeInMilliseconds();
  
  // returns time in seconds
  static double GetCurrentTimeInSeconds();

  // returns time value
  // note this is not in Milli Seconds nor nanoseconds
  // use GetCurrentTimeInNanoseconds
  static unsigned long long int GetCurrentTimeValue();

  // returns nanoseconds since previous time
  static unsigned long long int GetNanosecondsElapsedSince(unsigned long long int previousTime);
};

} // end namespace Time
} // end namespace Anki
} // namespace
#endif //UTIL_LOGGING_UNIVERSALTIME_H_
