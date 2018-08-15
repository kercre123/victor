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


#include "engine/moodSystem/emotionAffector.h"
#include "util/logging/logging.h"
#include "json/json.h"
#include <assert.h>


namespace Anki {
namespace Vector {

  
static const char* kEmotionTypeKey = "emotionType";
static const char* kValueKey       = "value";

  
void EmotionAffector::Reset()
{
  _emotionType = EmotionType::Count;
  _baseValue = 0.0;
}

  
bool EmotionAffector::ReadFromJson(const Json::Value& inJson)
{
  const Json::Value& emotionType = inJson[kEmotionTypeKey];
  const Json::Value& value       = inJson[kValueKey];
  
  const char* emotionTypeString = emotionType.isString() ? emotionType.asCString() : "";
  _emotionType  = EmotionTypeFromString( emotionTypeString );
  
  if (_emotionType == EmotionType::Count)
  {
    PRINT_NAMED_WARNING("EmotionAffector.ReadFromJson.BadType", "Bad '%s' = '%s'", kEmotionTypeKey, emotionTypeString);
    Reset();
    return false;
  }
  
  if (value.isNull())
  {
    PRINT_NAMED_WARNING("EmotionAffector.ReadFromJson.MissingValue", "Missing '%s' entry", kValueKey);
    Reset();
    return false;
  }
  
  _baseValue = value.asFloat();
  
  return true;
}

  
bool EmotionAffector::WriteToJson(Json::Value& outJson) const
{
  assert( outJson.empty() );
  outJson.clear();
  
  outJson[kEmotionTypeKey] = EmotionTypeToString(_emotionType);
  outJson[kValueKey]       = _baseValue;

  return true;
}


} // namespace Vector
} // namespace Anki

