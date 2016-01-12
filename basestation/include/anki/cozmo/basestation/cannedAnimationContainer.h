/**
 * File: cannedAnimationContainer.h
 *
 * Authors: Andrew Stein
 * Created: 2014-10-22
 *
 * Description: Container for hard-coded or json-defined "canned" animations
 *              stored on the basestation and send-able to the physical robot.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
#define ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/animation/animation.h"
#include "anki/cozmo/basestation/animationGroup/animationGroup.h"
#include <unordered_map>
#include <vector>

namespace Anki {
namespace Cozmo {
  
  class CannedAnimationContainer
  {
  public:
    CannedAnimationContainer();
    
    Result DefineHardCoded(); // called at construction
    
    Result DefineFromJson(Json::Value& jsonRoot, std::string& loadedAnimName);
    
    Result DefineAnimationGroupFromJson(Json::Value& jsonRoot, const std::string& animationGroupName);
    
    Result AddAnimation(const std::string& name);
    
    Animation* GetAnimation(const std::string& name);
    const Animation* GetAnimation(const std::string& name) const;

    Result AddAnimationGroup(const std::string& name);
    
    AnimationGroup* GetAnimationGroup(const std::string& name);
    const AnimationGroup* GetAnimationGroup(const std::string& name) const;
    
    std::vector<std::string> GetAnimationNames();
    
    std::vector<std::string> GetAnimationGroupNames();
    
    void Clear();
    
  private:
    
    std::unordered_map<std::string, Animation> _animations;
      
    std::unordered_map<std::string, AnimationGroup> _animationGroups;
    
  }; // class CannedAnimationContainer
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
