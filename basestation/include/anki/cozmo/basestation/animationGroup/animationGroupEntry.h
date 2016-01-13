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


#ifndef __Cozmo_Basestation_AnimationGroup_AnimationGroupEntry_H__
#define __Cozmo_Basestation_AnimationGroup_AnimationGroupEntry_H__

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/moodSystem/moodScorer.h"
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
      
      MoodScorer _moodScorer;
      
    }; // class AnimationGroupEntry
  } // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_AnimationGroup_AnimationGroupEntry_H__
