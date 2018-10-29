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


#include "engine/moodSystem/emotionEvent.h"
#include "engine/moodSystem/moodManager.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "json/json.h"


namespace Anki {
namespace Vector {

  
EmotionEvent::EmotionEvent()
  : _affectors()
  , _name()
{
}


void EmotionEvent::AddEmotionAffector(const EmotionAffector& inAffector)
{
  #if ANKI_DEVELOPER_CODE
  for (const EmotionAffector& affector : _affectors)
  {
    if (affector.GetType() == inAffector.GetType())
    {
      PRINT_NAMED_WARNING("EmotionEvent.AddEmotionAffector.DuplicateType",
                          "Type '%s' already present (%f vs %f) - adding anyway!",
                          EnumToString(inAffector.GetType()), inAffector.GetValue(), affector.GetValue());
    }
  }
  #endif // ANKI_DEVELOPER_CODE

  _affectors.push_back(inAffector);
}
  
float EmotionEvent::CalculateRepetitionPenalty(float timeSinceLastOccurrence,
                                               const Util::GraphEvaluator2d& defaultPenalty) const
{
  const float repetitionPenalty = _repetitionPenalty.Empty() ?
                                  defaultPenalty.EvaluateY(timeSinceLastOccurrence) :
                                  _repetitionPenalty.EvaluateY(timeSinceLastOccurrence);
  
  return repetitionPenalty;
}

  
static const char* kEmotionAffectorsKey  = "emotionAffectors";
static const char* kEventName            = "name";
static const char* kRepetitionPenaltyKey = "repetitionPenalty";


bool EmotionEvent::ReadFromJson(const Json::Value& inJson)
{
  _affectors.clear();
  _name.clear();
  _repetitionPenalty.Clear();
  
  // Name
  
  const Json::Value& eventNameField   = inJson[kEventName];
  
  if (!eventNameField.isString())
  {
    PRINT_NAMED_WARNING("EmotionEvent.ReadFromJson.MissingName", "Missing '%s' string entry", kEventName);
    return false;
  }
  
  _name = eventNameField.asCString();
  
  // Affectors
  
  const Json::Value& emotionAffectors = inJson[kEmotionAffectorsKey];
  if (emotionAffectors.isNull())
  {
    PRINT_NAMED_WARNING("EmotionEvent.ReadFromJson.MissingValue", "Missing '%s' entry", kEmotionAffectorsKey);
    return false;
  }
  
  const uint32_t numAffectors = emotionAffectors.size();
  _affectors.reserve(numAffectors);
  
  const Json::Value kNullAffectorValue;
  
  for (uint32_t i = 0; i < numAffectors; ++i)
  {
    const Json::Value& affectorJson = emotionAffectors.get(i, kNullAffectorValue);
    
    EmotionAffector newAffector;
    if (affectorJson.isNull() || !newAffector.ReadFromJson(affectorJson))
    {
      PRINT_NAMED_WARNING("EmotionEvent.ReadFromJson.BadAffector", "Affector %u failed to read", i);
      return false;
    }
    
    _affectors.push_back(newAffector);
  }
  
  const Json::Value& repetitionPenaltyJson = inJson[kRepetitionPenaltyKey];
  if (!repetitionPenaltyJson.isNull())
  {
    if (!_repetitionPenalty.ReadFromJson(repetitionPenaltyJson))
    {
      PRINT_NAMED_WARNING("EmotionEvent.ReadFromJson.BadRepetitionPenalty", "Behavior '%s': %s failed to read", _name.c_str(), kRepetitionPenaltyKey);
    }
  }

  // if the graph isn't specified, the passed in default will be used instead
  
  return true;
}


bool EmotionEvent::WriteToJson(Json::Value& outJson) const
{
  assert( outJson.empty() );
  outJson.clear();
  
  outJson[kEventName] = _name;
  
  {
    Json::Value emotionAffectorsJson(Json::arrayValue);
    
    for (const EmotionAffector& affector : _affectors)
    {
      Json::Value affectorJson;
      affector.WriteToJson(affectorJson);
      emotionAffectorsJson.append(affectorJson);
    }
    
    outJson[kEmotionAffectorsKey] = emotionAffectorsJson;
  }
  
  {
    Json::Value repetitionPenaltyJson;
    _repetitionPenalty.WriteToJson(repetitionPenaltyJson);
    
    outJson[kRepetitionPenaltyKey] = repetitionPenaltyJson;
  }
  
  return true;
}


} // namespace Vector
} // namespace Anki

