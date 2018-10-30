/**
 * File: backpackLightAnimationContainer.cpp
 *
 * Authors: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Container for json-defined backpack light animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/backpackLights/backpackLightAnimationContainer.h"

#include "coretech/common/engine/colorRGBA.h"
#include "cozmoAnim/backpackLights/animBackpackLightComponent.h"
#include "util/logging/DAS.h"

namespace Anki {
namespace Vector {

  namespace {
    void ShittyDebug(const char* str)
    {
      DASMSG(shitty_debug,
             "shitty_debug",
             "blah blah3");
      DASMSG_SET(s1, str, "debug");
      DASMSG_SEND();
    }
  }
  

BackpackLightAnimationContainer::BackpackLightAnimationContainer(const InitMap& initializationMap)
{
  for(const auto& pair: initializationMap){
    BackpackLightAnimation::BackpackAnimation animation;
    if(BackpackLightAnimation::DefineFromJSON(pair.second, animation)){
      const bool mustHaveExtension = true;
      const bool removeExtension = true;
      auto animName = Util::FileUtils::GetFileName(pair.first, mustHaveExtension, removeExtension);
      AddAnimation(animName, std::move(animation));
    }else{
      ShittyDebug(("could not parse " + pair.first).c_str());
      PRINT_NAMED_ERROR("BackpackLightAnimationContainer.Constructor.FailedToParseJSON",
                        "Failed to parse JSON for file %s",
                        pair.first.c_str());
    }
  }
}

template <class T>
T JsonColorValueToArray(const Json::Value& value)
{
  T arr;
  DEV_ASSERT(arr.size() == value.size(),
             "BackpackLightAnimationContainer.JsonColorValueToArray.DiffSizes");
  for(u8 i = 0; i < (int)arr.size(); ++i)
  {
    
    ColorRGBA color(value[i][0].asFloat(),
                    value[i][1].asFloat(),
                    value[i][2].asFloat(),
                    value[i][3].asFloat());
    arr[i] = color.AsRGBA();
  }
  return arr;
}

void BackpackLightAnimationContainer::AddAnimation(const std::string& animationName, const BackpackLightAnimation::BackpackAnimation&& anim)
{
  _animations.emplace(animationName, std::move(anim));
}

const BackpackLightAnimation::BackpackAnimation* BackpackLightAnimationContainer::GetAnimation(const std::string& name) const
{
  BackpackLightAnimation::BackpackAnimation* animPtr = nullptr;
  auto retVal = _animations.find(name);
  if(retVal == _animations.end()) {
    ShittyDebug(("Animation requested for unknown animation " + name).c_str());
    PRINT_NAMED_ERROR("BackpackLightAnimationContainer.GetAnimation_Const.InvalidName",
                      "Animation requested for unknown animation '%s'.",
                      name.c_str());
  } else {
    animPtr = const_cast<BackpackLightAnimation::BackpackAnimation*>(&retVal->second);
  }
  
  return animPtr;
}

} // namespace Vector
} // namespace Anki
