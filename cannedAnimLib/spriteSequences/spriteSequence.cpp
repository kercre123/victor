/**
 * File: spriteSequence.cpp
 *
 * Author: Matt Michini
 * Date:   2/6/2018
 *
 * Description: Defines container for image animations that display on the robot's face.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "cannedAnimLib/spriteSequences/spriteSequence.h"
#include "coretech/common/engine/array2d_impl.h"

namespace Anki {
namespace Cozmo {

template<>
void SpriteSequence::AddFrame(const Vision::Image& img, bool hold)
{
  DEV_ASSERT(!_hold, "FaceAnimation.AddFrame.AddFrameToHoldingAnim");
  DEV_ASSERT(_isGrayscale, "AvailableAnim.AddFrame.InvalidType");
  _framesGray.push_back(img);

  if(hold)
  {
    _framesGray.push_back(Vision::Image());
  }

  _hold = hold;
}

template<>
void SpriteSequence::AddFrame(const Vision::ImageRGB565& img, bool hold)
{
  DEV_ASSERT(!_hold, "FaceAnimation.AddFrame.AddFrameToHoldingAnim");
  DEV_ASSERT(!_isGrayscale, "AvailableAnim.AddFrame.InvalidType");
  _framesRGB565.push_back(img);

  if(hold)
  {
    _framesRGB565.push_back(Vision::ImageRGB565());
  }

  _hold = hold;
}

void SpriteSequence::PopFront()
{
  if (_isGrayscale && !_framesGray.empty()) {
    _framesGray.pop_front();
  } else if (!_isGrayscale && !_framesRGB565.empty()) {
    _framesRGB565.pop_front();
  }
}

void SpriteSequence::GetFrame(const u32 index, Vision::Image& img)
{
  DEV_ASSERT(_isGrayscale, "AvailableAnim.AddFrame.InvalidType");
  DEV_ASSERT(index < _framesGray.size(), "SpriteSequence.GetFrame.InvalidIndex");
  img = _framesGray[index];
}

void SpriteSequence::GetFrame(const u32 index, Vision::ImageRGB565& img)
{
  DEV_ASSERT(!_isGrayscale, "AvailableAnim.AddFrame.InvalidType");
  DEV_ASSERT(index < _framesRGB565.size(), "SpriteSequence.GetFrame.InvalidIndex");
  img = _framesRGB565[index];
}

} // namespace Cozmo
} // namespace Anki
