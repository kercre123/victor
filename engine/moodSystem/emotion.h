/**
 * File: emotion
 *
 * Author: Mark Wesley
 * Created: 10/14/15
 *
 * Description: Tracks a single emotion (which together make up the mood for a Cozmo Robot)
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_Emotion_H__
#define __Cozmo_Basestation_MoodSystem_Emotion_H__


#include "clad/types/emotionTypes.h"
#include "util/container/circularBuffer.h"


namespace Anki {
  
namespace Util {
  class GraphEvaluator2d;
}
  
namespace Cozmo {


class Emotion
{
public:
  
  Emotion();
  
  void Reset();
  
  void Update(const Anki::Util::GraphEvaluator2d& decayGraph, double currentTime, float timeDelta);
  
  void  Add(float penalizedDeltaValue);
  void  SetValue(float newValue);
  
  float GetValue() const { return _value; }
  float GetHistoryValueTicksAgo(uint32_t numTicksBackwards) const;
  float GetHistoryValueSecondsAgo(float secondsBackwards) const;
  float GetDeltaRecentTicks(uint32_t numTicksBackwards) const { return _value - GetHistoryValueTicksAgo(numTicksBackwards); }
  float GetDeltaRecentSeconds(float secondsBackwards)   const { return _value - GetHistoryValueSecondsAgo(secondsBackwards); }

  // range defaults to values specified in cpp, but can get set manually here
  void SetEmotionValueRange(float min, float max);
  
  struct HistorySample
  {
    explicit HistorySample(float value=0.0f, float timeDelta=0.0f) : _value(value), _timeDelta(timeDelta) {}
    float _value;
    float _timeDelta;
  };

private:
  
  // ============================== Member Vars ==============================
  
  Util::CircularBuffer<HistorySample> _history;
  float                               _value;
  float                               _minValue;
  float                               _maxValue;

  float                               _timeDecaying;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_Emotion_H__

