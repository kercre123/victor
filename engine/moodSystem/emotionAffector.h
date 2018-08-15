/**
 * File: emotionAffector
 *
 * Author: Mark Wesley
 * Created: 10/21/15
 *
 * Description: Rules for scoring a given emotion
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_EmotionAffector_H__
#define __Cozmo_Basestation_MoodSystem_EmotionAffector_H__


#include "clad/types/emotionTypes.h"


namespace Json {
  class Value;
}

namespace Anki {
namespace Vector {

  
class EmotionAffector
{
public:
  
  explicit EmotionAffector(EmotionType type = EmotionType::Count, float baseValue = 0.0f)
    : _emotionType(type)
    , _baseValue(baseValue)
  {  
  }
  
  void Reset();

  EmotionType GetType()  const { return _emotionType; }
  float       GetValue() const { return _baseValue; }
  
  void        SetType(EmotionType newVal)  { _emotionType = newVal; }
  void        SetValue(float newVal)       { _baseValue = newVal; }

  // ===== Json =====
  
  bool ReadFromJson(const Json::Value& inJson);
  bool WriteToJson(Json::Value& outJson) const;
  
private:

  EmotionType   _emotionType;
  float         _baseValue;
};


} // namespace Vector
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_EmotionAffector_H__

