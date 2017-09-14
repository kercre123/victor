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
#include "cozmoAnim/animation/animation.h"
#include <unordered_map>
#include <vector>

namespace CozmoAnim {
  struct AnimClip;
}

namespace Anki {
namespace Cozmo {
  
  class CannedAnimationContainer
  {
  public:
    using AnimIDToNameMap_t = std::unordered_map<u32, std::string>;
    using AnimNameToIDMap_t = std::unordered_map<std::string, u32>;
    
    CannedAnimationContainer();
    
    Result DefineHardCoded(); // called at construction
    
    Result DefineFromJson(const Json::Value& jsonRoot, std::string& loadedAnimName);

    Result DefineFromFlatBuf(const CozmoAnim::AnimClip* animClip, std::string& animName);

    Animation* GetAnimationWrapper(std::string& animationName);

    Result SanityCheck(Result lastResult, Animation* animation, std::string& animationName);
    
    Result AddAnimation(const std::string& name);
    
    Animation* GetAnimation(const std::string& name);
    const Animation* GetAnimation(const std::string& name) const;
    bool  HasAnimation(const std::string& name) const;
    
    std::vector<std::string> GetAnimationNames();
    
    bool GetAnimNameByID(u32 animID, std::string& animName)        const;
    bool GetAnimIDByName(const std::string& animName, u32& animID) const;
    
    const AnimIDToNameMap_t& GetAnimationIDToNameMap() const { return _animIDToNameMap; }
    const AnimNameToIDMap_t& GetAnimationNameToIDMap() const { return _animNameToIDMap; }
    
    void Clear();
    
  private:
    
    std::unordered_map<std::string, Animation> _animations;
    
    // Map of unique ID to animation string name and vice versa
    AnimIDToNameMap_t _animIDToNameMap;
    AnimNameToIDMap_t _animNameToIDMap;
    
    // AnimID counter
    u32 _animIDCtr;
    
  }; // class CannedAnimationContainer
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
