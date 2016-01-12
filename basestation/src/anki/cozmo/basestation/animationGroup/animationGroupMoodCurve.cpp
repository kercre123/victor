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
      
      if(!jsonRoot.isMember("Curve")) {
        PRINT_NAMED_ERROR("AnimationGroupMoodCurve.DefineFromJson.NoCurve",
                          "Missing 'Curve' field for mood curve.");
        return RESULT_FAIL;
      }
      
      Json::Value& jsonAnimationCurve = jsonRoot["Curve"];
      
      auto addResult = _curve.DefineFromJson(jsonAnimationCurve);
      
      return addResult;
    }
    
    
    float AnimationGroupMoodCurve::Evaluate(float mood) const {
      return _curve.Evaluate(mood);
    }
    
  } // namespace Cozmo
} // namespace Anki
