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

#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
  CannedAnimationContainer::CannedAnimationContainer()
  {
    DefineHardCoded();
   
    // Add special animation for procedural animation:
    AddAnimation(FaceAnimationManager::ProceduralAnimName);
    
    Animation* anim = GetAnimation(FaceAnimationManager::ProceduralAnimName);
    assert(anim != nullptr);
    FaceAnimationKeyFrame kf(FaceAnimationManager::ProceduralAnimName);
    if(RESULT_OK != anim->AddKeyFrameToBack(kf))
    {
      PRINT_NAMED_ERROR("CannedAnimationContainer.Constructor.AddProceduralFailed",
                        "Failed to add keyframe to procedural animation.");
    }
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
      // Don't include procedural animation name in list of available animations
      if (i->first != FaceAnimationManager::ProceduralAnimName) {
        v.push_back(i->first);
      }
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
    PRINT_NAMED_DEBUG("CannedAnimationContainer::DefineFromJson", "Loading '%s'", animationName.c_str());

    Animation* animation = GetAnimationWrapper(animationName);
    if(animation == nullptr) {
      return RESULT_FAIL;
    }
    Result lastResult = animation->DefineFromJson(animationName, jsonRoot[animationName]);

    return SanityCheck(lastResult, animation, animationName);
    
  } // CannedAnimationContainer::DefineFromJson()

  Animation* CannedAnimationContainer::GetAnimationWrapper(std::string& animationName)
  {
    if(animationName == FaceAnimationManager::ProceduralAnimName) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.ReservedName",
                        "Skipping animation with reserved name '%s'.",
                        FaceAnimationManager::ProceduralAnimName.c_str());
      return nullptr;
    }
    
    if(RESULT_OK != AddAnimation(animationName)) {
      PRINT_NAMED_INFO("CannedAnimationContainer.DefineFromJson.ReplaceName",
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

} // namespace Cozmo
} // namespace Anki
