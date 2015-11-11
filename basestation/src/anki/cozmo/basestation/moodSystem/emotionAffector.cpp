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


#include "anki/cozmo/basestation/moodSystem/emotionAffector.h"
#include "anki/cozmo/basestation/moodSystem/emotionTypesHelpers.h"
#include "util/logging/logging.h"
#include "json/json.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {

  
EmotionAffector::EmotionAffector(const Json::Value& inJson)
  : _emotionType(EmotionType::Count)
  , _baseValue(0.0f)
{
  ReadFromJson(inJson);
}

  
bool EmotionAffector::ReadFromJson(const Json::Value& inJson)
{
  // [MARKW:TODO] Implementation required
  
  return true;
}

  
bool EmotionAffector::WriteToJson(Json::Value& outJson) const
{
  assert( outJson.empty() );
  outJson.clear();
  
  // [MARKW:TODO] Implementation required

  return true;
}


} // namespace Cozmo
} // namespace Anki

