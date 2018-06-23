/**
 * File: cubeLightAnimationContainer.h
 *
 * Authors: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Container for json-defined cube light animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Anki_Cozmo_Cube_Light_Animation_Container_H__
#define __Anki_Cozmo_Cube_Light_Animation_Container_H__

#include "engine/components/cubes/cubeLights/cubeLightAnimation.h"

#include <list>
#include <unordered_map>

namespace Anki {
namespace Cozmo {


class CubeLightAnimationContainer
{
public:
  using InitMap = std::unordered_map<std::string, const Json::Value>;
  CubeLightAnimationContainer(const InitMap& initializationMap);
  CubeLightAnimation::Animation* GetAnimation(const std::string& name);
  const CubeLightAnimation::Animation* GetAnimation(const std::string& name) const;
    
private:
  CubeLightAnimation::Animation* GetAnimationHelper(const std::string& name) const;

  std::unordered_map<std::string, CubeLightAnimation::Animation> _animations;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Cube_Light_Animation_Container_H__

