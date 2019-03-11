/**
 * File: spriteSequenceContainer.cpp
 *
 * Author: Andrew Stein
 * Date:   7/7/2015
 *
 * Description: Implements container for managing available sequences for the robot's face display.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteSequenceContainer::SpriteSequenceContainer(SpriteSequenceMap&& spriteSequenceMap)
{
  _spriteSequenceMap = std::move(spriteSequenceMap);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteSequenceContainer::IsValidSpriteSequenceName(const std::string& spriteSeqName) const
{
  const auto& seqIter = _spriteSequenceMap.find(spriteSeqName);
  return seqIter != _spriteSequenceMap.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::SpriteSequence* const SpriteSequenceContainer::GetSpriteSequence(const std::string& spriteSeqName) const
{
  const auto& seqIter = _spriteSequenceMap.find(spriteSeqName);
  if(seqIter == _spriteSequenceMap.end()) {
    PRINT_NAMED_ERROR("SpriteSequenceContainer.UnknownSpriteSequenceName",
                      "Unknown sequence requested: %s", spriteSeqName.c_str());
    return nullptr;
  } else {
    return &seqIter->second;
  }
}

} // namespace Vector
} // namespace Anki
