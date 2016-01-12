/**
 * File: unityAnimationCurve.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Implimentation of Unity's AnimationCurve:
 *    http://docs.unity3d.com/ScriptReference/AnimationCurve.html
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/animationGroup/unityAnimationCurve.h"
#include "util/logging/logging.h"
//#include <cassert>

#define DEBUG_ANIMATIONS 0

namespace Anki {
  namespace Cozmo {
    
    UnityAnimationCurve::UnityAnimationCurve()
    {
    }
    
    Result UnityAnimationCurve::DefineFromJson(Json::Value &jsonRoot)
    {
      if(jsonRoot.isMember("keys")) {
        
        Json::Value& jsonKeyframes = jsonRoot["keys"];
        
        
        const s32 numEntries = jsonKeyframes.size();
        for(s32 i = 0; i < numEntries; ++i)
        {
          Json::Value& jsonKeyframe = jsonKeyframes[i];
          
          _keys.push_back(UnityKeyframe());
          
          auto addResult = _keys.back().DefineFromJson(jsonKeyframe);
          
          if(addResult != RESULT_OK) {
            PRINT_NAMED_ERROR("UnityAnimationCurve.DefineFromJson.AddKeyframeFailure",
                              "Adding keyframe %d failed.",
                              i);
            return addResult;
          }
          
        } // for each Keyframe
      }
      
      
      return RESULT_OK;
    }
    
    float evaluateBezier(float a, float b, float c, float d, float t) {
      float ab = a * t + b * (1 - t);
      float bc = b * t + c * (1 - t);
      float cd = c * t + d * (1 - t);
      float abbc = ab * t + bc * (1 - t);
      float bccd = bc * t + cd * (1 - t);
      return abbc * t + bccd * (1 - t);
    }
    
    float UnityAnimationCurve::Evaluate(float time) const {
      
      if(_keys.size() == 0) {
        return 0;
      }
      if(_keys.size() == 1) {
        return _keys.front().value;
      }
      
      auto b = _keys.begin();
      for(auto a = b++; b != _keys.end(); a = b++ ) {
        if(time < a->time) {
          return a->value;
        }
        
        if(time <= b->time) {
          float delta = b->time - a->time;
          
          if(delta <= 0) {
            return b->value;
          }
          
          float t = (time - a->time) / delta;
          
          float a1 = a->value + a->outTangent * delta * 0.3333333;
          float b1 = b->value - b->inTangent * delta * 0.33333333;
          
          return evaluateBezier(a->value, a1, b1, b->value, t);
        }
      }
      
      return _keys.back().value;
    }
    
    
    
  } // namespace Cozmo
} // namespace Anki
