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

#include "cannedAnimLib/baseTypes/spritePathMap.h"
#include "cannedAnimLib/spriteSequences/spriteSequence.h"
#include "clad/types/spriteNames.h"
#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/vision/engine/image.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class SpriteSequence;

class SpriteSequenceContainer : private Util::noncopyable
{
public:
  using MappedSequenceContainer = std::unordered_map<Vision::SpriteName, const SpriteSequence>;
  using UnmappedSequenceContainer = std::unordered_map<std::string, const SpriteSequence>;
  SpriteSequenceContainer(MappedSequenceContainer&& mappedSequences,
                          UnmappedSequenceContainer&& unmappedSequences);
  
  const SpriteSequence* const GetSequenceByName(Vision::SpriteName sequenceName) const;
  // stored as file name without extension, not absolute/full file path
  const SpriteSequence* const GetUnmappedSequenceByFileName(const std::string& fileName) const;

  // Get the total number of available sequences
  size_t GetSize() const;
  
protected:  
  MappedSequenceContainer   _mappedSequences;
  UnmappedSequenceContainer _unmappedSequences;

}; // class SpriteSequenceContainer

//
// Inlined Implementations
//

// Get the total number of available sequences
inline size_t SpriteSequenceContainer::GetSize() const {
  return _mappedSequences.size();
}

} // namespace Cozmo
} // namespace Anki


#endif // __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__

