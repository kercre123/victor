/**
 * File: spriteSequenceLoader .h
 *
 * Authors: Kevin M. Karol
 * Created: 4/10/18
 *
 * Description:
 *    Class that loads sprite sequences from data on worker threads and
 *    returns the final sprite sequence container
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __CannedAnimLib_SpriteSequences_SpriteSequenceLoader_H__
#define __CannedAnimLib_SpriteSequences_SpriteSequenceLoader_H__

#include <mutex>

#include "coretech/vision/shared/spriteSequence/spriteSequence.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"
#include "util/helpers/noncopyable.h"

// Forward declarations
namespace Json {
class Value;
}

namespace Anki {

namespace Util {
namespace Data {
class DataPlatform;
}
}  // namespace Util

namespace Vision {
class SpriteCache;
class SpritePathMap;
class SpriteSequenceContainer;
}  // namespace Vision
}  // namespace Anki

namespace Anki {
namespace Vector {

class SpriteSequenceLoader : private Util::noncopyable {
 public:
  SpriteSequenceLoader() {}

  Vision::SpriteSequenceContainer* LoadSpriteSequences(
      const Util::Data::DataPlatform* dataPlatform,
      Vision::SpritePathMap* spritePathMap, Vision::SpriteCache* cache,
      const std::vector<std::string>& spriteSequenceDirs);

 private:
  std::mutex _mapMutex;

  Vision::SpriteSequenceContainer::SpriteSequenceMap _spriteSequences;

  void LoadSequence(Vision::SpriteCache* cache,
                    const std::string& fullDirectoryPath);

  // Legacy implementation of loading sprite sequences
  // uses png names to determine image order and plays images straight through
  // with a hold on the final frame
  void LoadSequenceLegacy(Vision::SpriteCache* cache,
                          const std::string& fullDirectoryPath,
                          const std::vector<std::string>& relativeImgNames,
                          Vision::SpriteSequence& outSeq) const;

  // Use the json specification to load pngs at their relative file path
  void LoadSequenceFromSpec(Vision::SpriteCache* cache,
                            const std::string& fullDirectoryPath,
                            const Json::Value& spec,
                            const std::vector<std::string>& relativeImgNames,
                            Vision::SpriteSequence& outSeq) const;
};

}  // namespace Vector
}  // namespace Anki

#endif  // __CannedAnimLib_SpriteSequences_SpriteSequenceLoader_H__
