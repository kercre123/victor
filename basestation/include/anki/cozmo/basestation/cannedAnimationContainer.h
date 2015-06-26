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
#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/basestation/animation.h"
#include <map>
#include <vector>

namespace Anki {
namespace Cozmo {
  
  class CannedAnimationContainer
  {
  public:
    CannedAnimationContainer();
    
    Result DefineHardCoded(); // called at construction
    
    Result DefineFromJson(Json::Value& jsonRoot, std::vector<s32>& loadedAnims);
    
    Result AddAnimation(const std::string& name);
    
    Animation* GetAnimation(const std::string& name);
    
    AnimationID_t GetID(const std::string& name) const;
    
    // TODO: Add a way to ask how long an animation is
    //u16 GetLengthInMilliSeconds(const std::string& name) const;
    
    void Clear();
    
  private:
    
    std::map<std::string, std::pair<AnimationID_t, Animation> > _animations;
    
  }; // class CannedAnimationContainer
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
