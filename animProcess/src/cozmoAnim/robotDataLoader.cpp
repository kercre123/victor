/**
 * File: robotDataLoader
 *
 * Author: baustin
 * Created: 6/10/16
 *
 * Description: Loads and holds static data robots use for initialization
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "cozmoAnim/robotDataLoader.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"

#include "cannedAnimLib/cannedAnims/animation.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationLoader.h"
#include "cannedAnimLib/baseTypes/cozmo_anim_generated.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"
#include "cannedAnimLib/spriteSequences/spriteSequenceLoader.h"
#include "cannedAnimLib/proceduralFace/proceduralFace.h"
//#include "anki/cozmo/basestation/animations/animationTransfer.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animProcessMessages.h"

#include "util/console/consoleInterface.h"
#include "util/dispatchWorker/dispatchWorker.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/threading/threadPriority.h"
#include "util/time/universalTime.h"
#include <json/json.h>
#include <string>
#include <sys/stat.h>

#define LOG_CHANNEL   "RobotDataLoader"

namespace Anki {
namespace Vector {

namespace{
const char* pathToExternalIndependentSprites = "assets/sprites/independentSprites/";
const char* pathToEngineIndependentSprites = "config/devOnlySprites/independentSprites/";
const char* pathToExternalSpriteSequences = "assets/sprites/spriteSequences/";
const char* pathToEngineSpriteSequences   = "config/devOnlySprites/spriteSequences/";
const char* kProceduralAnimName = "_PROCEDURAL_";
}

RobotDataLoader::RobotDataLoader(const AnimContext* context)
: _context(context)
, _platform(_context->GetDataPlatform())
, _cannedAnimations(nullptr)
{
  _spritePaths = std::make_unique<Vision::SpritePathMap>();
  _spriteCache = std::make_unique<Vision::SpriteCache>(_spritePaths.get());
}

RobotDataLoader::~RobotDataLoader()
{
  if (_dataLoadingThread.joinable()) {
    _abortLoad = true;
    _dataLoadingThread.join();
  }
}

void RobotDataLoader::LoadConfigData()
{
  // Text-to-speech config
  {
    static const std::string & tts_config = "config/engine/tts_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, tts_config, _tts_config);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.TextToSpeechConfigNotFound",
                "Text-to-speech config file %s not found or failed to parse",
                  tts_config.c_str());
    }
  }
  // Web server config
  {
    static const std::string & ws_config = "webserver/webServerConfig_anim.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, ws_config, _ws_config);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.WebServerAnimConfigNotFound",
                "Web server anim config file %s not found or failed to parse",
                  ws_config.c_str());
    }
  }
  // Mic data config
  {
    const std::string& triggerConfigFile = "config/micData/micTriggerConfig.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, triggerConfigFile, _micTriggerConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.MicTriggerConfigNotFound",
                "Mic trigger config file %s not found or failed to parse",
                triggerConfigFile.c_str());
    }
  }
}

void RobotDataLoader::LoadNonConfigData()
{
  if (_platform == nullptr) {
    return;
  }
  
  // Dependency Order:
  //  1) SpritePaths load map of sprite name -> full file path
  //  2) SpriteSequences use sprite map to load sequenceName -> all images in sequence directory
  //  3) Canned animations use SpriteSequences for their FaceAnimation keyframe
  LoadSpritePaths();
  {
    std::vector<std::string> spriteSequenceDirs = {pathToExternalSpriteSequences, pathToEngineSpriteSequences};
    SpriteSequenceLoader seqLoader;
    auto* sContainer = seqLoader.LoadSpriteSequences(_platform, _spritePaths.get(), 
                                                     _spriteCache.get(), spriteSequenceDirs);
    _spriteSequenceContainer.reset(sContainer);
  }

  {
    // Set up container
    _cannedAnimations = std::make_unique<CannedAnimationContainer>();

    // Gather the files to load into the animation container
    CannedAnimationLoader animLoader(_platform,
                                    _spritePaths.get(), _spriteSequenceContainer.get(), 
                                    _loadingCompleteRatio, _abortLoad);

    std::vector<std::string> paths;
    if(FACTORY_TEST)
    {
      // Only need to load engine animations
      paths = {"config/engine/animations/"};
    }
    else
    {
      paths = {"assets/animations/", "config/engine/animations/"};
    }

    // Load the gathered files into the container
    const auto& fileInfo = animLoader.CollectAnimFiles(paths);
    animLoader.LoadAnimationsIntoContainer(fileInfo, _cannedAnimations.get());
  }
  
  SetupProceduralAnimation();
}

void RobotDataLoader::LoadAnimationFile(const std::string& path)
{
  if (_platform == nullptr) {
    return;
  }
  CannedAnimationLoader animLoader(_platform,
                                   _spritePaths.get(), _spriteSequenceContainer.get(), 
                                   _loadingCompleteRatio, _abortLoad);
  
  animLoader.LoadAnimationIntoContainer(path, _cannedAnimations.get());

  const auto animName = Util::FileUtils::GetFileName(path, true, true);
  const auto* anim = _cannedAnimations->GetAnimation(animName);
  NotifyAnimAdded(animName, anim->GetLastKeyFrameEndTime_ms());
}

void RobotDataLoader::LoadSpritePaths()
{
  // Creates a map of all sprite names to their file names
  const bool reverseLookupAllowed = true;
  _spritePaths->Load(_platform, "assets/cladToFileMaps/spriteMap.json", "SpriteName", reverseLookupAllowed);

  std::map<std::string, std::string> fileNameToFullPath;
  // Get all independent sprites
  {
    auto spritePaths = {pathToExternalIndependentSprites,
                        pathToEngineIndependentSprites};
    
    const bool useFullPath = true;
    const char* extensions = "png";
    const bool recurse = true;
    for(const auto& path: spritePaths){
      const std::string fullPathFolder = _platform->pathToResource(Util::Data::Scope::Resources, path);

      auto fullImagePaths = Util::FileUtils::FilesInDirectory(fullPathFolder, useFullPath, extensions, recurse);
      for(auto& fullImagePath : fullImagePaths){
        const std::string fileName = Util::FileUtils::GetFileName(fullImagePath, true, true);
        fileNameToFullPath.emplace(fileName, fullImagePath);
      }
    }
  }
  
  // Get all sprite sequences with recursive directory search
  {
    std::vector<std::string> directoriesToSearch = {
       _platform->pathToResource(Util::Data::Scope::Resources, pathToExternalSpriteSequences),
       _platform->pathToResource(Util::Data::Scope::Resources, pathToEngineSpriteSequences)};
    
    auto searchIter = directoriesToSearch.begin();
    while(searchIter != directoriesToSearch.end()){
      // Get all directories at this level and add them to the file map
      std::vector<std::string> outDirNames;
      Util::FileUtils::ListAllDirectories(*searchIter, outDirNames);
      for(auto& dirName: outDirNames){
        // turn name into full path
        dirName = Util::FileUtils::FullFilePath({*searchIter, dirName});
        fileNameToFullPath.emplace(Util::FileUtils::GetFileName(dirName), dirName);
      }
      directoriesToSearch.erase(searchIter);
      
      // Add directories for recursive search and advance to next directory
      copy(outDirNames.begin(), outDirNames.end(), back_inserter(directoriesToSearch));
      searchIter = directoriesToSearch.begin();
    }
  }
  
  for (auto key : _spritePaths->GetAllKeys()) {
    auto fullPath = fileNameToFullPath[_spritePaths->GetValue(key)];
    _spritePaths->UpdateValue(key, std::move(fullPath));
  }
}


Animation* RobotDataLoader::GetCannedAnimation(const std::string& name)
{
  DEV_ASSERT(_cannedAnimations != nullptr, "_cannedAnimations");
  return _cannedAnimations->GetAnimation(name);
}

std::vector<std::string> RobotDataLoader::GetAnimationNames()
{
  DEV_ASSERT(_cannedAnimations != nullptr, "_cannedAnimations");
  return _cannedAnimations->GetAnimationNames();
}

void RobotDataLoader::NotifyAnimAdded(const std::string& animName, uint32_t animLength)
{
  AnimationAdded msg;
  memcpy(msg.animName, animName.c_str(), animName.length());
  msg.animName_length = animName.length();
  msg.animLength = animLength;
  AnimProcessMessages::SendAnimToEngine(msg);
}
  
void RobotDataLoader::SetupProceduralAnimation()
{
  // TODO: kevink - This should probably live somewhere else but since robot data loader
  // currently maintains control of both canned animations and sprite sequences this
  // is the best spot to put it for the time being
  Animation proceduralAnim(kProceduralAnimName);
  bool outOverwrite = false;
  _cannedAnimations->AddAnimation(std::move(proceduralAnim), outOverwrite);
  
  assert(_cannedAnimations->GetAnimation(kProceduralAnimName) != nullptr);
}

bool RobotDataLoader::DoNonConfigDataLoading(float& loadingCompleteRatio_out)
{
  loadingCompleteRatio_out = _loadingCompleteRatio.load();
  
  if (_isNonConfigDataLoaded)
  {
    return true;
  }
  
  // loading hasn't started
  if (!_dataLoadingThread.joinable())
  {
    // start loading
    _dataLoadingThread = std::thread(&RobotDataLoader::LoadNonConfigData, this);
    return false;
  }
  
  // loading has started but isn't complete
  if (loadingCompleteRatio_out < 1.0f)
  {
    return false;
  }
  
  // loading is now done so lets clean up
  _dataLoadingThread.join();
  _dataLoadingThread = std::thread();
  _isNonConfigDataLoaded = true;
  
  return true;
}

}
}
