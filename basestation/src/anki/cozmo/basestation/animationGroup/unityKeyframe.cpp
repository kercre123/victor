/**
 * File: unityKeyframe.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Implimentation of Unity's Keyframe:
 *    http://docs.unity3d.com/ScriptReference/Keyframe.html
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/animationGroup/unityKeyframe.h"
#include "util/logging/logging.h"
//#include <cassert>

#define DEBUG_ANIMATIONS 0

namespace Anki {
  namespace Cozmo {
    
    UnityKeyframe::UnityKeyframe()
    {
    }
    
    Result UnityKeyframe::DefineFromJson(Json::Value &jsonRoot)
    {
      if(jsonRoot.isMember("inTangent")) {
        inTangent = jsonRoot["inTangent"].asFloat();
      }
      if(jsonRoot.isMember("outTangent")) {
        inTangent = jsonRoot["outTangent"].asFloat();
      }
      if(jsonRoot.isMember("time")) {
        inTangent = jsonRoot["time"].asFloat();
      }
      if(jsonRoot.isMember("value")) {
        inTangent = jsonRoot["value"].asFloat();
      }
      
      return RESULT_OK;
    }
    
  } // namespace Cozmo
} // namespace Anki
