/**
 * File: backpackLightAnimationContainer.h
 *
 * Authors: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Container for json-defined backpack light animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/common/basestation/jsonTools.h"
#include <unordered_map>

#ifndef __Anki_Cozmo_Backpack_Light_Animation_Container_H__
#define __Anki_Cozmo_Backpack_Light_Animation_Container_H__

namespace Anki {
namespace Cozmo {

struct BackpackLights;

// This is here so we can easily update what the container is holding in preparation for
// COZMO-9124
using BackpackAnimation = BackpackLights;

class BackpackLightAnimationContainer
{
public:
  BackpackAnimation* GetAnimation(const std::string& name);
  const BackpackAnimation* GetAnimation(const std::string& name) const;
  
  Result DefineFromJson(const Json::Value& jsonRoot);
  
private:

  BackpackAnimation* GetAnimationHelper(const std::string& name) const;
  
  void AddBackpackLightStateValues(const std::string& state, const Json::Value& data);
  
  std::unordered_map<std::string, BackpackAnimation> _animations;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Backpack_Light_Animation_Container_H__

