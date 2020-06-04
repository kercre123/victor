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
#include "coretech/common/shared/types.h"

#include <string>
#include <deque>

namespace Anki {

namespace Vision{
class Image;
class ImageRGB565;
  
class SpriteSequence
{
public:
  using ImgTypeCacheSpec = ISpriteWrapper::ImgTypeCacheSpec;

  // TODO(str): VIC-13523 Remove LoopConfig from SpriteSequence
  // This parameter was meant to be populated from a definition.json file included in
  // the folder for the spriteSequence. Since those files do not currenlty make it to 
  // the robot due to a build system oversight, this parameter always defaults to "Hold".
  // As such, this functionality can be removed and handed off to the SpriteBoxKeyFrame
  // infrastructure which allows for more dynamic usage of SpriteSequence assets from
  // animation to animation instead of hardcoding it into the SpriteSequence itself.
  enum class LoopConfig{
    Loop,
    Hold,
    DoNothing,
    Error
  };

  static LoopConfig LoopConfigFromString(const std::string& config);

  SpriteSequence(LoopConfig config = LoopConfig::Hold);
  virtual ~SpriteSequence();
  
  uint16_t GetNumFrames() const { return static_cast<uint16_t>(_frames.size()); }
  
  void AddFrame(Vision::SpriteHandle spriteHandle);
  
  // Get a frame with the given index
  bool GetFrame(const u32 index, Vision::SpriteHandle& handle) const;
  
  // Pop the front frame
  void PopFront();

  void CacheSequence(SpriteCache* cache, 
                     ImgTypeCacheSpec cacheSpec,
                     BaseStationTime_t cacheFor_ms,
                     const HSImageHandle& hueAndSaturation = {}) const;
  
  // Clear the underlying container
  void Clear() { _frames.clear();};

  void SetLoopConfig(LoopConfig config){ _loopConfig = config;}

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
