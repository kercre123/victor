/**
 * File: moodDecayEvaluator.h
 *
 * Author: Brad Neuman
 * Created: 2018-04-06
 *
 * Description: Helper to handle mood decay with the appropriate parameters
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/moodSystem/moodDecayEvaluator.h"

#include "coretech/common/engine/jsonTools.h"
#include "json/json.h"
#include "lib/util/source/anki/util/math/math.h"
#include "util/graphEvaluator/graphEvaluator2d.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kGraphTypeKey = "graphType";
static const char* kNodesKey = "nodes";
}

MoodDecayEvaulator::MoodDecayEvaulator()
{
}

MoodDecayEvaulator::~MoodDecayEvaulator()
{
}

bool MoodDecayEvaulator::ReadFromJson(const Json::Value& inJson)
{
  const std::string& graphTypeStr = JsonTools::ParseString(inJson, kGraphTypeKey, "MoodDecayEvaluator.ReadFromJson");

  if( graphTypeStr == "TimeRatio" ) {
    _graphType = DecayGraphType::TimeRatio;
  }
  else if( graphTypeStr == "ValueSlope" ) {
    _graphType = DecayGraphType::ValueSlope;
  }
  else {
    PRINT_NAMED_ERROR("MoodDecayEvaulator.Init.InvalidGraphType",
                      "Graph type of '%s' is not supported",
                      graphTypeStr.c_str());
    return false;
  }

  if( _decayGraph == nullptr ) {
    _decayGraph.reset(new Util::GraphEvaluator2d);
  }
  
  const bool readOK = _decayGraph->ReadFromJson(inJson);
  ANKI_VERIFY(readOK, "MoodDecayEvaulator.Init.InvalidGraphNodes",
              "Couldn't read decay graph from json");

  return readOK;  
}

bool MoodDecayEvaulator::WriteToJson(Json::Value& outJson) const
{
  if( nullptr == _decayGraph ) {
    PRINT_NAMED_WARNING("MoodDecayEvaulator.WriteToJson.Empty",
                        "Trying to write without a valid decay graph");
    return false;
  }
  
  switch( _graphType ) {
    case DecayGraphType::TimeRatio:
      outJson[kGraphTypeKey] = "TimeRatio";
      break;
    case DecayGraphType::ValueSlope:
      outJson[kGraphTypeKey] = "ValueSlope";
      break;
  }

  Json::Value graphJson;
  const bool ret = _decayGraph->WriteToJson(graphJson);
  if( ret ) {
    if( ANKI_VERIFY(!graphJson[kNodesKey].isNull(),
                    "MoodDecayEvaulator.WriteToJson.NoNodes","") ) {
      outJson[kNodesKey] = graphJson[kNodesKey];
      return true;
    }
  }
  return false;
}


bool MoodDecayEvaulator::VerifyDecayGraph(const Util::GraphEvaluator2d& newGraph,
                                          DecayGraphType graphType,
                                          bool warnOnErrors)
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

  // if this is time decay ratio graph, then it must be monotonic down
  const bool mustBeDecreasing = graphType == DecayGraphType::TimeRatio;
  const bool xMustBePositive = graphType == DecayGraphType::TimeRatio;
  
  bool isValid = true;
  
  float lastY = 0.0f;
  for (size_t i=0; i < numNodes; ++i) {
    const Util::GraphEvaluator2d::Node& node = newGraph.GetNode(i);

    if(xMustBePositive && node._x < 0.0f) {
      if (warnOnErrors) {
        PRINT_NAMED_WARNING("MoodDecayEvaulator.VerifyDecayGraph.NegativeXNode",
                            "Node[%zu] = (%f, %f) is Negative!", i, node._x, node._y);
      }
      isValid = false;
    }
    if(node._y < 0.0f) {
      if(warnOnErrors) {
        PRINT_NAMED_WARNING("VerifyDecayGraph.NegativeYNode", "Node[%zu] = (%f, %f) is Negative!", i, node._x, node._y);
      }
      isValid = false;
    }
    if(mustBeDecreasing && (i != 0) && (node._y > lastY)) {
      if(warnOnErrors) {
        PRINT_NAMED_WARNING("VerifyDecayGraph.IncreasingYNode", "Node[%zu] = (%f, %f) is has y > than previous (%f)", i, node._x, node._y, lastY);
      }
      isValid = false;
    }
    
    lastY = node._y;
  }
  
  return isValid;
}


void MoodDecayEvaulator::SetDecayGraph(const Util::GraphEvaluator2d& newGraph, DecayGraphType graphType)
{
  if( _decayGraph == nullptr ) {
    _decayGraph.reset(new Util::GraphEvaluator2d(newGraph));
  }
  else {
    *_decayGraph = newGraph;
  }
  _graphType = graphType;
}

bool MoodDecayEvaulator::Empty() const
{
  const bool emptyGraph = ( nullptr == _decayGraph ) || ( _decayGraph->GetNumNodes() == 0 );
  return emptyGraph;
}

float MoodDecayEvaulator::EvaluateDecay(float currentValue, float currentTimeSinceEvent_s, float deltaTime_s) const
{
  if( Empty() ) {
    PRINT_NAMED_ERROR("MoodDecayEvaulator.EvaluateDecay.Empty",
                      "trying to evaluate decay, but graph is empty! Returning the same value");
    return currentValue;
  }
  
  switch( _graphType ) {
    case DecayGraphType::TimeRatio: {
      const float prevDecayScalar = _decayGraph->EvaluateY(currentTimeSinceEvent_s);
      DEV_ASSERT_MSG(prevDecayScalar >= 0.0f,
                     "MoodDecayEvaulator.EvaluateDecay.NegativePrevDecay",
                     "Previous decay is %f needs to be nonnegative",
                     prevDecayScalar);

      const float newTime = currentTimeSinceEvent_s + deltaTime_s;
      const float newDecayScalar = _decayGraph->EvaluateY(newTime);
      DEV_ASSERT_MSG(prevDecayScalar >= 0.0f,
                     "MoodDecayEvaulator.EvaluateDecay.NegativeNewDecay",
                     "New decay is %f needs to be nonnegative",
                     prevDecayScalar);

      const float deltaScalar = Util::IsFltGT(prevDecayScalar, 0.0f) ? (newDecayScalar / prevDecayScalar) : newDecayScalar;
      const float newValue = currentValue * deltaScalar;
      return newValue;
    }

    case DecayGraphType::ValueSlope: {
      const float ratePerMinute = _decayGraph->EvaluateY( currentValue );
    
      if( Util::IsNearZero(ratePerMinute) ) {
        // no decay
        return currentValue;
      }
      else {        
        DEV_ASSERT_MSG(ratePerMinute > 0.0f, "Emotion.Update.NegativeRate",
                       "Additional decay rate is %f, but should be nonnegative",
                       ratePerMinute);

        const float deltaThisUpdate = -ratePerMinute * ( deltaTime_s / 60.0f );
        const float newValue = currentValue + deltaThisUpdate;
        if( std::signbit(newValue) == std::signbit(currentValue) ) {
          // apply delta
          return newValue;
        }
        else {
          // don't cross zero
          return 0.0f;
        }
      }
    }
  }
}

}
}

