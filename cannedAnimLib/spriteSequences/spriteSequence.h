/**
 * File: spriteSequence.h
 *
 * Author: Matt Michini
 * Date:   2/6/2018
 *
 * Description: Defines container for sprite sequences that display on the robot's face.
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
  SpriteSequence(bool isGrayscale);
  virtual ~SpriteSequence();
  
  bool IsGrayscale() const { return _isGrayscale; }
  size_t GetNumFrames() const { return _isGrayscale ? _framesGray.size() : _framesRGB565.size(); }
  
  // Add frames of the given type
  template<class ImageType>
  void AddFrame(const ImageType& img);
  
  // Get a frame with the given index
  void GetFrame(const u32 index, Vision::Image& img) const;
  void GetFrame(const u32 index, Vision::ImageRGB565& img) const;
  
  // Pop the front frame
  void PopFront();
  
  // Clear the underlying container
  void Clear() { _framesGray.clear(); _framesRGB565.clear(); };

private:
  bool _isGrayscale = true;
  std::deque<Vision::Image> _framesGray;         // underlying image container if isGrayscale == true
  std::deque<Vision::ImageRGB565> _framesRGB565; // underlying image container if images are color
};
  
} // namespace Cozmo
} // namespace Anki


#endif // __CannedAnimLib_SpriteSequences_SpriteSequence_H__

