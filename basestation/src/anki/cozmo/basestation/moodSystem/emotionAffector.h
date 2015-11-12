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
namespace Cozmo {

  
class EmotionAffector
{
public:
  
  EmotionAffector(EmotionType type, float baseValue)
    : _emotionType(type)
    , _baseValue(baseValue)
  {  
  }
  
  explicit EmotionAffector(const Json::Value& inJson);

  EmotionType GetType()  const { return _emotionType; }
  float       GetValue() const { return _baseValue; }

  // ===== Json =====
  
  bool ReadFromJson(const Json::Value& inJson);
  bool WriteToJson(Json::Value& outJson) const;
  
private:

  EmotionType   _emotionType;
  float         _baseValue;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_EmotionAffector_H__

