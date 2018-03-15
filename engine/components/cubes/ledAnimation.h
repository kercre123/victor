/**
 * File: ledAnimation
 *
 * Author: Matt Michini
 * Created: 03/12/2018
 *
 * Description: Generates individual keyframe sequences from CubeLight 'animations'
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_CubeLedKeyframeGenerator_H__
#define __Engine_Components_CubeLedKeyframeGenerator_H__

#include <vector>

namespace Anki {
namespace Cozmo {
  
// forware decl:
class CubeMessage;
struct CubeLights;
struct LightState;
struct CubeLightKeyframe;
  
class LedAnimation
{
public:
  
  // Construct an LED animation from a LightState struct.
  // This creates a set of keyframes defining an LED animation. By default, the
  // animation links to itself (i.e. it will repeat indefinitely). Link indices
  // are all offset by baseIndex.
  LedAnimation(const LightState& lightState, const int baseIndex);
  
  ~LedAnimation() = default;
  
  // Causes the animation to play one time then end (rather than repeating)
  void SetPlayOnce();

  // Links the end of this animation to the beginning of 'other'
  void LinkToOther(const LedAnimation& other);
  
  // Get the index of the starting keyframe of the animation (always the first element)
  int GetStartingIndex() const { return _baseIndex; }

  // Get the index of the final keyframe of the animation (always the last element)
  int GetFinalIndex() const { return _baseIndex + static_cast<int>(_keyframes.size() - 1); }
  
  // Get a reference to the underlying keyframes
  const std::vector<CubeLightKeyframe>& GetKeyframes() const { return _keyframes; }
  
private:
  
  std::vector<CubeLightKeyframe> _keyframes;
  
  // This animation's base index
  int _baseIndex = 0;
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_CubeLedKeyframeGenerator_H__
