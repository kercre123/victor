/**
 * File: batteryStats.cpp
 *
 * Author: Matt Michini
 * Created: 9/5/2018
 *
 * Description: Gather and record some long-running battery statistics
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/battery/batteryStats.h"

#include "util/logging/DAS.h"
#include "util/math/math.h"
#include "util/stats/statsAccumulator.h"
#include "util/time/universalTime.h"

#include <cmath>

namespace Anki {
namespace Vector {

namespace {
  const float kSamplePeriod_sec = 60.f; // sample every minute
  
  const float kDasSendPeriod_sec = 60.f * 60.f * 24.f; // send to DAS every 24 hours
}

BatteryStats::BatteryStats()
: _temperatureStats_degC(std::make_unique<Util::Stats::StatsAccumulator>())
, _voltageStats(std::make_unique<Util::Stats::StatsAccumulator>())
{
  // Set _lastDasSendTime_sec to now so that we do not send to DAS right away
  const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  _lastDasSendTime_sec = now_sec;
}

  
BatteryStats::~BatteryStats()
{
  LogToDas();
}

  
void BatteryStats::Update(const float batteryTemp_degC, const float batteryVolts)
{
  const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  
  // If it is time, add a sample to the statistics accumulators
  if ((_lastSampleTime_sec == 0.f) ||
      (now_sec - _lastSampleTime_sec > kSamplePeriod_sec)) {
    *_temperatureStats_degC += batteryTemp_degC;
    // Ignore voltage readings near zero (processes may have started while battery was 'disconnected' and thus
    // reporting 0 volts)
    if (!Util::IsNearZero(batteryVolts)) {
      *_voltageStats += batteryVolts;
    }
    _lastSampleTime_sec = now_sec;
  }
  
  // If it is time, send to DAS
  if (now_sec - _lastDasSendTime_sec > kDasSendPeriod_sec) {
    LogToDas();
    _lastDasSendTime_sec = now_sec;
  }
}


void BatteryStats::LogToDas()
{
  DASMSG(battery_temperature_stats, "battery.temperature_stats", "Battery temperature statistics");
  DASMSG_SET(i1, _temperatureStats_degC->GetIntMin(), "Minimum battery temperature experienced (degC)");
  DASMSG_SET(i2, _temperatureStats_degC->GetIntMax(), "Maximum battery temperature experienced (degC)");
  DASMSG_SET(i3, _temperatureStats_degC->GetIntMean(), "Average battery temperature experienced (degC)");
  DASMSG_SET(i4, _temperatureStats_degC->GetNum(), "Total number of samples");
  DASMSG_SEND();
  
  // Voltage stats are recorded in volts, but reported to DAS in millivolts
  DASMSG(battery_voltage_stats, "battery.voltage_stats", "Battery voltage statistics");
  DASMSG_SET(i1, std::round(1000.f * _voltageStats->GetMin()), "Minimum battery voltage experienced (mV)");
  DASMSG_SET(i2, std::round(1000.f * _voltageStats->GetMax()), "Maximum battery voltage experienced (mV)");
  DASMSG_SET(i3, std::round(1000.f * _voltageStats->GetMean()), "Average battery voltage experienced (mV)");
  DASMSG_SET(i4, _voltageStats->GetNum(), "Total number of samples");
  DASMSG_SEND();
  
  _temperatureStats_degC->Clear();
  _voltageStats->Clear();
}


} // Vector namespace
} // Anki namespace
