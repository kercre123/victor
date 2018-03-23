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

#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/math/point_impl.h"
#include "engine/animations/animationContainers/cubeLightAnimationContainer.h"
#include "engine/components/cubes/cubeLightComponent.h"

namespace Anki {
namespace Cozmo {
  
namespace {
// Pattern Keys
static const char* kOnColorsKey            = "onColors";
static const char* kOffColorsKey           = "offColors";
static const char* kOnPeriodKey            = "onPeriod_ms";
static const char* kOffPeriodKey           = "offPeriod_ms";
static const char* kTransitionOnPeriodKey  = "transitionOnPeriod_ms";
static const char* kTransitionOffPeriodKey = "transitionOffPeriod_ms";
static const char* kOffsetKey              = "offset";
static const char* kRotationPeriodKey      = "rotationPeriod_ms";

// Sequence Keys
static const char* kDurationKey            = "duration_ms";
static const char* kPatternKey             = "pattern";
static const char* kCanBeOverriddenKey     = "canBeOverridden";
static const char* kPatternDebugNameKey    = "patternDebugName";

}

bool CubeLightAnimationContainer::ParseJsonToPattern(const Json::Value& json, LightPattern& pattern)
{
  ObjectLights values;
  
  bool res = true;
  res &= JsonTools::GetColorValuesToArrayOptional(json, kOnColorsKey,  values.onColors);
  res &= JsonTools::GetColorValuesToArrayOptional(json, kOffColorsKey, values.offColors);
  res &= JsonTools::GetArrayOptional(json, kOnPeriodKey,               values.onPeriod_ms);
  res &= JsonTools::GetArrayOptional(json, kOffPeriodKey,              values.offPeriod_ms);
  res &= JsonTools::GetArrayOptional(json, kTransitionOnPeriodKey,     values.transitionOnPeriod_ms);
  res &= JsonTools::GetArrayOptional(json, kTransitionOffPeriodKey,    values.transitionOffPeriod_ms);
  res &= JsonTools::GetArrayOptional(json, kOffsetKey,                 values.offset);
  res &= json.isMember(kRotationPeriodKey);
  
  // If any of the member fields are missing from the pattern definition return false
  if(!res)
  {
    return false;
  }
  
  values.rotationPeriod_ms = json[kRotationPeriodKey].asUInt();
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

Result CubeLightAnimationContainer::DefineFromJson(const Json::Value& jsonRoot)
{
  
  const Json::Value::Members animationNames = jsonRoot.getMemberNames();
  
  if(animationNames.empty()){
    PRINT_NAMED_ERROR("CubeLightAnimationContainer.DefineFromJson.EmptyFile",
                      "Found no animations in JSON");
    return RESULT_FAIL;
  }else if(animationNames.size() != 1){
    PRINT_NAMED_WARNING("CubeLightAnimationContainer.DefineFromJson.TooManyAnims",
                        "Expecting only one animation per json file, found %lu. "
                        "Will use first: %s",
                        (unsigned long)animationNames.size(), animationNames[0].c_str());
  }
  
  const std::string& animationName = animationNames[0];
  const Json::Value& patternContainer = jsonRoot[animationName];
  
  
  std::list<LightPattern> anim;

  const s32 numPatterns = patternContainer.size();
  
  for(s32 patternNum = 0; patternNum < numPatterns; ++patternNum){
    LightPattern pattern;
    const auto& lightPattern = patternContainer[patternNum];
    
    if(!lightPattern.isMember(kPatternKey) ||
       !lightPattern.isMember(kDurationKey))
    {
      PRINT_NAMED_ERROR("CubeLightAnimationContainer.ParseJsonToAnim.InvalidJson",
                        "Missing member field in %s json file",
                        animationName.c_str());
      return RESULT_FAIL;
    }
    
    const bool res = ParseJsonToPattern(lightPattern[kPatternKey], pattern);
    if(!res)
    {
      PRINT_NAMED_ERROR("CubeLightAnimationContainer.ParseJsonToAnim.InvalidPattern",
                        "Missing member field in one of the pattern definitions in %s json file",
                        animationName.c_str());
      return RESULT_FAIL;
    }
    
    pattern.duration_ms = lightPattern[kDurationKey].asUInt();
    
    if(lightPattern.isMember(kCanBeOverriddenKey))
    {
      pattern.canBeOverridden = lightPattern[kCanBeOverriddenKey].asBool();
    }
    
    // Pattern name debug if defined, otherwise animation name and pattern idx
    if(lightPattern.isMember(kPatternDebugNameKey)){
      pattern.name = lightPattern[kPatternDebugNameKey].asString();
    }else{
      pattern.name = animationName + std::to_string(patternNum);;
    }
    
    anim.push_back(std::move(pattern));
  }
  _animations[animationName] = anim;
  return RESULT_OK;
}
  
} // namespace Cozmo
} // namespace Anki
