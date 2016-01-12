/**
 * File: animationGroupMoodCurve.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing an curve defined
 *    for a specific mode for an animation.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/animationGroup/animationGroupMoodCurve.h"
#include "util/logging/logging.h"
//#include <cassert>

#define DEBUG_ANIMATIONS 0

namespace Anki {
  namespace Cozmo {
    
    AnimationGroupMoodCurve::AnimationGroupMoodCurve()
    {
    }
    
    Result AnimationGroupMoodCurve::DefineFromJson(Json::Value &jsonRoot)
    {
      if(!jsonRoot.isMember("Emotion")) {
        PRINT_NAMED_ERROR("AnimationGroupMoodCurve.DefineFromJson.NoEmotion",
                          "Missing 'Emotion' field for mood curve.");
        return RESULT_FAIL;
      }
      
      _emotion = jsonRoot["Emotion"].asInt();
      
      if(!jsonRoot.isMember("GraphEvaluator")) {
        PRINT_NAMED_ERROR("AnimationGroupMoodCurve.DefineFromJson.NoGraphEvaluator",
                          "Missing 'GraphEvaluator' field for mood graph.");
        return RESULT_FAIL;
      }
      
      Json::Value& jsonAnimationCurve = jsonRoot["GraphEvaluator"];
      
      return _graphEvaluator.ReadFromJson(jsonAnimationCurve) ? RESULT_OK : RESULT_FAIL;
    }
    
    
    float AnimationGroupMoodCurve::Evaluate(float mood) const {
      return _graphEvaluator.EvaluateY(mood);
    }
    
  } // namespace Cozmo
} // namespace Anki
