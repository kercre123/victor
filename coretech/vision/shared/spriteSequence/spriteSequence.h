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

#ifndef __Vision_Shared_SpriteSequence_H__
#define __Vision_Shared_SpriteSequence_H__

#include "coretech/vision/shared/spriteCache/spriteCache.h"

#include <string>
#include <deque>

namespace Anki {

namespace Vision{
class Image;
class ImageRGB565;
  
class SpriteSequence
{
public:
  // Define what should be returned if there are attempts to access frames beyond
  // the final frame index
  enum class LoopConfig{
    Loop,
    Hold,
    Error
  };

  SpriteSequence(LoopConfig config = LoopConfig::Hold);
  virtual ~SpriteSequence();
  
  uint GetNumFrames() const { return static_cast<uint>(_frames.size()); }
  
  void AddFrame(Vision::SpriteHandle spriteHandle);
  
  // Get a frame with the given index
  bool GetFrame(const u32 index, Vision::SpriteHandle& handle) const;
  
  // Pop the front frame
  void PopFront();
  
  // Clear the underlying container
  void Clear() { _frames.clear();};

private:
  // Returns true if the moddedIndex was set properly
  // returns false if the index is invalid 
  bool GetModdedIndex(const u32 index, u32& moddedIndex) const;

  LoopConfig _loopConfig;
  std::deque<Vision::SpriteHandle> _frames;
};
  
} // namespace Vision
} // namespace Anki


#endif // __Vision_Shared_SpriteSequence_H__
