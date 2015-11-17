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


#ifndef __Cozmo_Basestation_ProgressionSystem_ProgressionStat_H__
#define __Cozmo_Basestation_ProgressionSystem_ProgressionStat_H__


#include "clad/types/progressionStatTypes.h"
#include "util/container/circularBuffer.h"


namespace Anki {
namespace Cozmo {


class ProgressionStat
{
public:
  
  using ValueType = uint32_t;
  
  ProgressionStat();
  
  void Reset();
  
  void Update(double currentTime);
  
  void Add(ValueType deltaValue);
  void SetValue(ValueType newValue);
  
  ValueType GetValue() const { return _value; }
  
  ValueType GetHistoryValueTicksAgo(uint32_t numTicksBackwards) const;
  ValueType GetDeltaRecentTicks(uint32_t numTicksBackwards) const { return _value - GetHistoryValueTicksAgo(numTicksBackwards); }
  
  struct HistorySample
  {
    explicit HistorySample(ValueType value=0) : _value(value) {}
    ValueType _value;
  };
  
  static const ValueType kStatValueMin;
  static const ValueType kStatValueDefault;
  static const ValueType kStatValueMax;

private:
  
  // ============================== Member Vars ==============================
  
  Util::CircularBuffer<HistorySample> _history;
  ValueType                           _value;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_ProgressionSystem_ProgressionStat_H__

