/**
 * File: animationGroupEntry.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing an animation select
 *    Which defines a set of mood animation curves
 *    by which to evaluate the suitability of this animation
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef ANKI_COZMO_ANIMATION_GROUP_ENTRY_H
#define ANKI_COZMO_ANIMATION_GROUP_ENTRY_H

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/animationGroup/animationGroupMoodCurve.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include <list>

namespace Anki {
  namespace Cozmo {
    
    class AnimationGroupEntry
    {
    public:
      
      AnimationGroupEntry();
      
      // For reading animation groups from files
      Result DefineFromJson(Json::Value& json);
      
      // Evaluate this animation group entry based on the current mood.
      float Evaluate(const MoodManager& moodManager) const;
      
      const std::string& GetName() const { return _name; }
      
    private:
      
      // Name of this animation
      std::string _name;
      
      std::list<AnimationGroupMoodCurve> _moodCurves;
      
    }; // class AnimationGroupEntry
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIMATION_GROUP_ENTRY_H
