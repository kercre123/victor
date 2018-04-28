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

#include "util/stats/runningStat.h"
#include "util/logging/iEntityLoggerComponent.h"
#include <math.h>
#include <string>
#include <stdio.h>
#include "util/logging/logging.h"

namespace Anki {
namespace Util {
namespace Stats {

// Constructor
RunningStat::RunningStat() :
  m_n (0), m_oldM (0.0f), m_newM(0.0f), m_oldS(0.0f), m_newS(0.0f)
{

}

// clear data
void RunningStat::Clear()
{
  m_n = 0;
}

// add a new value
void RunningStat::Push(float x)
{
  m_n++;

  // See Knuth TAOCP vol 2, 3rd edition, page 232
  if (m_n == 1)
  {
    m_oldM = m_newM = x;
    m_oldS = 0.0;
  }
  else
  {
    m_newM = m_oldM + (x - m_oldM)/m_n;
    m_newS = m_oldS + (x - m_oldM)*(x - m_newM);

    // set up for next iteration
    m_oldM = m_newM;
    m_oldS = m_newS;
  }
}

// returns the number of values in the accumulator
int RunningStat::NumDataValues() const
{
  return m_n;
}

// returns the mean of the sample
float RunningStat::Mean() const
{
  return (m_n > 0) ? m_newM : 0.0;
}

// returns the variance of the sample
float RunningStat::Variance() const
{
  return ( (m_n > 1) ? m_newS/(m_n - 1) : 0.0 );
}

// returns the standard deviation of the sample
float RunningStat::StandardDeviation() const
{
  return sqrtf( Variance() );
}

// Logs collected data
void RunningStat::LogStats(const char * statName)
{
  #define SEND_STATS(statNameThird, stat) {                             \
    std::string fullStatName (std::string(statName) + statNameThird); \
    PRINT_NAMED_INFO(fullStatName.c_str(), "%f", stat);                        \
  }

  float fData = NumDataValues();
  SEND_STATS(".f_numDataValues", fData);
  fData = Mean();
  SEND_STATS(".i_mean", fData);
  fData = Variance();
  SEND_STATS(".f_variance", fData);
  fData = StandardDeviation();
  SEND_STATS(".f_standardDeviation", fData);
}

// Logs collected data
void RunningStat::LogStats(const char * eventName, const IEntityLoggerComponent * logger)
{
  #define SEND_LOGGER_STATS(eventNameThird, stat) {                      \
    std::string fullEventName (std::string(eventName) + eventNameThird); \
    logger->InfoF(fullEventName.c_str(), "%f", stat);                   \
  }

  float fData = NumDataValues();
  SEND_LOGGER_STATS(".i_numDataValues", fData);
  fData = Mean();
  SEND_LOGGER_STATS(".f_mean", fData);
  fData = Variance();
  SEND_LOGGER_STATS(".f_variance", fData);
  fData = StandardDeviation();
  SEND_LOGGER_STATS(".f_standardDeviation", fData);
}

} // end namespace Stats
} // end namespace Util
} // end namespace Anki
