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

#ifndef __Engine_MoodSystem_MoodDecayEvaluator_H__
#define __Engine_MoodSystem_MoodDecayEvaluator_H__

#include "json/json-forwards.h"
#include <memory>

namespace Anki {

namespace Util {
class GraphEvaluator2d;
}

namespace Cozmo {

class MoodDecayEvaulator
{
public:

  enum class DecayGraphType {
    // ratio of value based on time in seconds since the event
    TimeRatio,

    // flat value of decay per minute based on the current emotion value
    ValueSlope
  };

  MoodDecayEvaulator();
  ~MoodDecayEvaulator();

  // Return true if successful
  bool ReadFromJson(const Json::Value& inJson);
  bool WriteToJson(Json::Value& outJson) const;

  void SetDecayGraph(const Util::GraphEvaluator2d& newGraph, DecayGraphType graphType);

  static bool VerifyDecayGraph(const Util::GraphEvaluator2d& newGraph, DecayGraphType graphType, bool warnOnErrors);

  // Take in the current value and time and return the _new_ emotion value. Will apply the correct graph
  // internally and won't cross 0 with decay
  float EvaluateDecay(float currentValue, float currentTimeSinceEvent_s, float deltaTime_s) const;

  // Return true if the evaluator is empty and has no value (e.g. was default constructed or given an empty
  // graph)
  bool Empty() const;
  
private:

  std::unique_ptr<Util::GraphEvaluator2d> _decayGraph;
  DecayGraphType _graphType;
};

}
}


#endif
