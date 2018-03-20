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

#include "engine/components/cubes/ledAnimation.h"

#include "clad/externalInterface/messageEngineToCube.h"
#include "clad/externalInterface/messageToActiveObject.h"
#include "clad/types/ledTypes.h"

#include "util/logging/logging.h"

#define RGBA_TO_RGB_ARRAY(rgba) {{static_cast<uint8_t>((rgba & 0xFF000000) >> 24),  \
                                  static_cast<uint8_t>((rgba & 0x00FF0000) >> 16),  \
                                  static_cast<uint8_t>((rgba & 0x0000FF00) >> 8)}}

namespace Anki {
namespace Cozmo {
  
LedAnimation::LedAnimation(const LightState& lightState, const int baseIndex)
{
  const bool hasOffset = lightState.offset != 0;

  // Re-used temporary keyframe:
  CubeLightKeyframe keyframe;
  
  size_t currKeyFrameIndex = 0;
  
  if (hasOffset) {
    keyframe.color.fill(0);
    keyframe.holdFrames = lightState.offset;
    keyframe.decayFrames = 0;
    _keyframes.push_back(keyframe);
    ++currKeyFrameIndex;
  }
  
  keyframe.color = RGBA_TO_RGB_ARRAY(lightState.offColor);
  keyframe.holdFrames = 0;
  keyframe.decayFrames = lightState.transitionOnFrames;
  keyframe.nextIndex = currKeyFrameIndex + 1;
  _keyframes.push_back(keyframe);
  ++currKeyFrameIndex;
  
  keyframe.color = RGBA_TO_RGB_ARRAY(lightState.onColor);
  keyframe.holdFrames = lightState.onFrames;
  keyframe.decayFrames = lightState.transitionOffFrames;
  keyframe.nextIndex = currKeyFrameIndex + 1;
  _keyframes.push_back(keyframe);
  ++currKeyFrameIndex;
  
  keyframe.color = RGBA_TO_RGB_ARRAY(lightState.offColor);
  keyframe.holdFrames = lightState.offFrames;
  keyframe.decayFrames = 0;
  keyframe.nextIndex = hasOffset ? 1 : 0; // If there is an offset frame in the front, don't go back to it
  _keyframes.push_back(keyframe);
  
  // Handle the special case of all lightState times being zero,
  // indicating that we should keep OnColor indefinitely
  if (lightState.onFrames == 0 &&
      lightState.offFrames == 0 &&
      lightState.transitionOnFrames == 0 &&
      lightState.transitionOffFrames == 0) {
    DEV_ASSERT(_keyframes.empty(), "LedAnimation.LedAnimation.ShouldBeNoKeyframes");
    keyframe.color = RGBA_TO_RGB_ARRAY(lightState.onColor);
    keyframe.holdFrames = 0;
    keyframe.decayFrames = 0;
    _keyframes.push_back(keyframe);
  }
      
  DEV_ASSERT(!_keyframes.empty(), "LedAnimation.LedAnimation.NoKeyframes");
  
  // Set up linking among the keyframes. Link each keyframe
  // (except the final one) to the keyframe after it.
  // Need to add baseIndex to each index.
  for (int i=0 ; i < _keyframes.size()-1 ; i++) {
    _keyframes[i].nextIndex = i + 1 + baseIndex;
  }
  
  _baseIndex = baseIndex;
}

  
void LedAnimation::SetPlayOnce()
{
  // Setting both hold and decay to zero will cause
  // the last frame to remain offColor indefinitely
  _keyframes.back().holdFrames  = 0;
  _keyframes.back().decayFrames = 0;
}
  
  
void LedAnimation::LinkToOther(const LedAnimation& other)
{
  _keyframes.back().nextIndex = other.GetStartingIndex();
}


} // Cozmo namespace
} // Anki namespace
