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

#include "cannedAnimLib/baseTypes/spritePathMap.h"
#include "cannedAnimLib/spriteSequences/spriteSequence.h"
#include "cannedAnimLib/spriteSequences/spriteSequenceContainer.h"
#include "util/helpers/noncopyable.h"

namespace Anki {

// Forward declaration
namespace Util {
namespace Data {
class DataPlatform;
}
}

namespace Cozmo {

class SpriteSequenceContainer;

class SpriteSequenceLoader : private Util::noncopyable
{
public:
  SpriteSequenceLoader(){};

  SpriteSequenceContainer* LoadSpriteSequences(const Util::Data::DataPlatform* dataPlatform, 
                                               CannedAnimLib::SpritePathMap* spriteMap,
                                               const std::vector<std::string>& spriteSequenceDirs);
private:
  SpriteSequenceContainer::MappedSequenceContainer _mappedSequences;
  // If new SpriteSequneces are introduced into EXTERNALS but the SpriteMap and CLAD enum
  // haven't been updated yet, the sequences will be stored in this string map
  // NO SEQEUNCES SHOULD BE IN THIS ARRAY FOR MORE THAN A DAY OR TWO AND DEFINITELY NOT IN SHIPPING CODE
  SpriteSequenceContainer::UnmappedSequenceContainer _unmappedSequences;

  template<class ImageType>
  void LoadImage(SpriteSequence& sequence, const std::string& fullFilename);

  void LoadSequenceImageFrames(const std::string& fullDirectoryPath, Vision::SpriteName sequenceName);


};

} // namespace Cozmo
} // namespace Anki

#endif // __CannedAnimLib_SpriteSequences_SpriteSequenceLoader_H__
