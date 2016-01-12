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


#ifndef ANKI_COZMO_ANIMATION_GROUP_CONTAINER_H
#define ANKI_COZMO_ANIMATION_GROUP_CONTAINER_H

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/animationGroup/animationGroup.h"
#include <unordered_map>
#include <vector>

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

#endif // ANKI_COZMO_ANIMATION_GROUP_CONTAINER_H
