/**
 * File: cannedAnimationContainer.cpp
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

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "cannedAnimLib/spriteSequences/spriteSequenceContainer.h"
#include "cannedAnimLib/baseTypes/track.h"

#include "util/helpers/boundedWhile.h"
#include "util/logging/logging.h"

#define LOG_CHANNEL "Animations"

namespace Anki {
namespace Cozmo {

  const std::string CannedAnimationContainer::ProceduralAnimName("_PROCEDURAL_");
  
  CannedAnimationContainer::CannedAnimationContainer(const CannedAnimLib::SpritePathMap* spriteMap, 
                                                     SpriteSequenceContainer* spriteSequenceContainer)
  : _spriteMap(spriteMap)
  , _spriteSequenceContainer(spriteSequenceContainer)
  {
    DefineHardCoded();
  }

  Result CannedAnimationContainer::AddAnimation(const std::string& name)
  {
    Result lastResult = RESULT_OK;

    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      _animations.emplace(name,Animation(name));
    }

    return lastResult;
  }
  
  Result CannedAnimationContainer::AddAnimation(Animation* animation)
  {
    Result lastResult = RESULT_OK;
    if (animation == nullptr) {
      return RESULT_FAIL;
    }
    const std::string& name = animation->GetName();

    // Replace animation with the given one because this
    // is mainly for animators testing new animations
    auto iter = _animations.find(name);
    if(iter != _animations.end()) {
      _animations.erase(iter);
    }
    _animations.emplace(name, *animation);
    
    return lastResult;
  }

  Animation* CannedAnimationContainer::GetAnimation(const std::string& name)
  {
    const Animation* animPtr = const_cast<const CannedAnimationContainer *>(this)->GetAnimation(name);
    return const_cast<Animation*>(animPtr);
  }
  
  const Animation* CannedAnimationContainer::GetAnimation(const std::string& name) const
  {
    const Animation* animPtr = nullptr;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.GetAnimation_Const.InvalidName",
                        "Animation requested for unknown animation '%s'.",
                        name.c_str());
    } else {
      animPtr = &retVal->second;
    }
    
    return animPtr;
  }
  
  bool CannedAnimationContainer::HasAnimation(const std::string& name) const
  {
    auto retVal = _animations.find(name);
    return retVal != _animations.end();
  }
  
  std::vector<std::string> CannedAnimationContainer::GetAnimationNames()
  {
    std::vector<std::string> v;
    v.reserve(_animations.size());
    for (std::unordered_map<std::string, Animation>::iterator i=_animations.begin(); i != _animations.end(); ++i) {
      v.push_back(i->first);
    }
    return v;
  }

  Result CannedAnimationContainer::DefineFromFlatBuf(const CozmoAnim::AnimClip* animClip, std::string& animName)
  {
    Animation* animation = GetAnimationWrapper(animName);
    if(animation == nullptr) {
      return RESULT_FAIL;
    }
    Result lastResult = animation->DefineFromFlatBuf(animName, animClip);
    SetSpriteSequenceContainerOnAnimation(animation);

    return SanityCheck(lastResult, animation, animName);

  } // CannedAnimationContainer::DefineFromFlatBuf()
  
  Result CannedAnimationContainer::DefineFromJson(const Json::Value& jsonRoot, std::string& animationName)
  {

    Json::Value::Members animationNames = jsonRoot.getMemberNames();
    
    /*
    // Add _all_ the animations first to register the IDs, so Trigger keyframes
    // which specify another animation's name will still work (because that
    // name should already exist, no matter the order the Json is parsed)
    for(auto const& animationName : animationNames) {
      AddAnimation(animationName);
    }
    */
    
    if(animationNames.empty()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.EmptyFile",
                        "Found no animations in JSON");
      return RESULT_FAIL;
    } else if(animationNames.size() != 1) {
      PRINT_NAMED_WARNING("CannedAnimationContainer.DefineFromJson.TooManyAnims",
                          "Expecting only one animation per json file, found %lu. "
                          "Will use first: %s",
                          (unsigned long)animationNames.size(), animationNames[0].c_str());
    }
    
    animationName = animationNames[0];

    PRINT_CH_DEBUG(LOG_CHANNEL, "CannedAnimationContainer::DefineFromJson", "Loading '%s'", animationName.c_str());

    Animation* animation = GetAnimationWrapper(animationName);
    if(animation == nullptr) {
      return RESULT_FAIL;
    }
    Result lastResult = animation->DefineFromJson(animationName, jsonRoot[animationName]);
    SetSpriteSequenceContainerOnAnimation(animation);

    return SanityCheck(lastResult, animation, animationName);
    
  } // CannedAnimationContainer::DefineFromJson()

  Animation* CannedAnimationContainer::GetAnimationWrapper(std::string& animationName)
  {
    if(animationName == CannedAnimationContainer::ProceduralAnimName) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.ReservedName",
                        "Skipping animation with reserved name '%s'.",
                        CannedAnimationContainer::ProceduralAnimName.c_str());
      return nullptr;
    }
    
    if(RESULT_OK != AddAnimation(animationName)) {
      PRINT_CH_DEBUG(LOG_CHANNEL, "CannedAnimationContainer.DefineFromJson.ReplaceName",
                     "Replacing existing animation named '%s'.",
                     animationName.c_str());
    }
    
    Animation* animation = GetAnimation(animationName);
    if(animation == nullptr) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                        "Could not GetAnimation named '%s'.",
                        animationName.c_str());
      return nullptr;
    }

    return animation;
  } // CannedAnimationContainer::GetAnimationWrapper()

  Result CannedAnimationContainer::SanityCheck(Result lastResult, Animation* animation, std::string& animationName)
  {
    if(animation->GetName() != animationName) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                        "Animation's internal name ('%s') doesn't match container's name for it ('%s').",
                        animation->GetName().c_str(),
                        animationName.c_str());
      return RESULT_FAIL;
    }
    
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                        "Failed to define animation '%s' from Json.",
                        animationName.c_str());
      return lastResult;
    }

    return RESULT_OK;
  } // CannedAnimationContainer::SanityCheck()
  
  Result CannedAnimationContainer::DefineHardCoded()
  {
    return RESULT_OK;

  } // DefineHardCodedAnimations()
  
  void CannedAnimationContainer::Clear()
  {
    _animations.clear();
  } // Clear()

  void CannedAnimationContainer::SetSpriteSequenceContainerOnAnimation(Animation* animation) const
  {
    auto& spriteSeqTrack = animation->GetTrack<SpriteSequenceKeyFrame>();
    const auto& maxCount = Animations::Track<SpriteSequenceKeyFrame>::ConstMaxFramesPerTrack();
    BOUNDED_WHILE(maxCount, spriteSeqTrack.HasFramesLeft()){
      auto& keyframe = spriteSeqTrack.GetCurrentKeyFrame();
      keyframe.GiveKeyframeImageAccess(_spriteMap, _spriteSequenceContainer);
      spriteSeqTrack.MoveToNextKeyFrame();
    }
    spriteSeqTrack.MoveToStart();
  }


} // namespace Cozmo
} // namespace Anki
