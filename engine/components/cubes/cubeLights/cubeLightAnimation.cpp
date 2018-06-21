/**
 * File: cubeLightAnimation.cpp
 *
 * Author: Kevin M. Karol
 * Created: 6/19/18
 *
 * Description: Defines the structs/classes related to cube lights in engine
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/components/cubes/cubeLights/cubeLightAnimation.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace CubeLightAnimation {

namespace{
// Pattern Keys
static const char* kOnColorsKey            = "onColors";
static const char* kOffColorsKey           = "offColors";
static const char* kOnPeriodKey            = "onPeriod_ms";
static const char* kOffPeriodKey           = "offPeriod_ms";
static const char* kTransitionOnPeriodKey  = "transitionOnPeriod_ms";
static const char* kTransitionOffPeriodKey = "transitionOffPeriod_ms";
static const char* kOffsetKey              = "offset";
static const char* kRotateKey              = "rotate";

// Sequence Keys
static const char* kDurationKey            = "duration_ms";
static const char* kPatternKey             = "pattern";
static const char* kCanBeOverriddenKey     = "canBeOverridden";
static const char* kPatternDebugNameKey    = "patternDebugName";
}

bool ParseCubeAnimationFromJson(const Json::Value& jsonRoot, CubeLightAnimation::Animation& outAnimation)
{
  const Json::Value::Members animationNames = jsonRoot.getMemberNames();
  
  if(animationNames.empty()){
    PRINT_NAMED_ERROR("CubeLightAnimationContainer.DefineFromJson.EmptyFile",
                      "Found no animations in JSON");
    return false;
  }else if(animationNames.size() != 1){
    PRINT_NAMED_ERROR("CubeLightAnimationContainer.DefineFromJson.TooManyAnims",
                      "Expecting only one animation per json file, found %lu. "
                      "Will use first: %s",
                      (unsigned long)animationNames.size(), animationNames[0].c_str());
  }
  
  const std::string& animationName = animationNames[0];
  const Json::Value& patternContainer = jsonRoot[animationName];
  
  

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
      return false;
    }
    
    const bool res = ParseJsonToPattern(lightPattern[kPatternKey], pattern);
    if(!res)
    {
      PRINT_NAMED_ERROR("CubeLightAnimationContainer.ParseJsonToAnim.InvalidPattern",
                        "Missing member field in one of the pattern definitions in %s json file",
                        animationName.c_str());
      return false;
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
    
    outAnimation.push_back(std::move(pattern));
  }

  return true;
}


bool ParseJsonToPattern(const Json::Value& json, LightPattern& pattern)
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
  res &= json.isMember(kRotateKey);
  
  // If any of the member fields are missing from the pattern definition return false
  if(!res)
  {
    return false;
  }
  
  values.rotate            = json[kRotateKey].asBool();
  values.makeRelative      = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
  values.relativePoint     = {0,0};
  
  pattern.lights = std::move(values);
  
  return true;
}

void LightPattern::Print() const
{
  for(int i = 0; i < static_cast<int>(CubeConstants::NUM_CUBE_LEDS); i++)
  {
    PRINT_CH_DEBUG("CubeLightComponent", "LightPattern.Print",
                   "%s LED %u, onColor %u, offColor %u, onFrames %u, "
                   "offFrames %u, transOnFrames %u, transOffFrames %u, offset %u",
                   name.c_str(),
                   i, lights.onColors[i],
                   lights.offColors[i],
                   lights.onPeriod_ms[i],
                   lights.offPeriod_ms[i],
                   lights.transitionOnPeriod_ms[i],
                   lights.transitionOffPeriod_ms[i],
                   lights.offset[i]);
  }
}

} // namespace CubeLightAnimation
} // namespace Cozmo
} // namespace Anki
