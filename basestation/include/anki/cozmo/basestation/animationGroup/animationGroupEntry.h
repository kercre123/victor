/**
 * File: animationGroupEntry.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing an animation selection
 *    Which defines a set of mood score graphs
 *    by which to evaluate the suitability of this animation
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_AnimationGroup_AnimationGroupEntry_H__
#define __Cozmo_Basestation_AnimationGroup_AnimationGroupEntry_H__

#include "clad/types/simpleMoodTypes.h"
#include "anki/common/types.h"

// Forward Declaration
namespace Json {
  class Value;
}

namespace Anki {
  namespace Cozmo {
    
    // Forward Declaration
    class MoodManager;
    
    class AnimationGroupEntry
    {
    public:
      
      AnimationGroupEntry();
      
      // For reading animation groups from files
      Result DefineFromJson(const Json::Value& json);
      
      const std::string& GetName() const { return _name; }
      f32 GetWeight() const { return _weight; }
      SimpleMoodType  GetMood() const { return _mood; }
      
      double GetCooldown() const { return _cooldownTime_s; }
      
    private:
      
      // Name of this animation
      std::string _name;
      double _cooldownTime_s;
      f32 _weight;
      SimpleMoodType _mood;
      
    }; // class AnimationGroupEntry
  } // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_AnimationGroup_AnimationGroupEntry_H__
