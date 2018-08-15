/**
 * File: emotionEvent
 *
 * Author: Mark Wesley
 * Created: 11/09/15
 *
 * Description: Moode/Emotion affecting event
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_EmotionEvent_H__
#define __Cozmo_Basestation_MoodSystem_EmotionEvent_H__


#include "engine/moodSystem/emotionAffector.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include <vector>


namespace Json {
  class Value;
}

namespace Anki {
namespace Vector {

  
class MoodManager;

  
class EmotionEvent
{
public:
  
  EmotionEvent();
  
  // ensure noexcept default move and copy assignment-operators/constructors are created
  EmotionEvent(const EmotionEvent& other) = default;
  EmotionEvent& operator=(const EmotionEvent& other) = default;
  EmotionEvent(EmotionEvent&& rhs) noexcept = default;
  EmotionEvent& operator=(EmotionEvent&& rhs) noexcept = default;
  
  void AddEmotionAffector(const EmotionAffector& inAffector);
  
  const std::string& GetName() const { return _name; }
  void SetName(const char* inName) { _name = inName; }
  
  size_t GetNumAffectors() const { return _affectors.size(); }
  const EmotionAffector& GetAffector(size_t index) const { return _affectors[index]; }
  const std::vector<EmotionAffector>& GetAffectors() const { return _affectors; }
  
  // penalty is applied as a scalar, so 1.0f == no penalty. If this event doesn't have a configured penalty,
  // the passed in default penalty config will be used
  float CalculateRepetitionPenalty(float timeSinceLastOccurrence, const Util::GraphEvaluator2d& defaultPenalty) const;

  // ===== Json =====
  
  bool ReadFromJson(const Json::Value& inJson);
  bool WriteToJson(Json::Value& outJson) const;
  
private:
  
  std::vector<EmotionAffector>  _affectors;
  std::string                   _name;
  Util::GraphEvaluator2d        _repetitionPenalty;
};


} // namespace Vector
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_EmotionEvent_H__

