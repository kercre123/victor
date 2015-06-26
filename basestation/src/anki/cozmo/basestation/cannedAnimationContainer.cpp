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

#include "anki/util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
  CannedAnimationContainer::CannedAnimationContainer()
  {
    DefineHardCoded();
  }
  
  Result CannedAnimationContainer::AddAnimation(const std::string& name)
  {
    Result lastResult = RESULT_OK;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      _animations.emplace(name,Animation());
    }
    
    return lastResult;
  }

  Animation* CannedAnimationContainer::GetAnimation(const std::string& name)
  {
    Animation* animPtr = nullptr;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.GetAnimation.InvalidName",
                        "Animation requested for unknown animation '%s'.\n",
                        name.c_str());
    } else {
      animPtr = &retVal->second;
    }
    
    return animPtr;
  }

  Result CannedAnimationContainer::DefineFromJson(Json::Value& jsonRoot, std::string& loadedAnimName)
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
    
    for(auto const& animationName : animationNames)
    {
      if(RESULT_OK != AddAnimation(animationName)) {
        PRINT_NAMED_INFO("CannedAnimationContainer.DefineFromJson.ReplaceName",
                          "Replacing existing animation named '%s'.\n",
                          animationName.c_str());
      }
      
      Animation* animation = GetAnimation(animationName);
      if(animation == nullptr) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Could not GetAnimation named '%s'.",
                          animationName.c_str());
        return RESULT_FAIL;
      }
      
      Result lastResult = animation->DefineFromJson(animationName,
                                                    jsonRoot[animationName]);
      
      // Sanity check
      if(animation->GetName() != animationName) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Animation's internal name ('%s') doesn't match container's name for it ('%s').\n",
                          animation->GetName().c_str(),
                          animationName.c_str());
        return RESULT_FAIL;
      }
      
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Failed to define animation '%s' from Json.\n",
                          animationName.c_str());
        return lastResult;
      }


      loadedAnimName = animationName;
    } // for each animation
    
    return RESULT_OK;
  } // CannedAnimationContainer::DefineFromJson()
  
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