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

#include "coretech/common/engine/jsonTools.h"
#include "cannedAnimLib/animation.h"
#include <unordered_map>
#include <vector>

namespace CozmoAnim {
  struct AnimClip;
}

namespace Anki {
namespace Cozmo {
  class SpriteSequenceContainer;
  
  class CannedAnimationContainer
  {
  public:
    
    CannedAnimationContainer(SpriteSequenceContainer* spriteSequenceContainer);
    
    Result DefineHardCoded(); // called at construction
    
    Result DefineFromJson(const Json::Value& jsonRoot, std::string& loadedAnimName);

    Result DefineFromFlatBuf(const CozmoAnim::AnimClip* animClip, std::string& animName);

    Animation* GetAnimationWrapper(std::string& animationName);

    Result SanityCheck(Result lastResult, Animation* animation, std::string& animationName);
    
    Result AddAnimation(const std::string& name);
    Result AddAnimation(Animation* animation);
    
    Animation* GetAnimation(const std::string& name);
    const Animation* GetAnimation(const std::string& name) const;
    bool  HasAnimation(const std::string& name) const;
    
    std::vector<std::string> GetAnimationNames();
    
    void Clear();
    
  private:
    SpriteSequenceContainer* _spriteSequenceContainer = nullptr;
    std::unordered_map<std::string, Animation> _animations;

    void SetSpriteSequenceContainerOnAnimation(Animation* animation) const;
    
  }; // class CannedAnimationContainer
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
