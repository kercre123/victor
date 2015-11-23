/**
 * File: progressionStat
 *
 * Author: Mark Wesley
 * Created: 11/10/15
 *
 * Description: Tracks a single stat
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "anki/cozmo/basestation/progressionSystem/progressionStat.h"
#include "util/console/consoleInterface.h"
#include "util/math/math.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {

  
const ProgressionStat::ValueType ProgressionStat::kStatValueMin     =     0;
const ProgressionStat::ValueType ProgressionStat::kStatValueDefault =     0;
const ProgressionStat::ValueType ProgressionStat::kStatValueMax     = 99999;


static const char* kStatSectionName = "Progression.Stats";
CONSOLE_VAR(uint32_t, kMaxStatHistorySamples, kStatSectionName,  128);
  

ProgressionStat::ProgressionStat()
  : _history(kMaxStatHistorySamples)
  , _value(kStatValueDefault)
{
  _history.push_back( HistorySample(_value) );
}

  
void ProgressionStat::Reset()
{
  _history.Reset(kMaxStatHistorySamples);
  _value = kStatValueDefault;
  _history.push_back( HistorySample(_value) );
}

  
ProgressionStat::ValueType ProgressionStat::GetHistoryValueTicksAgo(uint32_t numTicksBackwards) const
{
  const uint32_t numEntries = Util::numeric_cast<uint32_t>(_history.size());
  if ((numEntries > 0) && (numTicksBackwards > 0))
  {
    const uint32_t sampleIndex = (numEntries > numTicksBackwards) ? (numEntries - numTicksBackwards) : 0;
    const HistorySample& oldSample = _history[sampleIndex];
    return oldSample._value;
  }
  
  // use current value
  return _value;
}

  
void ProgressionStat::Update(double currentTime)
{
  _history.push_back( HistorySample(_value) );
}


void ProgressionStat::Add(ValueType deltaValue)
{
  const ValueType newValue = Anki::Util::ClampedAddition(_value, deltaValue);
  SetValue(newValue);
}

  
void ProgressionStat::SetValue(ValueType newValue)
{
  const ValueType clampedValue = Anki::Util::Clamp(newValue, kStatValueMin, kStatValueMax);
  _value = clampedValue;
}


} // namespace Cozmo
} // namespace Anki

