/**
 * File: emotionScorer
 *
 * Author: Mark Wesley
 * Created: 10/21/15
 *
 * Description: Rules for scoring a given emotion
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "engine/moodSystem/emotionScorer.h"
#include "util/logging/logging.h"
#include "json/json.h"
#include <assert.h>


namespace Anki {
namespace Vector {


EmotionScorer::EmotionScorer(const Json::Value& inJson)
  : _emotionType(EmotionType::Count)
  , _scoreGraph()
  , _trackDeltaScore(false)
{
  ReadFromJson(inJson);
}
  
  
static const char* kEmotionTypeKey = "emotionType";
static const char* kScoreGraphKey  = "scoreGraph";
static const char* kTrackDeltaKey  = "trackDelta";


bool EmotionScorer::ReadFromJson(const Json::Value& inJson)
{
  const Json::Value& emotionType = inJson[kEmotionTypeKey];
  const Json::Value& scoreGraph  = inJson[kScoreGraphKey];
  const Json::Value& trackDelta  = inJson[kTrackDeltaKey];
  
  const char* emotionTypeString = emotionType.isString() ? emotionType.asCString() : "";
  _emotionType  = EmotionTypeFromString( emotionTypeString );
  
  if (_emotionType == EmotionType::Count)
  {
    PRINT_NAMED_WARNING("EmotionScorer.ReadFromJson.BadType", "Bad '%s' = '%s'", kEmotionTypeKey, emotionTypeString);
    return false;
  }
  
  if (!_scoreGraph.ReadFromJson(scoreGraph))
  {
    PRINT_NAMED_WARNING("EmotionScorer.ReadFromJson.BadScoreGraph", "Bad '%s' failed to parse", kScoreGraphKey);
    return false;
  }

  if (trackDelta.isNull())
  {
    PRINT_NAMED_WARNING("EmotionScorer.ReadFromJson.MissingTrackDelta", "Missing '%s' entry", kTrackDeltaKey);
    return false;
  }
  
  _trackDeltaScore = trackDelta.asBool();
  
  return true;
}


bool EmotionScorer::WriteToJson(Json::Value& outJson) const
{
  assert( outJson.empty() );
  outJson.clear();
  
  outJson[kEmotionTypeKey] = EmotionTypeToString(_emotionType);
  
  Json::Value scoreGraphJson;
  _scoreGraph.WriteToJson(scoreGraphJson);
  outJson[kScoreGraphKey] = scoreGraphJson;

  outJson[kTrackDeltaKey] = _trackDeltaScore;
  
  return true;
}


} // namespace Vector
} // namespace Anki

