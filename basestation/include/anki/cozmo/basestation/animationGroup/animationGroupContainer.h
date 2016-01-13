/**
 * File: animationGroupContainer.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-12
 *
 * Description: Container for hard-coded or json-defined animations groups
 *              used to determine which animations to send to cozmo
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_AnimationGroup_AnimationGroupContainer_H__
#define __Cozmo_Basestation_AnimationGroup_AnimationGroupContainer_H__

#include "anki/cozmo/basestation/animationGroup/animationGroup.h"
#include <unordered_map>
#include <vector>

// Forward declaration
namespace Json {
  class Value;
}

namespace Anki {
  namespace Cozmo {
    
    class AnimationGroupContainer
    {
    public:
      AnimationGroupContainer();

      Result DefineFromJson(Json::Value& jsonRoot, const std::string& animationGroupName);
      
      Result AddAnimationGroup(const std::string& name);
      
      AnimationGroup* GetAnimationGroup(const std::string& name);
      const AnimationGroup* GetAnimationGroup(const std::string& name) const;
      
      std::vector<std::string> GetAnimationGroupNames();
      
      void Clear();
      
    private:
      
      std::unordered_map<std::string, AnimationGroup> _animationGroups;
      
    }; // class AnimationGroupContainer
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_AnimationGroup_AnimationGroupContainer_H__
