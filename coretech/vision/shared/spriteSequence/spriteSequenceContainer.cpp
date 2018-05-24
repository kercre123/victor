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
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/data/dataScope.h"
#include "coretech/common/engine/array2d_impl.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"
#include <algorithm>

#include <opencv2/highgui.hpp>

#include <errno.h>
#include <dirent.h>

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteSequenceContainer::SpriteSequenceContainer(MappedSequenceContainer&& mappedSequences,
                                                 UnmappedSequenceContainer&& unmappedSequences)
{
  _mappedSequences = std::move(mappedSequences);
  _unmappedSequences = std::move(unmappedSequences);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::SpriteSequence* const SpriteSequenceContainer::GetSequenceByName(Vision::SpriteName sequenceName) const
{
  auto seqIter = _mappedSequences.find(sequenceName);
  if(seqIter == _mappedSequences.end()) {
    PRINT_NAMED_WARNING("SpriteSequenceContainer.GetSequenceByName.UnknownName",
                        "Unknown sequence requested: %s", Vision::SpriteNameToString(sequenceName));
    return nullptr;
  } else {
    return &seqIter->second;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::SpriteSequence* const SpriteSequenceContainer::GetUnmappedSequenceByFileName(const std::string& fileName) const
{
  auto seqIter = _unmappedSequences.find(fileName);
  if(seqIter == _unmappedSequences.end()) {
    PRINT_NAMED_WARNING("SpriteSequenceContainer.GetSequenceByName.UnknownName",
                        "Unknown sequence requested: %s", fileName.c_str());
    return nullptr;
  } else {
    return &seqIter->second;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::SpriteSequence* const SpriteSequenceContainer::GetSequenceAgnostic(Vision::SpriteName sequenceName,
                                                                                 const std::string& fileName) const
{
  const Vision::SpriteSequence* spriteSeq = GetSequenceByName(sequenceName);
  if(spriteSeq == nullptr){
    spriteSeq = GetUnmappedSequenceByFileName(fileName);
  }
  return spriteSeq;
}


} // namespace Cozmo
} // namespace Anki
