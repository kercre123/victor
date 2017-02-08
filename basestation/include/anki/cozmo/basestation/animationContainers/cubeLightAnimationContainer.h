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

#include "anki/common/basestation/jsonTools.h"
#include <list>
#include <unordered_map>

#ifndef __Anki_Cozmo_Cube_Light_Animation_Container_H__
#define __Anki_Cozmo_Cube_Light_Animation_Container_H__

namespace Anki {
namespace Cozmo {

struct LightPattern;

using CubeAnimation = std::list<LightPattern>;

class CubeLightAnimationContainer
{
public:
  CubeAnimation* GetAnimation(const std::string& name);
  const CubeAnimation* GetAnimation(const std::string& name) const;
  
  Result DefineFromJson(const Json::Value& jsonRoot);
  
private:

  CubeAnimation* GetAnimationHelper(const std::string& name) const;

  bool ParseJsonToPattern(const Json::Value& json, LightPattern& pattern);
  
  std::unordered_map<std::string, CubeAnimation> _animations;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Cube_Light_Animation_Container_H__

