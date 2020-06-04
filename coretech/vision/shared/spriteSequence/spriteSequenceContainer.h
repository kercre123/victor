/**
 * File: spriteSequenceContainer.h
 *
 * Author: Andrew Stein
 * Date:   7/7/2015
 *
 * Description: Defines container for managing available sequences for the robot's face display.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__
#define __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__

#include <string>
#include <unordered_map>

#include "coretech/vision/shared/spritePathMap.h"
#include "coretech/vision/shared/spriteSequence/spriteSequence.h"
#include "util/helpers/noncopyable.h"

namespace Anki {

// forward decleration
namespace Vision{

class SpriteSequenceContainer : private Util::noncopyable
{
public:
  using SpriteSequenceMap = std::unordered_map<Vision::SpritePathMap::AssetID, const Vision::SpriteSequence>;
  SpriteSequenceContainer(SpriteSequenceMap&& spriteSequences);
  
  bool IsValidSpriteSequenceID(const Vision::SpritePathMap::AssetID spriteSeqID) const;
  const Vision::SpriteSequence* const GetSpriteSequence(const Vision::SpritePathMap::AssetID spriteSeqID) const;

protected:  
  SpriteSequenceMap _spriteSequenceMap;

}; // class SpriteSequenceContainer

} // namespace Vision
} // namespace Anki


#endif // __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__

