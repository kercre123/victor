/**
 * File: animationGroup.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing a group of animations,
 *    from which an animation can be selected
 *    for a given set of moods
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_AnimationGroup_AnimationGroup_H__
#define __Cozmo_Basestation_AnimationGroup_AnimationGroup_H__

#include "anki/cozmo/basestation/animationGroup/animationGroupEntry.h"
#include <vector>

// Forward declaration
namespace Json {
  class Value;
}

namespace Anki {
  namespace Cozmo {
    
    class AnimationGroup
    {
    public:
      
      explicit AnimationGroup(const std::string& name = "");
      
      // For reading animation groups from files
      Result DefineFromJson(const std::string& name, Json::Value& json);
      
      // Retrieve an animation based on a set of moods
      const std::string& GetAnimationName(const MoodManager& moodManager) const;
      
      // An animation group is empty if it has no animations
      bool IsEmpty() const;
      
      const std::string& GetName() const { return _name; }
      
    private:
      
      // Name of this animation
      std::string _name;
      
      std::vector<AnimationGroupEntry> _animations;
      
    }; // class AnimationGroup
  } // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_AnimationGroup_AnimationGroup_H__
