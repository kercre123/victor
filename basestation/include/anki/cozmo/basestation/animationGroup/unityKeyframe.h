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


#ifndef ANKI_COZMO_UNITY_KEYFRAME_H
#define ANKI_COZMO_UNITY_KEYFRAME_H

#include "anki/common/basestation/jsonTools.h"
#include <list>

namespace Anki {
  namespace Cozmo {
    
    class UnityKeyframe
    {
    public:
      
      UnityKeyframe();
      
      // For animation curve from file
      Result DefineFromJson(Json::Value& json);
      
      float inTangent;
      
      float outTangent;
      
      float time;
      
      float value;
      
    }; // class AnimationGroupEntry
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_UNITY_KEYFRAME_H
