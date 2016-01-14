/**
 * File: animationGroupContainer.cpp
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

#include "anki/cozmo/basestation/animationGroup/animationGroupContainer.h"
#include "util/logging/logging.h"

namespace Anki {
  namespace Cozmo {
    
    AnimationGroupContainer::AnimationGroupContainer()
    {
    }
    
    Result AnimationGroupContainer::AddAnimationGroup(const std::string& name)
    {
      Result lastResult = RESULT_OK;
      
      auto retVal = _animationGroups.find(name);
      if(retVal == _animationGroups.end()) {
        _animationGroups.emplace(name,AnimationGroup(name));
      }
      
      return lastResult;
    }
    
    AnimationGroup* AnimationGroupContainer::GetAnimationGroup(const std::string& name)
    {
      const AnimationGroup* animGroupPtr = const_cast<const AnimationGroupContainer *>(this)->GetAnimationGroup(name);
      return const_cast<AnimationGroup*>(animGroupPtr);
    }
    
    const AnimationGroup* AnimationGroupContainer::GetAnimationGroup(const std::string& name) const
    {
      const AnimationGroup* animPtr = nullptr;
      
      auto retVal = _animationGroups.find(name);
      if(retVal == _animationGroups.end()) {
        PRINT_NAMED_ERROR("AnimationGroupContainer.GetAnimationGroup_Const.InvalidName",
                          "AnimationGroup requested for unknown animation group '%s'.\n",
                          name.c_str());
      } else {
        animPtr = &retVal->second;
      }
      
      return animPtr;
    }
    
    std::vector<std::string> AnimationGroupContainer::GetAnimationGroupNames()
    {
      std::vector<std::string> v;
      v.reserve(_animationGroups.size());
      for (std::unordered_map<std::string, AnimationGroup>::iterator i=_animationGroups.begin(); i != _animationGroups.end(); ++i) {
        v.push_back(i->first);
      }
      return v;
    }
    
    
    Result AnimationGroupContainer::DefineFromJson(const Json::Value& jsonRoot, const std::string& animationGroupName)
    {
      
      if(RESULT_OK != AddAnimationGroup(animationGroupName)) {
        PRINT_NAMED_INFO("AnimationGroupContainer.DefineAnimationGroupFromJson.ReplaceName",
                         "Replacing existing animation group named '%s'.\n",
                         animationGroupName.c_str());
      }
      
      AnimationGroup* animationGroup = GetAnimationGroup(animationGroupName);
      if(animationGroup == nullptr) {
        PRINT_NAMED_ERROR("AnimationGroupContainer.DefineAnimationGroupFromJson",
                          "Could not GetAnimationGroup named '%s'.",
                          animationGroupName.c_str());
        return RESULT_FAIL;
      }
      
      Result result = animationGroup->DefineFromJson(animationGroupName,
                                                     jsonRoot);
      
      
      if(result != RESULT_OK) {
        PRINT_NAMED_ERROR("AnimationGroupContainer.DefineAnimationGroupFromJson",
                          "Failed to define animation group '%s' from Json.\n",
                          animationGroupName.c_str());
      }
      
      return result;
    } // AnimationGroupContainer::DefineAnimationGroupFromJson()
    
    
    void AnimationGroupContainer::Clear()
    {
      _animationGroups.clear();
    } // Clear()
    
  } // namespace Cozmo
} // namespace Anki