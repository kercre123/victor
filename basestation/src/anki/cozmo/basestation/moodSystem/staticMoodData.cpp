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


#include "anki/cozmo/basestation/moodSystem/staticMoodData.h"
#include "anki/cozmo/basestation/moodSystem/emotionTypesHelpers.h"
#include "json/json.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  
StaticMoodData::StaticMoodData()
{
  InitDecayGraphs();
}
  
  
void StaticMoodData::Init(const Json::Value& inJson)
{
  InitDecayGraphs();
  ReadFromJson(inJson);
}


// Hard coded static init for now, will replace with data loading eventually
void StaticMoodData::InitDecayGraphs()
{
  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    Anki::Util::GraphEvaluator2d& decayGraph = _emotionDecayGraphs[i];
    if (decayGraph.GetNumNodes() == 0)
    {
      decayGraph.AddNode(  0.0f, 1.0f);
      decayGraph.AddNode( 15.0f, 1.0f);
      decayGraph.AddNode( 60.0f, 0.9f);
      decayGraph.AddNode(150.0f, 0.6f);
      decayGraph.AddNode(300.0f, 0.0f);
    }
    assert(VerifyDecayGraph(decayGraph));
  }
}


bool StaticMoodData::VerifyDecayGraph(const Anki::Util::GraphEvaluator2d& newGraph, bool warnOnErrors)
{
  const size_t numNodes = newGraph.GetNumNodes();
  if (numNodes == 0)
  {
    if (warnOnErrors)
    {
      PRINT_NAMED_WARNING("VerifyDecayGraph.NoNodes", "Invalid graph has 0 nodes");
    }
    return false;
  }
  
  bool isValid = true;
  
  float lastY = 0.0f;
  for (size_t i=0; i < numNodes; ++i)
  {
    const Anki::Util::GraphEvaluator2d::Node& node = newGraph.GetNode(i);
    
    if (node._y < 0.0f)
    {
      if (warnOnErrors)
      {
        PRINT_NAMED_WARNING("VerifyDecayGraph.NegativeYNode", "Node[%zu] = (%f, %f) is Negative!", i, node._x, node._y);
      }
      isValid = false;
    }
    if ((i != 0) && (node._y > lastY))
    {
      if (warnOnErrors)
      {
        PRINT_NAMED_WARNING("VerifyDecayGraph.IncreasingYNode", "Node[%zu] = (%f, %f) is has y > than previous (%f)", i, node._x, node._y, lastY);
      }
      isValid = false;
    }
    
    lastY = node._y;
  }
  
  return isValid;
}


bool StaticMoodData::SetDecayGraph(EmotionType emotionType, const Anki::Util::GraphEvaluator2d& newGraph)
{
  const size_t index = (size_t)emotionType;
  assert((index >= 0) && (index < (size_t)EmotionType::Count));
  
  if (!VerifyDecayGraph(newGraph))
  {
    PRINT_NAMED_WARNING("MoodManager.SetDecayGraph.Invalid", "Invalid graph for emotion '%s'", EnumToString(emotionType));
    return false;
  }
  else
  {
    _emotionDecayGraphs[index] = newGraph;
    return true;
  }
}


const Anki::Util::GraphEvaluator2d& StaticMoodData::GetDecayGraph(EmotionType emotionType)
{
  const size_t index = (size_t)emotionType;
  assert((index >= 0) && (index < (size_t)EmotionType::Count));
  
  Anki::Util::GraphEvaluator2d& decayGraph = _emotionDecayGraphs[index];
  assert(decayGraph.GetNumNodes() > 0);
  
  return decayGraph;
}
  

static const char* kDecayGraphsKey   = "decayGraphs";
static const char* kEmotionTypeKey   = "emotionType";
static const char* kEventMapperKey   = "eventMapper";
  
  
bool StaticMoodData::ReadFromJson(const Json::Value& inJson)
{
  _emotionEventMapper.Clear();
  
  const Json::Value& decayGraphsJson = inJson[kDecayGraphsKey];
  if (decayGraphsJson.isNull())
  {
    PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.MissingValue", "Missing '%s' entry", kDecayGraphsKey);
    return false;
  }
  
  Anki::Util::GraphEvaluator2d defaultDecayGraph;
  bool decayGraphsSet[(size_t)EmotionType::Count] = {0};
  
  // Per emotion graph
  const Json::Value kNullGraphValue;
  for (uint32_t i = 0; i < decayGraphsJson.size(); ++i)
  {
    const Json::Value& decayGraphJson = decayGraphsJson.get(i, kNullGraphValue);

    Anki::Util::GraphEvaluator2d decayGraph;
    if (decayGraphJson.isNull() || !decayGraph.ReadFromJson(decayGraphJson))
    {
      PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.BadDecayGraph", "DecayGraph %u failed to read", i);
      return false;
    }
    
    const Json::Value& graphEmotionTypeName = decayGraphJson[kEmotionTypeKey];
    
    const char* emotionTypeString = graphEmotionTypeName.isString() ? graphEmotionTypeName.asCString() : "";
    const EmotionType emotionType = EmotionTypeFromString( emotionTypeString );
    
    if (emotionType < EmotionType::Count)
    {
      if (SetDecayGraph(emotionType, decayGraph))
      {
        decayGraphsSet[(size_t)emotionType] = true;
      }
      else
      {
        PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.InvalidDecayGraph", "DecayGraph %u '%s' failed to set", i, emotionTypeString);
        return false;
      }
    }
    else
    {
      if (strcmp(emotionTypeString, "default") == 0)
      {
        // Default graph
        defaultDecayGraph = decayGraph;
      }
      else
      {
        PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.BadDecayGraphEmotionType", "DecayGraph %u failed to read - unknown type name '%s'", i, emotionTypeString);
        return false;
      }
    }
  }
  
  const bool hasValidDefaultDecayGraph = VerifyDecayGraph(defaultDecayGraph, false);
  if (hasValidDefaultDecayGraph)
  {
    for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
    {
      if (!decayGraphsSet[i])
      {
        SetDecayGraph((EmotionType)i, defaultDecayGraph);
      }
    }
  }
  
  const Json::Value& emotionEventsJson = inJson[kEventMapperKey];
  if (emotionEventsJson.isNull() || !_emotionEventMapper.ReadFromJson(emotionEventsJson))
  {
    PRINT_NAMED_WARNING("StaticMoodData.ReadFromJson.BadEventMapper", "EventMapper '%s' failed to read", kEventMapperKey);
    return false;
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
      const Anki::Util::GraphEvaluator2d& decayGraph = _emotionDecayGraphs[i];
      const char* emotionTypeName = EmotionTypeToString((EmotionType)i);

      Json::Value graphJson;
      decayGraph.WriteToJson(graphJson);
      graphJson[kEmotionTypeKey] = emotionTypeName;
      decayGraphsJson.append(graphJson);
    }
    
    outJson[kDecayGraphsKey] = decayGraphsJson;
  }
  
  {
    Json::Value emotionEventsJson;
    _emotionEventMapper.WriteToJson(emotionEventsJson);
    
    outJson[kEventMapperKey] = emotionEventsJson;
  }
  
  return true;
}

  
} // namespace Cozmo
} // namespace Anki

