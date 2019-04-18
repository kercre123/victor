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
bool SpriteSequenceContainer::IsValidSpriteSequenceID(const Vision::SpritePathMap::AssetID spriteSeqID) const
{
  const auto& seqIter = _spriteSequenceMap.find(spriteSeqID);
  return seqIter != _spriteSequenceMap.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::SpriteSequence* const SpriteSequenceContainer::GetSpriteSequence(
  const Vision::SpritePathMap::AssetID spriteSeqID) const
{
  const auto& seqIter = _spriteSequenceMap.find(spriteSeqID);
  if(seqIter == _spriteSequenceMap.end()) {
    PRINT_NAMED_ERROR("SpriteSequenceContainer.UnknownSpriteSequenceID",
                      "Unknown sequence id requested: %d", spriteSeqID);
    return nullptr;
  } else {
    return &seqIter->second;
  }
}

} // namespace Vector
} // namespace Anki
