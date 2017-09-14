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

#include "anki/common/basestation/colorRGBA.h"
#include "engine/animations/animationContainers/backpackLightAnimationContainer.h"
#include "engine/components/bodyLightComponent.h"

namespace Anki {
namespace Cozmo {

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

void BackpackLightAnimationContainer::AddBackpackLightStateValues(const std::string& state,
                                                                  const Json::Value& data)
{
  BackpackLights values;
  
  // Test to see if we're dealing with V1 or V2 lights.
  // There are 4 lights in V2 and 5 lights in V1.
  bool isV1 = false;
  const char* testKey = "onColors";
  if(data.isMember(testKey))
  {
    const Json::Value& jsonValues = data[testKey];
    if (5 == jsonValues.size()) {
      isV1 = true;
    }
  }
  
  bool res = true;
  if (!isV1) {
    res &= JsonTools::GetColorValuesToArrayOptional(data, "onColors",  values.onColors, true);
    res &= JsonTools::GetColorValuesToArrayOptional(data, "offColors", values.offColors, true);
    res &= JsonTools::GetArrayOptional(data, "onPeriod_ms",            values.onPeriod_ms);
    res &= JsonTools::GetArrayOptional(data, "offPeriod_ms",           values.offPeriod_ms);
    res &= JsonTools::GetArrayOptional(data, "transitionOnPeriod_ms",  values.transitionOnPeriod_ms);
    res &= JsonTools::GetArrayOptional(data, "transitionOffPeriod_ms", values.transitionOffPeriod_ms);
    res &= JsonTools::GetArrayOptional(data, "offset",                 values.offset);
  } else {
    BackpackLights_V1 v1Values;
    res &= JsonTools::GetColorValuesToArrayOptional(data, "onColors",  v1Values.onColors, true);
    res &= JsonTools::GetColorValuesToArrayOptional(data, "offColors", v1Values.offColors, true);
    res &= JsonTools::GetArrayOptional(data, "onPeriod_ms",            v1Values.onPeriod_ms);
    res &= JsonTools::GetArrayOptional(data, "offPeriod_ms",           v1Values.offPeriod_ms);
    res &= JsonTools::GetArrayOptional(data, "transitionOnPeriod_ms",  v1Values.transitionOnPeriod_ms);
    res &= JsonTools::GetArrayOptional(data, "transitionOffPeriod_ms", v1Values.transitionOffPeriod_ms);
    res &= JsonTools::GetArrayOptional(data, "offset",                 v1Values.offset);
    
    if (res) {
      PRINT_NAMED_INFO("BackpackLightAnimationContainer.AddBackpackLightStateValues.ConvertingV1ToV2", "");
      values = v1Values;
    }
  }
  
  // If any of the member fields are missing from the pattern definition return false
  if(!res)
  {
    PRINT_NAMED_ERROR("BackpackLightAnimationContainer.AddBackpackLightStateValues.InvalidJson",
                      "Missing member field in pattern definition for BackpackLightsState %s",
                      state.c_str());
    return;
  }
  
  _animations.emplace(state, values);
}

BackpackAnimation* BackpackLightAnimationContainer::GetAnimationHelper(const std::string& name) const
{
  BackpackAnimation* animPtr = nullptr;
  auto retVal = _animations.find(name);
  if(retVal == _animations.end()) {
    PRINT_NAMED_ERROR("BackpackLightAnimationContainer.GetAnimation_Const.InvalidName",
                      "Animation requested for unknown animation '%s'.",
                      name.c_str());
  } else {
    animPtr = const_cast<BackpackAnimation*>(&retVal->second);
  }
  
  return animPtr;
}

const BackpackAnimation* BackpackLightAnimationContainer::GetAnimation(const std::string& name) const
{
  return GetAnimationHelper(name);
}

BackpackAnimation* BackpackLightAnimationContainer::GetAnimation(const std::string& name)
{
  return GetAnimationHelper(name);
}

Result BackpackLightAnimationContainer::DefineFromJson(const Json::Value& jsonRoot)
{
  // These strings directly correspond to the BackpackLightsStates in bodyLightComponent.h
  _animations.emplace("OffCharger", BodyLightComponent::GetOffBackpackLights());
  AddBackpackLightStateValues("Charging",   jsonRoot["charging"]);
  AddBackpackLightStateValues("Charged",    jsonRoot["charged"]);
  AddBackpackLightStateValues("BadCharger", jsonRoot["badCharger"]);
  return RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
