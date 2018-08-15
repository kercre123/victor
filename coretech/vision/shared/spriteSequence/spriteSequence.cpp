/**
 * File: spriteSequence.cpp
 *
 * Author: Matt Michini
 * Date:   2/6/2018
 *
 * Description: Defines container for sprite sequences that display on the robot's face.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/vision/shared/spriteSequence/spriteSequence.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Vision {

SpriteSequence::LoopConfig SpriteSequence::LoopConfigFromString(const std::string& config)
{
  if(config == "loop"){
    return LoopConfig::Loop;
  }else if(config == "hold"){
    return LoopConfig::Hold;
  }else if(config == "error"){
    return LoopConfig::Error;
  }else if(config == "doNothing"){
    return LoopConfig::DoNothing;
  }else{
    PRINT_NAMED_ERROR("SpriteSequence.LoopConfigFromString.ImproperString",
                      "No config for %s",
                      config.c_str());
    return LoopConfig::Error;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteSequence::SpriteSequence(LoopConfig config)
: _loopConfig(config)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteSequence::~SpriteSequence()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteSequence::AddFrame(Vision::SpriteHandle spriteHandle)
{
  _frames.push_back(spriteHandle);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteSequence::PopFront()
{
  if(!_frames.empty()){
    _frames.pop_front();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteSequence::CacheSequence(SpriteCache* cache, 
                                   ImgTypeCacheSpec cacheSpec,
                                   BaseStationTime_t cacheFor_ms,
                                   const HSImageHandle& hueAndSaturation) const
{
  for(auto& frame: _frames){
    cache->CacheSprite(frame, cacheSpec, cacheFor_ms, hueAndSaturation);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteSequence::GetFrame(const u32 index, Vision::SpriteHandle& handle) const
{
  if(_frames.empty()){
    return false;
  }

  if((_loopConfig == LoopConfig::DoNothing) &&
     (index >= _frames.size())){
    return false;
  }
  
  u32 modIndex = 0;
  const bool success = GetModdedIndex(index, modIndex);
  if(ANKI_VERIFY(success, 
                 "SpriteSequence.GetFrame.ModIndexFail",
                 "Index %d is invalid for greyscale sprite sequence of length %zu and cannot be looped",
                 index, _frames.size())){
    handle = _frames[modIndex];
  }

  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteSequence::GetModdedIndex(const u32 index, u32& moddedIndex) const
{
  switch(_loopConfig){
    case LoopConfig::Loop:
      moddedIndex = index % _frames.size();
      break;
    case LoopConfig::Hold:
      moddedIndex = (index < _frames.size()) ? index : static_cast<u32>(_frames.size() - 1);
      break;
    case LoopConfig::DoNothing:
      if(index >= _frames.size()){
        return false;
      }
      moddedIndex = index;
    case LoopConfig::Error:
      if(index >= _frames.size()){
        PRINT_NAMED_ERROR("SpriteSequence.GetFrame.FrameBeyondIndex",
                          "Attempted to access frame %d",
                          index);
        return false;
      }
  }
  return true;
}


} // namespace Vector
} // namespace Anki
