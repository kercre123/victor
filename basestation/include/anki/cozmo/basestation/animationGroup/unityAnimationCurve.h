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


#ifndef ANKI_COZMO_UNITY_ANIMATION_CURVE_H
#define ANKI_COZMO_UNITY_ANIMATION_CURVE_H

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/animationGroup/unityKeyframe.h"
#include <list>

namespace Anki {
    namespace Cozmo {
        
        class UnityAnimationCurve
        {
        public:
            
            UnityAnimationCurve();
            
            // For animation curve from file
            Result DefineFromJson(Json::Value& json);
            
            // Evaluate the curve
            float Evaluate(float time) const;
            
        private:
            
            std::list<UnityKeyframe> _keys;
            
        }; // class AnimationGroupEntry
    } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_UNITY_ANIMATION_CURVE_H
