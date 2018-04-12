/**
 * File: spriteSequence.h
 *
 * Author: Matt Michini
 * Date:   2/6/2018
 *
 * Description: Defines container for image animations that display on the robot's face.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __CannedAnimLib_SpriteSequences_SpriteSequence_H__
#define __CannedAnimLib_SpriteSequences_SpriteSequence_H__

#include <string>
#include <deque>

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Cozmo {
  
class SpriteSequence
{
public:
  
  bool IsGrayscale() const { return _isGrayscale; };
  void SetGrayscale(const bool b) { _isGrayscale = b; }
  
  time_t GetLastLoadedTime() const { return _lastLoadedTime; }
  void SetLastLoadedTime(const time_t t) { _lastLoadedTime = t; }

  bool ShouldHold() const { return _hold; }
  
  size_t GetNumFrames() const { return _isGrayscale ? _framesGray.size() : _framesRGB565.size(); }
  
  // Add frames of the given type
  template<class ImageType>
  void AddFrame(const ImageType& img, bool hold = false);
  
  // Get a frame with the given index
  void GetFrame(const u32 index, Vision::Image& img);
  void GetFrame(const u32 index, Vision::ImageRGB565& img);
  
  // Pop the front frame
  void PopFront();
  
  // Clear the underlying container
  void Clear() { _framesGray.clear(); _framesRGB565.clear(); };

private:
  bool _isGrayscale;
  time_t _lastLoadedTime;
  bool _hold = false;
  std::deque<Vision::Image> _framesGray;         // underlying image container if isGrayscale == true
  std::deque<Vision::ImageRGB565> _framesRGB565; // underlying image container if images are color
};
  
} // namespace Cozmo
} // namespace Anki


#endif // __CannedAnimLib_SpriteSequences_SpriteSequence_H__

