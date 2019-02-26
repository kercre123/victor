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

#include "coretech/vision/shared/spriteSequence/spriteSequence.h"
#include "util/helpers/noncopyable.h"

namespace Anki {

// forward decleration
namespace Vision{
class SpriteSequence;

class SpriteSequenceContainer : private Util::noncopyable
{
public:
  using SpriteSequenceMap = std::unordered_map<std::string, const Vision::SpriteSequence>;
  SpriteSequenceContainer(SpriteSequenceMap&& spriteSequences);
  
  bool IsValidSpriteSequenceName(const std::string& spriteSeqName) const;
  const Vision::SpriteSequence* const GetSpriteSequence(const std::string& spriteSeqName) const;

protected:  
  SpriteSequenceMap _spriteSequenceMap;

}; // class SpriteSequenceContainer

} // namespace Vision
} // namespace Anki


#endif // __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__

