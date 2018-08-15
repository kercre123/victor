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

#include "coretech/vision/shared/spritePathMap.h"
#include "coretech/vision/shared/spriteSequence/spriteSequence.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"
#include "util/helpers/noncopyable.h"
#include <mutex>

namespace Anki {

// Forward declaration
namespace Util {
namespace Data {
class DataPlatform;
}
}

namespace Vision {
class SpriteCache;
class SpriteSequenceContainer;
}

namespace Vector {


class SpriteSequenceLoader : private Util::noncopyable
{
public:
  SpriteSequenceLoader(){}

  Vision::SpriteSequenceContainer* LoadSpriteSequences(const Util::Data::DataPlatform* dataPlatform, 
                                                       Vision::SpritePathMap* spriteMap,
                                                       Vision::SpriteCache* cache,
                                                       const std::vector<std::string>& spriteSequenceDirs);
private:
  std::mutex _mapMutex;

  Vision::SpriteSequenceContainer::MappedSequenceContainer _mappedSequences;
  // If new SpriteSequneces are introduced into EXTERNALS but the SpriteMap and CLAD enum
  // haven't been updated yet, the sequences will be stored in this string map
  // NO SEQEUNCES SHOULD BE IN THIS ARRAY FOR MORE THAN A DAY OR TWO AND DEFINITELY NOT IN SHIPPING CODE
  Vision::SpriteSequenceContainer::UnmappedSequenceContainer _unmappedSequences;

  void LoadSequence(Vision::SpriteCache* cache,
                    const std::string& fullDirectoryPath, 
                    Vision::SpriteName sequenceName);
  
  // Legacy implementation of loading sprite sequences 
  // uses png names to determine image order and plays images straight through with a hold on the final frame
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

} // namespace Vector
} // namespace Anki

#endif // __CannedAnimLib_SpriteSequences_SpriteSequenceLoader_H__
