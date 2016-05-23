/**
 * File: RunningStat
 *
 * Author: damjan
 * Created: 3/27/14
 * 
 * Description: Accurately computing running variance
 * http://www.johndcook.com/standard_deviation.html
 * adapted for less precision - floats..
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef BASESTATION_UTILS_RUNNINGSTAT_H_
#define BASESTATION_UTILS_RUNNINGSTAT_H_


namespace Anki {
namespace Util {
class IEntityLoggerComponent;
namespace Stats {

/**
 * Accurately computing running variance
 * http://www.johndcook.com/standard_deviation.html
 * adapted for less precision - floats..
 *
 * @author   damjan
 */
class RunningStat {
public:

  // Constructor
  RunningStat();

  // clear data
  void Clear();

  // add a new value
  void Push(float x);

  // returns the number of values in the accumulator
  int NumDataValues() const;

  // returns the mean of the sample
  float Mean() const;

  // returns the variance of the sample
  float Variance() const;

  // returns the standard deviation of the sample
  float StandardDeviation() const;

  // Logs collected data
  void LogStats(const char * eventName);

  // Logs collected data
  void LogStats(const char * eventName, const IEntityLoggerComponent * logger);

private:
  int m_n;
  float m_oldM, m_newM, m_oldS, m_newS;
};

} // end namespace Stats
} // end namespace Util
} // end namespace Anki


#endif //BASESTATION_UTILS_RUNNINGSTAT_H_
