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


#ifndef ANKI_COZMO_ANIMATION_GROUP_MOOD_CURVE_H
#define ANKI_COZMO_ANIMATION_GROUP_MOOD_CURVE_H

#include "anki/common/basestation/jsonTools.h"
#include "util/graphEvaluator/graphEvaluator2d.h"

namespace Anki {
  namespace Cozmo {
    
    class AnimationGroupMoodCurve
    {
    public:
      
      AnimationGroupMoodCurve();
      
      // For reading animation groups from files
      Result DefineFromJson(Json::Value& json);
      
      // Retrieve an animation based on a set of moods
      float Evaluate(float mood) const;
      
      int GetEmotion() const { return _emotion; }
      
    private:
      
      // Emotion this Curve is associated with
      int _emotion;
      
      // The animation curve for this animation
      Util::GraphEvaluator2d _graphEvaluator;
      
    }; // class AnimationGroupMoodCurve
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIMATION_GROUP_MOOD_CURVE_H
