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

#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"
#include "cannedAnimLib/spriteSequences/spriteSequenceLoader.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"
#include "util/dispatchWorker/dispatchWorker.h"

#include <set>
#include <sys/stat.h>


namespace Anki {
namespace Cozmo {

namespace{
static const std::set<Vision::SpriteName> kSequencesRefuseCache = {
  Vision::SpriteName::PairingIconAwaitingApp,
  Vision::SpriteName::PairingIconUpdate,
  Vision::SpriteName::PairingIconUpdateError,
  Vision::SpriteName::PairingIconWifi
};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vision::SpriteSequenceContainer* SpriteSequenceLoader::LoadSpriteSequences(const Util::Data::DataPlatform* dataPlatform,
                                                                           Vision::SpritePathMap* spriteMap,
                                                                           Vision::SpriteCache* cache,
                                                                           const std::vector<std::string>& spriteSequenceDirs,
                                                                           const std::set<Vision::SpriteCache::CacheSpec>& cacheSpecs)
{
  if (dataPlatform == nullptr) { 
    return nullptr;
  }
  Util::Data::Scope resourceScope = Util::Data::Scope::Resources;

  // Set up the worker that will process all the image frame folders
  using MyDispatchWorker = Util::DispatchWorker<3, Vision::SpriteCache*, const std::set<Vision::SpriteCache::CacheSpec>&, std::string, Vision::SpriteName>;
  MyDispatchWorker::FunctionType workerFunction = std::bind(&SpriteSequenceLoader::LoadSequenceImageFrames, 
                                                            this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
  MyDispatchWorker worker(workerFunction);
  for(const auto& path : spriteSequenceDirs){
    const std::string spriteSeqFolder = dataPlatform->pathToResource(resourceScope, path);
    
    // Get the list of all the directory names
    std::vector<std::string> spriteSequenceDirNames;
    Util::FileUtils::ListAllDirectories(spriteSeqFolder, spriteSequenceDirNames);
    
    // Go through the list of directories, removing disallowed names, updating timestamps, and removing ones that don't need to be loaded
    auto nameIter = spriteSequenceDirNames.begin();
    while (nameIter != spriteSequenceDirNames.end())
    {
      const std::string& folderName = *nameIter;
      std::string fullDirPath = Util::FileUtils::FullFilePath({spriteSeqFolder, folderName});

      Vision::SpriteName sequenceName = Vision::SpriteName::Count;
      const bool res = spriteMap->GetKeyForValue(fullDirPath, sequenceName);
      if(!res){
        PRINT_NAMED_WARNING("SpriteSequenceLoader.LoadSequences.NoSpritenameForPath",
                            "Path %s not found in spritemap - adding it to the tmp map",
                            fullDirPath.c_str());
      }
            
      // Now we can start looking at the next name, this one is ok to load
      worker.PushJob(cache, cacheSpecs, fullDirPath, sequenceName);
      ++nameIter;
    }
    
    // Go through and load the sequences from our list
    worker.Process();
  }

  return new Vision::SpriteSequenceContainer(std::move(_mappedSequences), std::move(_unmappedSequences));

} // LoadSpriteSequences()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteSequenceLoader::LoadSequenceImageFrames(Vision::SpriteCache* cache,
                                                   const std::set<Vision::SpriteCache::CacheSpec>& cacheSpecs,
                                                   const std::string& fullDirectoryPath, 
                                                   Vision::SpriteName sequenceName)
{
  // Setup the sequence that will have image frames loaded into it
  Vision::SpriteSequence sequence;


  // Even though files *might* be sorted alphabetically by the readdir call inside FilesInDirectory,
  // we can't rely on it so do it ourselves
  std::vector<std::string> fileNames = Util::FileUtils::FilesInDirectory(fullDirectoryPath);
  std::sort(fileNames.begin(), fileNames.end());
  for (auto fileIter = fileNames.begin(); fileIter != fileNames.end(); ++fileIter)
  {
    const std::string& filename = *fileIter;
    size_t underscorePos = filename.find_last_of("_");
    size_t dotPos = filename.find_last_of(".");
    if(dotPos == std::string::npos) {
      PRINT_NAMED_ERROR("SpriteSequenceContainer.LoadSequenceImageFrames",
                        "Could not find '.' in frame filename %s",
                        filename.c_str());
      return;
    } else if(underscorePos == std::string::npos) {
      PRINT_NAMED_ERROR("SpriteSequenceContainer.LoadSequenceImageFrames",
                        "Could not find '_' in frame filename %s",
                        filename.c_str());
      return;
    } else if(dotPos <= underscorePos+1) {
      PRINT_NAMED_ERROR("SpriteSequenceContainer.LoadSequenceImageFrames",
                        "Unexpected relative positions for '.' and '_' in frame filename %s",
                        filename.c_str());
      return;
    }
    
    const std::string digitStr(filename.substr(underscorePos+1,
                                                (dotPos-underscorePos-1)));
    
    s32 frameNum = 0;
    try {
      frameNum = std::stoi(digitStr);
    } catch (std::invalid_argument&) {
      PRINT_NAMED_ERROR("SpriteSequenceContainer.LoadSequenceImageFrames",
                        "Could not get frame number from substring '%s' "
                        "of filename '%s'.",
                        digitStr.c_str(), filename.c_str());
      return;
    }
    
    if(frameNum < 0) {
      PRINT_NAMED_ERROR("SpriteSequenceContainer.LoadSequenceImageFrames",
                        "Found negative frame number (%d) for filename '%s'.",
                        frameNum, filename.c_str());
      return;
    }
        
    // Add empty frames if there's a gap
    if(frameNum > 1) { 
      s32 emptyFramesAdded = 0;
      while(sequence.GetNumFrames() < frameNum-1) {
        auto handle = std::make_shared<Vision::SpriteWrapper>(new Vision::ImageRGBA());
        sequence.AddFrame(handle);
        ++emptyFramesAdded;
      }
    }

    // Load the image
    const std::string fullFilename = Util::FileUtils::FullFilePath({fullDirectoryPath, filename});
    
    Vision::SpriteHandle handle;
    if(kSequencesRefuseCache.find(sequenceName) !=  kSequencesRefuseCache.end()){
      handle = cache->GetSpriteHandle(fullFilename);
    }else{
      handle = cache->GetSpriteHandle(fullFilename, cacheSpecs);
    }
    sequence.AddFrame(handle);
  }

  // Place the sequence in the appropriate map
  std::lock_guard<std::mutex> guard(_mapMutex);
  if(sequenceName != Vision::SpriteName::Count){   
    _mappedSequences.emplace(sequenceName, sequence);
  }else{
    _unmappedSequences.emplace(Util::FileUtils::GetFileName(fullDirectoryPath), sequence);
  }
}

} // namespace Cozmo
} // namespace Anki
