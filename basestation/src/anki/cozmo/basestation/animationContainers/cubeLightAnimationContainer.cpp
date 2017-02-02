/**
 * File: cubeLightAnimationContainer.cpp
 *
 * Authors: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Container for json-defined cube light animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/cozmo/basestation/animationContainers/cubeLightAnimationContainer.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"

namespace Anki {
namespace Cozmo {

bool CubeLightAnimationContainer::ParseJsonToPattern(const Json::Value& json, LightPattern& pattern)
{
  ObjectLights values;
  
  bool res = true;
  res &= JsonTools::GetColorValuesToArrayOptional(json, "onColors",  values.onColors);
  res &= JsonTools::GetColorValuesToArrayOptional(json, "offColors", values.offColors);
  res &= JsonTools::GetArrayOptional(json, "onPeriod_ms",            values.onPeriod_ms);
  res &= JsonTools::GetArrayOptional(json, "offPeriod_ms",           values.offPeriod_ms);
  res &= JsonTools::GetArrayOptional(json, "transitionOnPeriod_ms",  values.transitionOnPeriod_ms);
  res &= JsonTools::GetArrayOptional(json, "transitionOffPeriod_ms", values.transitionOffPeriod_ms);
  res &= JsonTools::GetArrayOptional(json, "offset",                 values.offset);
  res &= json.isMember("rotationPeriod_ms");
  
  // If any of the member fields are missing from the pattern definition return false
  if(!res)
  {
    return false;
  }
  
  values.rotationPeriod_ms = json["rotationPeriod_ms"].asUInt();
  values.makeRelative      = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
  values.relativePoint     = {0,0};
  
  pattern.lights = std::move(values);
  
  return true;
}

CubeAnimation* CubeLightAnimationContainer::GetAnimationHelper(const std::string& name) const
{
  CubeAnimation* animPtr = nullptr;
  
  auto retVal = _animations.find(name);
  if(retVal == _animations.end()) {
    PRINT_NAMED_ERROR("CubeLightAnimationContainer.GetAnimation_Const.InvalidName",
                      "Animation requested for unknown animation '%s'.",
                      name.c_str());
  } else {
    animPtr = const_cast<CubeAnimation*>(&retVal->second);
  }
  
  return animPtr;
}

const CubeAnimation* CubeLightAnimationContainer::GetAnimation(const std::string& name) const
{
  return GetAnimationHelper(name);
}

CubeAnimation* CubeLightAnimationContainer::GetAnimation(const std::string& name)
{
  return GetAnimationHelper(name);
}

Result CubeLightAnimationContainer::DefineFromJson(const Json::Value& jsonRoot, std::string& animationName)
{
  std::list<LightPattern> anim;
  for(const auto& member : jsonRoot.getMemberNames())
  {
    LightPattern pattern;
    const auto& jsonMember = jsonRoot[member];
    
    if(!jsonMember.isMember("pattern") ||
       !jsonMember.isMember("duration_ms"))
    {
      PRINT_NAMED_ERROR("CubeLightAnimationContainer.ParseJsonToAnim.InvalidJson",
                        "Missing member field in %s json file",
                        animationName.c_str());
      return RESULT_FAIL;
    }
    
    const bool res = ParseJsonToPattern(jsonMember["pattern"], pattern);
    if(!res)
    {
      PRINT_NAMED_ERROR("CubeLightAnimationContainer.ParseJsonToAnim.InvalidPattern",
                        "Missing member field in one of the pattern definitions in %s json file",
                        animationName.c_str());
      return RESULT_FAIL;
    }
    
    pattern.duration_ms = jsonMember["duration_ms"].asUInt();
    
    if(jsonMember.isMember("canBeOverridden"))
    {
      pattern.canBeOverridden = jsonMember["canBeOverridden"].asBool();
    }
    
    pattern.name = member;
    anim.push_back(std::move(pattern));
  }
  _animations[animationName] = anim;
  return RESULT_OK;
}
  
} // namespace Cozmo
} // namespace Anki
