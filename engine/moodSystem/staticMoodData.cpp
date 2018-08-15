/**
 * File: staticMoodData
 *
 * Author: Mark Wesley
 * Created: 11/16/15
 *
 * Description: Storage for static data (e.g. loaded from files) for the mood system, can be shared amongst multiple
 *              Robots / MoodManagers etc.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "engine/moodSystem/staticMoodData.h"

#include "coretech/common/engine/jsonTools.h"
#include "json/json.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

namespace {

static const char* kDecayGraphsKey = "decayGraphs";
static const char* kEmotionTypeKey = "emotionType";
static const char* kEventMapperKey = "eventMapper";
static const char* kDefaultRepetitionPenaltyKey = "defaultRepetitionPenalty";
static const char* kValueRangeKey = "valueRanges";
static const char* kMinKey = "min";
static const char* kMaxKey = "max";

static const char* kJsonDebugName = "StaticMoodData";

}


StaticMoodData::StaticMoodData()
{
  InitDecayEvaluators();
}
  
  
void StaticMoodData::Init(const Json::Value& inJson)
{
  InitDecayEvaluators();
  ReadFromJson(inJson);
}


void StaticMoodData::InitDecayEvaluators()
{
  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    auto& decayEvaluator = _emotionDecayEvaluators[i];
    if (decayEvaluator.Empty())
    {
      // use a hardcoded default      
      // NOTE: unfortunately, unit tests depend on this. It shouldn't make a difference in production at all

      Util::GraphEvaluator2d decayGraph;
      decayGraph.AddNode(  0.0f, 1.0f);
      decayGraph.AddNode( 15.0f, 1.0f);
      decayGraph.AddNode( 60.0f, 0.9f);
      decayGraph.AddNode(150.0f, 0.6f);
      decayGraph.AddNode(300.0f, 0.0f);

      const auto defaultDecayType = MoodDecayEvaulator::DecayGraphType::TimeRatio;
      if( ANKI_VERIFY(MoodDecayEvaulator::VerifyDecayGraph(decayGraph, defaultDecayType, true),
                      "StaticMoodData.Init.InvalidDefaultDecayGraph",
                      "default static-constructed mood graph is invalid! This is a bug") ) {

        decayEvaluator.SetDecayGraph(decayGraph, defaultDecayType);
      }
    }
  }
}

bool StaticMoodData::SetDecayEvaluator(EmotionType emotionType,
                                       const Util::GraphEvaluator2d& newGraph,
                                       const MoodDecayEvaulator::DecayGraphType graphType)
{
  const size_t index = (size_t)emotionType;
  assert((index >= 0) && (index < (size_t)EmotionType::Count));
  
  if (!MoodDecayEvaulator::VerifyDecayGraph(newGraph, graphType, true))
  {
    PRINT_NAMED_WARNING("MoodManager.SetDecayGraph.Invalid",
                        "Invalid graph for emotion '%s'",
                        EnumToString(emotionType));
    return false;
  }
  else
  {
    _emotionDecayEvaluators[index].SetDecayGraph(newGraph, graphType);
    return true;
  }
}

const MoodDecayEvaulator& StaticMoodData::GetDecayEvaluator(EmotionType emotionType) const
{
  const size_t index = (size_t)emotionType;
  DEV_ASSERT((index >= 0) && (index < (size_t)EmotionType::Count), "StaticMoodData.GetDecayEvaluator.InvalidIndex");
  
  const MoodDecayEvaulator& ret = _emotionDecayEvaluators[index];
  DEV_ASSERT(!ret.Empty(), "StaticMoodData.GetDecayEvaluator.EmptyEvaluator");
  
  return ret;
}
  
bool StaticMoodData::LoadEmotionEvents(const Json::Value& inJson)
{
  return _emotionEventMapper.LoadEmotionEvents(inJson);
}

bool StaticMoodData::ReadFromJson(const Json::Value& inJson)
{
  _emotionEventMapper.Clear();
  _defaultRepetitionPenalty.Clear();
  _emotionValueRangeMap.clear();

  
  const Json::Value& decayGraphsJson = inJson[kDecayGraphsKey];
  if (decayGraphsJson.isNull())
  {
    PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.MissingValue", "Missing '%s' entry", kDecayGraphsKey);
    return false;
  }

  Json::Value defaultDecayConfig;  

  // track if they have been set by data or need to be set to defaults
  bool decayGraphsSet[(size_t)EmotionType::Count] = {0};
  
  // Per emotion graph
  const Json::Value kNullGraphValue;
  for (uint32_t i = 0; i < decayGraphsJson.size(); ++i)
  {
    const Json::Value& decayGraphJson = decayGraphsJson.get(i, kNullGraphValue);
    
    const Json::Value& graphEmotionTypeName = decayGraphJson[kEmotionTypeKey];
    
    const char* emotionTypeString = graphEmotionTypeName.isString() ? graphEmotionTypeName.asCString() : "";

    if (strcmp(emotionTypeString, "default") == 0) {
      // Default graph
      defaultDecayConfig = decayGraphJson;
    }
    else {
      const EmotionType emotionType = EmotionTypeFromString( emotionTypeString );
    
      if (emotionType < EmotionType::Count)
      {
        if (_emotionDecayEvaluators[(size_t)emotionType].ReadFromJson(decayGraphJson))
        {
          decayGraphsSet[(size_t)emotionType] = true;
        }
        else
        {
          PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.InvalidDecayGraph",
                              "DecayGraph %u '%s' failed to set",
                              i, emotionTypeString);
          return false;
        }
      }
      else
      {
        PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.BadDecayGraphEmotionType",
                            "DecayGraph %u failed to read - unknown type name '%s'",
                            i, emotionTypeString);
        return false;
      }
    }
  }
  
  const bool hasDefault = !defaultDecayConfig.isNull();
  if (hasDefault)
  {
    for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
    {
      if (!decayGraphsSet[i])
      {
        _emotionDecayEvaluators[i].ReadFromJson(defaultDecayConfig);
      }
    }
  }

  const Json::Value& emotionEventsJson = inJson[kEventMapperKey];
  if (emotionEventsJson.isNull() || !_emotionEventMapper.ReadFromJson(emotionEventsJson))
  {
    PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.BadEventMapper", "EventMapper '%s' failed to read", kEventMapperKey);
    return false;
  }
  
  const Json::Value& defaultRepetitionPenaltyJson = inJson[kDefaultRepetitionPenaltyKey];
  if (!defaultRepetitionPenaltyJson.isNull())
  {
    if (!_defaultRepetitionPenalty.ReadFromJson(defaultRepetitionPenaltyJson))
    {
      PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.BadDefaultRepetitionPenalty", "'%s' failed to read", kDefaultRepetitionPenaltyKey);
    }
  }

  const Json::Value& valueRangesJson = inJson[kValueRangeKey];
  if (!valueRangesJson.isNull())
  {
    for( const auto& entry : valueRangesJson ) {
      const std::string& emotionStr = JsonTools::ParseString(entry, kEmotionTypeKey, kJsonDebugName);
      const EmotionType emotion = EmotionTypeFromString( emotionStr );

      const float minVal = JsonTools::ParseFloat(entry, kMinKey, kJsonDebugName);
      const float maxVal = JsonTools::ParseFloat(entry, kMaxKey, kJsonDebugName);
      if( ANKI_VERIFY( minVal <= maxVal,
                       "StaticMoodData.ReadFromJson.BadValueRange",
                       "Value range for emotion '%s' in invalid: %f - %f",
                       emotionStr.c_str(),
                       minVal,
                       maxVal) ) {
        
        _emotionValueRangeMap.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(emotion),
                                      std::forward_as_tuple(minVal, maxVal));
      }
    }
  }
  
  // Ensure there is a valid graph
  if (_defaultRepetitionPenalty.GetNumNodes() == 0)
  {
    PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.EmptyDefaultRepetitionPenalty", "'%s' missing or bad - defaulting to no penalty", kDefaultRepetitionPenaltyKey);
    _defaultRepetitionPenalty.AddNode(0.0f, 1.0f); // no penalty for any value
  }

  return true;
}


bool StaticMoodData::WriteToJson(Json::Value& outJson) const
{
  assert( outJson.empty() );
  outJson.clear();
  
  {
    Json::Value decayGraphsJson(Json::arrayValue);
    
    for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
    {
      const auto& decayEvaluator = _emotionDecayEvaluators[i];
      const char* emotionTypeName = EmotionTypeToString((EmotionType)i);

      Json::Value evaluatorJson;
      decayEvaluator.WriteToJson(evaluatorJson);
      evaluatorJson[kEmotionTypeKey] = emotionTypeName;
      decayGraphsJson.append(evaluatorJson);
    }
    
    outJson[kDecayGraphsKey] = decayGraphsJson;
  }
  
  {
    Json::Value emotionEventsJson;
    _emotionEventMapper.WriteToJson(emotionEventsJson);
    
    outJson[kEventMapperKey] = emotionEventsJson;
  }
  
  {
    Json::Value defaultRepetitionPenaltyJson;
    if (_defaultRepetitionPenalty.WriteToJson(defaultRepetitionPenaltyJson))
    {
      outJson[kDefaultRepetitionPenaltyKey] = defaultRepetitionPenaltyJson;
    }
  }
  
  return true;
}

  
} // namespace Vector
} // namespace Anki

