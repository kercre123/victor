/**
* File: compositeImageLayoutModifier.cpp
*
* Author: Kevin M. Karol
* Created: 8/20/2018
*
* Description: Provides the ability to define x/y coordinate changes on a per frame basis for a sprite box
*
* Copyright: Anki, Inc. 2018
*
**/


#include "coretech/vision/shared/compositeImage/compositeImageLayoutModifier.h"

#include "coretech/vision/engine/image_impl.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayoutModifier::CompositeImageLayoutModifier(LoopConfig loopConfig)
: loopConfig(loopConfig)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompositeImageLayoutModifier::~CompositeImageLayoutModifier()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayoutModifier::GetPositionForFrame(const u32 frameIdx, Point2i& outTopLeftCorner)
{
  u32 getIdx = frameIdx;

  // If frame index is out of range, adjust according to loop mode
  const auto size = _coordinatesSequence.size();
  if (getIdx >= size) {
    switch (loopConfig) {
      case LoopConfig::Loop:
        getIdx = (getIdx % size);
        break;
      case LoopConfig::Hold:
        getIdx = static_cast<u32>(size - 1);
        break;
      case LoopConfig::DoNothing:
        return false;
      case LoopConfig::Error:
        return false;
    }
  }

  DEV_ASSERT(getIdx < size, "CompositeImageLayoutModifier.GetPositionForFrame.InvalidIndex");
  outTopLeftCorner = _coordinatesSequence[getIdx];

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CompositeImageLayoutModifier::UpdatePositionForFrame(const u32 frameIdx, const Point2i& topLeftCorner, bool fillMissingFramesWithValue)
{
  if(frameIdx < _coordinatesSequence.size()){
    _coordinatesSequence[frameIdx] = topLeftCorner;
    return true;
  }else if(frameIdx == _coordinatesSequence.size()){
    _coordinatesSequence.push_back(topLeftCorner);
  }else if(fillMissingFramesWithValue){
    for(auto i = _coordinatesSequence.size(); i <= frameIdx; ++i){
      _coordinatesSequence.push_back(topLeftCorner);
    }
    return true;
  }

  return false;
}


}; // namespace Vision
}; // namespace Anki
