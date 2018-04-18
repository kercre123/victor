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
#include "clad/types/spriteNames.h"
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include "util/helpers/noncopyable.h"

namespace Anki {

// forward decleration
namespace Vision{
class SpriteSequence;

class SpriteSequenceContainer : private Util::noncopyable
{
public:
  using MappedSequenceContainer = std::unordered_map<Vision::SpriteName, const Vision::SpriteSequence, Util::EnumHasher>;
  using UnmappedSequenceContainer = std::unordered_map<std::string, const Vision::SpriteSequence>;
  SpriteSequenceContainer(MappedSequenceContainer&& mappedSequences,
                          UnmappedSequenceContainer&& unmappedSequences);
  
  const Vision::SpriteSequence* const GetSequenceByName(Vision::SpriteName sequenceName) const;
  // stored as file name without extension, not absolute/full file path
  const Vision::SpriteSequence* const GetUnmappedSequenceByFileName(const std::string& fileName) const;
  
protected:  
  MappedSequenceContainer   _mappedSequences;
  UnmappedSequenceContainer _unmappedSequences;

}; // class SpriteSequenceContainer

} // namespace Vision
} // namespace Anki


#endif // __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__

