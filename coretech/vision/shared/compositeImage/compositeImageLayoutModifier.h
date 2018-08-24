/**
* File: compositeImageLayoutModifier.h
*
* Author: Kevin M. Karol
* Created: 8/20/2018
*
* Description: Provides the ability to define x/y coordinate changes on a per frame basis for a sprite box
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Vision_CompositeImageLayoutModifier_fwd_H__
#define __Vision_CompositeImageLayoutModifier_fwd_H__

#include "coretech/common/engine/math/point.h"
#include "coretech/vision/shared/spriteSequence/spriteSequence.h"


// forward declaration
namespace Json {
class Value;
}

namespace Anki {
namespace Vision {

class SpriteSequenceContainer;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CompositeImageLayoutModifier{
public:
  using LoopConfig = SpriteSequence::LoopConfig;

  CompositeImageLayoutModifier(LoopConfig loopConfig = LoopConfig::Hold);
  virtual ~CompositeImageLayoutModifier();

  // Call will fail if an attempt is made to access a frame that is out of bounds
  bool GetPositionForFrame(const u32 frameIdx, Point2i& outTopLeftCorner);

  // position will be updated if
  //   a) frame already exists with a different point
  //   b) the index specified is the next element to be added to the vector
  //   c) fillMissingFramesWithValue is true and the frameIdx > the current sequence length, in which case n values
  //        will be added to the sequence where n = frameIdx - sequenceLength
  // otherwise update may fail and false will be returned
  bool UpdatePositionForFrame(const u32 frameIdx, const Point2i& topLeftCorner, bool fillMissingFramesWithValue = false);

  const std::vector<Point2i>& GetModifierSequence(){ return _coordinatesSequence; }

private:
  LoopConfig loopConfig;
  std::vector<Point2i> _coordinatesSequence;
};




}; // namespace Vision
}; // namespace Anki

#endif // __Vision_CompositeImageLayoutModifier_fwd_H__
