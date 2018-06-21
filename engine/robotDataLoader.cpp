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

#include "engine/robotDataLoader.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationLoader.h"
#include "cannedAnimLib/spriteSequences/spriteSequenceLoader.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"
#include "engine/actions/sayTextAction.h"
#include "engine/animations/animationContainers/backpackLightAnimationContainer.h"
#include "engine/animations/animationGroup/animationGroupContainer.h"
#include "engine/animations/animationTransfer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/cozmoContext.h"
#include "engine/utils/cozmoExperiments.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "threadedPrintStressTester.h"
#include "util/ankiLab/ankiLab.h"
#include "util/cladHelpers/cladEnumToStringMap.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/dispatchWorker/dispatchWorker.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/threading/threadPriority.h"
#include "util/time/universalTime.h"
#include <json/json.h>
#include <string>
#include <sys/stat.h>

#define LOG_CHANNEL    "RobotDataLoader"

namespace {

CONSOLE_VAR(bool, kStressTestThreadedPrintsDuringLoad, "RobotDataLoader", false);

#if REMOTE_CONSOLE_ENABLED
static Anki::Cozmo::ThreadedPrintStressTester stressTester;
#endif // REMOTE_CONSOLE_ENABLED

const char* pathToExternalIndependentSprites = "assets/sprites/independentSprites/";
const char* pathToEngineIndependentSprites = "config/devOnlySprites/independentSprites/";
const char* pathToExternalSpriteSequences = "assets/sprites/spriteSequences/";
const char* pathToEngineSpriteSequences   = "config/devOnlySprites/spriteSequences/";

const std::vector<std::string> kPathsToEngineAccessibleAnimations = {
  // Dance to the beat:
  "assets/animations/anim_dancebeat_01.bin",
  "assets/animations/anim_dancebeat_getin_01.bin",
  "assets/animations/anim_dancebeat_getout_01.bin",
  
  // Weather:
  "assets/animations/anim_weather_cloud_01.bin",
  "assets/animations/anim_weather_snow_01.bin",
  "assets/animations/anim_weather_rain_01.bin",
  "assets/animations/anim_weather_sunny_01.bin",
  "assets/animations/anim_weather_stars_01.bin",
  "assets/animations/anim_weather_cold_01.bin",
  "assets/animations/anim_weather_windy_01.bin",

  // Blackjack
  "assets/animations/anim_blackjack_gameplay_01.bin",
};

}

namespace Anki {
namespace Cozmo {

RobotDataLoader::RobotDataLoader(const CozmoContext* context)
: _context(context)
, _platform(_context->GetDataPlatform())
, _animationGroups(new AnimationGroupContainer(*context->GetRandom()))
, _animationTriggerResponses(new Util::CladEnumToStringMap<AnimationTrigger>())
, _cubeAnimationTriggerResponses(new Util::CladEnumToStringMap<CubeAnimationTrigger>())
, _backpackLightAnimations(new BackpackLightAnimationContainer())
, _dasBlacklistedAnimationTriggers()
{
  _spritePaths = std::make_unique<Vision::SpritePathMap>();
  _compLayoutMap = std::make_unique<CompLayoutMap>();
  _compImageMap = std::make_unique<CompImageMap>();
}

RobotDataLoader::~RobotDataLoader()
{
  if (_dataLoadingThread.joinable()) {
    _abortLoad = true;
    _dataLoadingThread.join();
  }
}

void RobotDataLoader::LoadNonConfigData()
{
  if (_platform == nullptr) {
    return;
  }

  Anki::Util::SetThreadName(pthread_self(), "RbtDataLoader");

  // Uncomment this line to enable the profiling of loading data
  //ANKI_CPU_TICK_ONE_TIME("RobotDataLoader::LoadNonConfigData");

  ANKI_VERIFY( !_context->IsEngineThread(), "RobotDataLoadingShouldNotBeOnEngineThread", "" );

  if( kStressTestThreadedPrintsDuringLoad ) {
    REMOTE_CONSOLE_ENABLED_ONLY( stressTester.Start() );
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::CollectFiles");
    CollectAnimFiles();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadBehaviors");
    LoadBehaviors();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadWeatherResponseMaps");
    LoadWeatherResponseMaps();
  }
  
  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadBackpackLightAnimations");
    LoadBackpackLightAnimations();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadSpritePaths");
    LoadSpritePaths();
    _spriteCache = std::make_unique<Vision::SpriteCache>(_spritePaths.get());
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadSpriteSequences");
    std::vector<std::string> spriteSequenceDirs = {pathToExternalSpriteSequences, pathToEngineSpriteSequences};
    SpriteSequenceLoader seqLoader;
    auto* sContainer = seqLoader.LoadSpriteSequences(_platform, _spritePaths.get(), 
                                                     _spriteCache.get(), spriteSequenceDirs);
    _spriteSequenceContainer.reset(sContainer);
  }

  if(!FACTORY_TEST)
  {
    {
      ANKI_CPU_PROFILE("RobotDataLoader::LoadAnimationGroups");
      LoadAnimationGroups();
    }

    {
      ANKI_CPU_PROFILE("RobotDataLoader::LoadCubeLightAnimations");
      LoadCubeLightAnimations();
    }

    {
      ANKI_CPU_PROFILE("RobotDataLoader::LoadCubeAnimationTriggerResponses");
      LoadCubeAnimationTriggerResponses();
    }

    {
      ANKI_CPU_PROFILE("RobotDataLoader::LoadEmotionEvents");
      LoadEmotionEvents();
    }

    {
      ANKI_CPU_PROFILE("RobotDataLoader::LoadDasBlacklistedAnimationTriggers");
      LoadDasBlacklistedAnimationTriggers();
    }

    {
      ANKI_CPU_PROFILE("RobotDataLoader::LoadAnimationTriggerResponses");
      LoadAnimationTriggerResponses();
    }

    {
      ANKI_CPU_PROFILE("RobotDataLoader::LoadCompositeImageMaps");
      LoadCompositeImageMaps();
    }
  }
  
  {
    CannedAnimationLoader animLoader(_platform,
                                     _spritePaths.get(), _spriteSequenceContainer.get(),
                                     _loadingCompleteRatio, _abortLoad);

    // Create the canned animation container, but don't load any data into it
    // Engine side animations are loaded only when requested
    _cannedAnimations = std::make_unique<CannedAnimationContainer>();
    for(const auto& path : kPathsToEngineAccessibleAnimations){
      const auto fullPath =  _platform->pathToResource(Util::Data::Scope::Resources, path);
      animLoader.LoadAnimationIntoContainer(fullPath, _cannedAnimations.get());
    }
  }

  // this map doesn't need to be persistent
  _jsonFiles.clear();

  if( kStressTestThreadedPrintsDuringLoad ) {
    REMOTE_CONSOLE_ENABLED_ONLY( stressTester.Stop() );
  }

  // we're done
  _loadingCompleteRatio.store(1.0f);
}

void RobotDataLoader::AddToLoadingRatio(float delta)
{
  // Allows for a thread to repeatedly try to update the loading ratio until it gets access
  auto current = _loadingCompleteRatio.load();
  while (!_loadingCompleteRatio.compare_exchange_weak(current, current + delta));
}

void RobotDataLoader::CollectAnimFiles()
{
  // animations
  {
    std::vector<std::string> paths;
    if(FACTORY_TEST)
    {
      paths = {"config/engine/animations/"};
    }
    else
    {
      paths = {"assets/animations/", "config/engine/animations/"};
    }

    for (const auto& path : paths) {
      WalkAnimationDir(path, _animFileTimestamps, [this] (const std::string& filename) {
        _jsonFiles[FileType::Animation].push_back(filename);
      });
    }
  }

  // cube light animations
  {
    WalkAnimationDir("config/engine/lights/cubeLights", _cubeLightAnimFileTimestamps, [this] (const std::string& filename) {
      _jsonFiles[FileType::CubeLightAnimation].push_back(filename);
    });
  }

  // backpack light animations
  {
    WalkAnimationDir("config/engine/lights/backpackLights", _backpackLightAnimFileTimestamps, [this] (const std::string& filename) {
      _jsonFiles[FileType::BackpackLightAnimation].push_back(filename);
    });
  }

  if(!FACTORY_TEST)
  {
    // animation groups
    {
      WalkAnimationDir("assets/animationGroups/", _groupAnimFileTimestamps, [this] (const std::string& filename) {
        _jsonFiles[FileType::AnimationGroup].push_back(filename);
      });
    }
  }

  // print results
  {
    for (const auto& fileListPair : _jsonFiles) {
      PRINT_CH_INFO("Animations", "RobotDataLoader.CollectAnimFiles.Results", "Found %zu animation files of type %d",
                    fileListPair.second.size(), (int)fileListPair.first);
    }
  }
}

bool RobotDataLoader::IsCustomAnimLoadEnabled() const
{
  return (_context->IsInSdkMode() || (ANKI_DEV_CHEATS != 0));
}

void RobotDataLoader::LoadCubeLightAnimations()
{
  const auto& fileList = _jsonFiles[FileType::CubeLightAnimation];
  const auto size = fileList.size();

  const double startTime = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();

  using MyDispatchWorker = Util::DispatchWorker<3, const std::string&>;
  MyDispatchWorker::FunctionType loadFileFunc = std::bind(&RobotDataLoader::LoadCubeLightAnimationFile, 
                                                          this, std::placeholders::_1);
  MyDispatchWorker myWorker(loadFileFunc);

  for (int i = 0; i < size; i++) {
    myWorker.PushJob(fileList[i]);
  }
  
  myWorker.Process();

  const double endTime = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
  double loadTime = endTime - startTime;
  PRINT_CH_INFO("Animations", "RobotDataLoader.LoadCubeLightAnimations.LoadTime",
                "Time to load cube light animations = %.2f ms", loadTime);

}

void RobotDataLoader::LoadCubeLightAnimationFile(const std::string& path)
{
  Json::Value animDefs;
  const bool success = _platform->readAsJson(path.c_str(), animDefs);
  if (success && !animDefs.empty()) {
    std::lock_guard<std::mutex> guard(_parallelLoadingMutex);
    _cubeLightAnimations.emplace(path, animDefs);
  }
}


void RobotDataLoader::LoadBackpackLightAnimations()
{
  const double startTime = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();

  using MyDispatchWorker = Util::DispatchWorker<3, const std::string&>;
  MyDispatchWorker::FunctionType loadFileFunc = std::bind(&RobotDataLoader::LoadBackpackLightAnimationFile, this, std::placeholders::_1);
  MyDispatchWorker myWorker(loadFileFunc);

  const auto& fileList = _jsonFiles[FileType::BackpackLightAnimation];
  const auto size = fileList.size();
  for (int i = 0; i < size; i++) {
    myWorker.PushJob(fileList[i]);
  }

  myWorker.Process();

  const double endTime = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
  double loadTime = endTime - startTime;
  PRINT_CH_INFO("Animations", "RobotDataLoader.LoadBackpackLightAnimations.LoadTime",
                "Time to load backpack light animations = %.2f ms", loadTime);
}

void RobotDataLoader::LoadBackpackLightAnimationFile(const std::string& path)
{
  Json::Value animDefs;
  const bool success = _platform->readAsJson(path.c_str(), animDefs);
  if (success && !animDefs.empty()) {
    std::lock_guard<std::mutex> guard(_parallelLoadingMutex);
    _backpackLightAnimations->DefineFromJson(animDefs);
  }
}

void RobotDataLoader::LoadAnimationGroups()
{
  using MyDispatchWorker = Util::DispatchWorker<3, const std::string&>;
  MyDispatchWorker::FunctionType loadFileFunc = std::bind(&RobotDataLoader::LoadAnimationGroupFile, this, std::placeholders::_1);
  MyDispatchWorker myWorker(loadFileFunc);
  const auto& fileList = _jsonFiles[FileType::AnimationGroup];
  const auto size = fileList.size();
  for (int i = 0; i < size; i++) {
    myWorker.PushJob(fileList[i]);
    //LOG_DEBUG("RobotDataLoader.LoadAnimationGroups", "loaded anim group %d of %zu", i, size);
  }
  myWorker.Process();
}

void RobotDataLoader::WalkAnimationDir(const std::string& animationDir, TimestampMap& timestamps, const std::function<void(const std::string&)>& walkFunc)
{
  const std::string animationFolder = _platform->pathToResource(Util::Data::Scope::Resources, animationDir);
  static const std::vector<const char*> fileExts = {"json", "bin"};
  auto filePaths = Util::FileUtils::FilesInDirectory(animationFolder, true, fileExts, true);

  for (const auto& path : filePaths) {
    struct stat attrib{0};
    int result = stat(path.c_str(), &attrib);
    if (result == -1) {
      LOG_WARNING("RobotDataLoader.WalkAnimationDir", "could not get mtime for %s", path.c_str());
      continue;
    }
    bool loadFile = false;
    auto mapIt = timestamps.find(path);
#ifdef __APPLE__  // TODO: COZMO-1057
    time_t tmpSeconds = attrib.st_mtimespec.tv_sec;
#else
    time_t tmpSeconds = attrib.st_mtime;
#endif
    if (mapIt == timestamps.end()) {
      timestamps.insert({path, tmpSeconds});
      loadFile = true;
    } else {
      if (mapIt->second < tmpSeconds) {
        mapIt->second = tmpSeconds;
        loadFile = true;
      }
    }
    if (loadFile) {
      walkFunc(path);
    }
  }
}

void RobotDataLoader::LoadAnimationGroupFile(const std::string& path)
{
  if (_abortLoad.load(std::memory_order_relaxed)) {
    return;
  }
  Json::Value animGroupDef;
  const bool success = _platform->readAsJson(path, animGroupDef);
  if (success && !animGroupDef.empty()) {
    std::string animationGroupName = Util::FileUtils::GetFileName(path, true, true);

    //PRINT_CH_DEBUG("Animations", "RobotDataLoader.LoadAnimationGroupFile.LoadingSpecificAnimGroupFromJson",
    //               "Loading '%s' from %s", animationGroupName.c_str(), path.c_str());

    std::lock_guard<std::mutex> guard(_parallelLoadingMutex);
    _animationGroups->DefineFromJson(animGroupDef, animationGroupName);
  }
}

void RobotDataLoader::LoadEmotionEvents()
{
  const std::string emotionEventFolder = _platform->pathToResource(Util::Data::Scope::Resources, "config/engine/emotionevents/");
  auto eventFiles = Util::FileUtils::FilesInDirectory(emotionEventFolder, true, ".json", false);
  for (const std::string& filename : eventFiles) {
    Json::Value eventJson;
    const bool success = _platform->readAsJson(filename, eventJson);
    if (success && !eventJson.empty())
    {
      _emotionEvents.emplace(std::piecewise_construct, std::forward_as_tuple(filename), std::forward_as_tuple(std::move(eventJson)));
      LOG_DEBUG("RobotDataLoader.EmotionEvents", "Loaded '%s'", filename.c_str());
    }
    else
    {
      LOG_WARNING("RobotDataLoader.EmotionEvents", "Failed to read '%s'", filename.c_str());
    }
  }
}

void RobotDataLoader::LoadBehaviors()
{
  const std::string path =  "config/engine/behaviorComponent/behaviors/";

  const std::string behaviorFolder = _platform->pathToResource(Util::Data::Scope::Resources, path);
  auto behaviorJsonFiles = Util::FileUtils::FilesInDirectory(behaviorFolder, true, ".json", true);
  for (const auto& filename : behaviorJsonFiles)
  {
    Json::Value behaviorJson;
    const bool success = _platform->readAsJson(filename, behaviorJson);
    if (success && !behaviorJson.empty())
    {
      BehaviorID behaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behaviorJson, filename);
      auto result = _behaviors.emplace(std::piecewise_construct,
                                       std::forward_as_tuple(behaviorID),
                                       std::forward_as_tuple(std::move(behaviorJson)));

      DEV_ASSERT_MSG(result.second,
                     "RobotDataLoader.LoadBehaviors.FailedEmplace",
                     "Failed to insert BehaviorID %s - make sure all behaviors have unique IDs",
                     BehaviorTypesWrapper::BehaviorIDToString(behaviorID));

    }
    else if (!success)
    {
      LOG_WARNING("RobotDataLoader.Behavior", "Failed to read '%s'", filename.c_str());
    }
  }
}


void RobotDataLoader::LoadSpritePaths()
{
 // Creates a map of all sprite names to their file names
  const bool reverseLookupAllowed = true;
  _spritePaths->Load(_platform, "assets/cladToFileMaps/spriteMap.json", "SpriteName", reverseLookupAllowed);

  auto spritePaths = {pathToExternalIndependentSprites,
                      pathToEngineIndependentSprites};
  auto fileNameToFullPath = CreateFileNameToFullPathMap(spritePaths, "png");
  
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
    if(fullPath.empty()){
      PRINT_NAMED_ERROR("RobotDataLoader.LoadSpritePaths.EmptyPath",
                        "No path found for %s",
                        EnumToString(key));
    }else{
      _spritePaths->UpdateValue(key, std::move(fullPath));
    }
  }
}

void RobotDataLoader::LoadCompositeImageMaps()
{
  const bool useFullPath = false;
  const char* extensions = ".json";
  const bool recurse = true;
  const bool shouldCacheLookup = true;

  // Load in image layouts
  {
    // Load the layout map file and fileName Map
    const auto layoutBasePath = "assets/compositeImageResources/imageLayouts/";
    const std::string layoutFullPath = _platform->pathToResource(Util::Data::Scope::Resources,
                                                                 layoutBasePath);
    const bool reverseLookupAllowed = true;
    Util::CladEnumToStringMap<Vision::CompositeImageLayout> layoutMap;
    layoutMap.Load(_platform, "assets/cladToFileMaps/CompositeImageLayoutMap.json", "LayoutName", reverseLookupAllowed);
    auto fileNameToFullPath = CreateFileNameToFullPathMap({layoutBasePath}, "json");

    // Iterate through all files in the directory and extract the associated
    // enum value
    auto fullImagePaths = Util::FileUtils::FilesInDirectory(layoutFullPath, useFullPath, extensions, recurse);
    for(auto& fullImagePath : fullImagePaths){
      const std::string fileName = Util::FileUtils::GetFileName(fullImagePath, true, true);
      Vision::CompositeImageLayout ev = Vision::CompositeImageLayout::Count;
      if(layoutMap.GetKeyForValue(fileName, ev, shouldCacheLookup)){
        // Load the layout contents into a composite image and place it in the map
        Json::Value contents;
        const auto& fullPath = fileNameToFullPath[fileName];
        const bool success = _platform->readAsJson(fullPath, contents);
        if(success){
          auto compImg = Vision::CompositeImage(_spriteCache.get(), ProceduralFace::GetHueSatWrapper(),contents);
          _compLayoutMap->emplace(ev, std::move(compImg));
        }
      }else{
        PRINT_NAMED_WARNING("RobotDataLoader.LoadCompositeImageLayouts",
                            "Failed to find %s in map", 
                            CompositeImageLayoutToString(ev));
      }
    }
  }

  // Load in image map
  {
    // Load the image map file and fileName Map
    const auto mapBasePath = "assets/compositeImageResources/imageMaps/";
    const std::string mapFullPath = _platform->pathToResource(Util::Data::Scope::Resources,
                                                              mapBasePath);
    
    const bool reverseLookupAllowed = true;
    Util::CladEnumToStringMap<Vision::CompositeImageMap> mapMap;
    mapMap.Load(_platform, "assets/cladToFileMaps/CompositeImageMapMap.json", "MapName", reverseLookupAllowed);
    auto fileNameToFullPath = CreateFileNameToFullPathMap({mapBasePath}, "json");

    // Iterate through all files in the directory and extract the associated
    // enum value
    auto fullImagePaths = Util::FileUtils::FilesInDirectory(mapFullPath, useFullPath, extensions, recurse);
    for(auto& fullImagePath : fullImagePaths){
      const std::string fileName = Util::FileUtils::GetFileName(fullImagePath, true, true);
      Vision::CompositeImageMap ev = Vision::CompositeImageMap::Count;
      if(mapMap.GetKeyForValue(fileName, ev, shouldCacheLookup)){
        // Load the layout contents into a composite image and place it in the map
        Json::Value contents;
        const auto& fullPath = fileNameToFullPath[fileName];
        const bool success = _platform->readAsJson(fullPath, contents);
        if(success){
          const std::string debugStr = "RobotDataLoader.LoadCompositeImageMaps.";

          Vision::CompositeImage::LayerImageMap fullImageMap;
          for(auto& mapEntry: contents){
            // Extract Layer Name
            const std::string strLayerName = JsonTools::ParseString(mapEntry, Vision::CompositeImageConfigKeys::kLayerNameKey, debugStr + "NoLayerName");
            const Vision::LayerName layerName = Vision::LayerNameFromString(strLayerName);

            // Extract all image entries for the layer
            if(mapEntry.isMember(Vision::CompositeImageConfigKeys::kImagesListKey)){
              Vision::CompositeImageLayer::ImageMap partialMap;
              Json::Value imageArray = mapEntry[Vision::CompositeImageConfigKeys::kImagesListKey];
              for(auto& imageEntry: imageArray){
                // Extract Sprite Box Name
                const std::string strSpriteBox = JsonTools::ParseString(imageEntry, Vision::CompositeImageConfigKeys::kSpriteBoxNameKey, debugStr + "NoSpriteBoxName");
                const Vision::SpriteBoxName sbName = Vision::SpriteBoxNameFromString(strSpriteBox);
                // Extract Sprite Name
                const std::string strSpriteName = JsonTools::ParseString(imageEntry, Vision::CompositeImageConfigKeys::kSpriteNameKey, debugStr + "NoSpriteName");
                const Vision::SpriteName spriteName = Vision::SpriteNameFromString(strSpriteName);

                auto spriteEntry = Vision::CompositeImageLayer::SpriteEntry(_spriteCache.get(), _spriteSequenceContainer.get(), spriteName);
                partialMap.emplace(sbName, std::move(spriteEntry));
              }
              
              fullImageMap.emplace(layerName, partialMap);
            }else{
              PRINT_NAMED_WARNING("RobotDataLoader.LoadCompositeImageMap.MissingKey", 
                                  "Missing image map key %s",
                                  Vision::CompositeImageConfigKeys::kImagesListKey);
            }
          }// end for(contents)

          _compImageMap->emplace(ev, std::move(fullImageMap));
        }
      }else{
        PRINT_NAMED_WARNING("RobotDataLoader.LoadCompositeImageMaps",
                            "Failed to find %s in map", 
                            Vision::CompositeImageMapToString(ev));
      }
    }
  }
}


void RobotDataLoader::LoadWeatherResponseMaps()
{
  _weatherResponseMap = std::make_unique<WeatherResponseMap>();
  const bool useFullPath = false;
  const char* extensions = ".json";
  const bool recurse = true;


  const std::string path =  "config/engine/behaviorComponent/weatherResponseMaps/";
  const char* kAPIValueKey = "APIValue";
  const char* kCladTypeKey = "CladType";

  const std::string responseFolder = _platform->pathToResource(Util::Data::Scope::Resources, path);
  auto responseJSONFiles = Util::FileUtils::FilesInDirectory(responseFolder, useFullPath, extensions, recurse);
  for (const auto& filename : responseJSONFiles)
  {
    Json::Value responseJSON;
    const bool success = _platform->readAsJson(filename, responseJSON);
    if (success && 
        !responseJSON.empty() &&
        responseJSON.isArray())
    {
      for(const auto& pair : responseJSON){
        if(ANKI_VERIFY(pair.isMember(kAPIValueKey) && pair.isMember(kCladTypeKey),
                       "RobotDataLoader.LoadWeatherResponseMaps.PairMissingKey",
                       "File %s has an invalid pair",
                       filename.c_str())){
          WeatherConditionType cond = WeatherConditionTypeFromString(pair[kCladTypeKey].asString());

          std::string str = pair[kAPIValueKey].asString();
          std::transform(str.begin(), str.end(), str.begin(), 
                         [](const char c) { return std::tolower(c); });
          
          if(!str.empty()){
            auto resPair  = _weatherResponseMap->emplace(std::make_pair(str, cond));
            ANKI_VERIFY(resPair.second,
                        "RobotDataLoader.LoadWeatherResponseMaps.DuplicateAPIKey",
                        "Key %s already exists within the weather response map ",
                        str.c_str());
          }else{
            PRINT_NAMED_ERROR("RobotDataLoader.LoadWeatherResponseMaps.MissingAPIValue",
                              "APIValue that maps to %s in file %s is blank",
                              WeatherConditionTypeToString(cond), filename.c_str());
          }

        }
      }
    }
    else if (!success)
    {
      LOG_WARNING("RobotDataLoader.LoadWeatherResponseMap", "Failed to read '%s'", filename.c_str());
    }
  }


}


std::map<std::string, std::string> RobotDataLoader::CreateFileNameToFullPathMap(const std::vector<const char*> & srcDirs, const std::string& fileExtensions) const
{
  std::map<std::string, std::string> fileNameToFullPath;
  // Get all independent sprites
  {
    const bool useFullPath = true;
    const bool recurse = true;
    for(const auto& dir: srcDirs){
      const std::string fullPathFolder = _platform->pathToResource(Util::Data::Scope::Resources, dir);

      auto fullImagePaths = Util::FileUtils::FilesInDirectory(fullPathFolder, useFullPath, fileExtensions.c_str(), recurse);
      for(auto& fullImagePath : fullImagePaths){
        const std::string fileName = Util::FileUtils::GetFileName(fullImagePath, true, true);
        fileNameToFullPath.emplace(fileName, fullImagePath);
      }
    }
  }

  return fileNameToFullPath;
}

void RobotDataLoader::LoadAnimationTriggerResponses()
{
  _animationTriggerResponses->Load(_platform, "assets/cladToFileMaps/AnimationTriggerMap.json", "AnimName");
}

void RobotDataLoader::LoadCubeAnimationTriggerResponses()
{
  _cubeAnimationTriggerResponses->Load(_platform, "assets/cladToFileMaps/CubeAnimationTriggerMap.json", "AnimName");
}

void RobotDataLoader::LoadDasBlacklistedAnimationTriggers()
{
  static const std::string kBlacklistedAnimationTriggersConfigKey = "blacklisted_animation_triggers";
  const Json::Value& blacklistedTriggers = _dasEventConfig[kBlacklistedAnimationTriggersConfigKey];
  for (int i = 0; i < blacklistedTriggers.size(); i++)
  {
    const std::string& trigger = blacklistedTriggers[i].asString();
    _dasBlacklistedAnimationTriggers.insert(AnimationTriggerFromString(trigger));
  }
}


void RobotDataLoader::LoadRobotConfigs()
{
  if (_platform == nullptr) {
    return;
  }

  ANKI_CPU_TICK_ONE_TIME("RobotDataLoader::LoadRobotConfigs");
  // mood config
  {
    static const std::string jsonFilename = "config/engine/mood_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _robotMoodConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.MoodConfigJsonNotFound",
                "Mood Json config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }

  // victor behavior systems config
  {
    static const std::string jsonFilename = "config/engine/behaviorComponent/victor_behavior_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _victorFreeplayBehaviorConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.BehaviorSystemJsonFailed",
                "Behavior Json config file %s not found or failed to parse",
                jsonFilename.c_str());
      _victorFreeplayBehaviorConfig.clear();
    }
  }

  // vision config
  {
    static const std::string jsonFilename = "config/engine/vision_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _robotVisionConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.VisionConfigJsonNotFound",
                "Vision Json config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }

  // visionScheduleMediator config
  {
    static const std::string jsonFilename = "config/engine/visionScheduleMediator_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _visionScheduleMediatorConfig);
    if(!success)
    {
      LOG_ERROR("RobotDataLoader.VisionScheduleMediatorConfigNotFound",
                "VisionScheduleMediator Json config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }

  // userIntentsComponent config (also maps cloud intents to user intents)
  {
    static const std::string jsonFilename = "config/engine/behaviorComponent/user_intent_map.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _userIntentsConfig);
    if(!success)
    {
      LOG_ERROR("RobotDataLoader.UserIntentsConfigNotFound",
                "UserIntents Json config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }

  // DAS event config
  {
    static const std::string jsonFilename = "config/engine/das_event_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _dasEventConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.DasEventConfigJsonNotFound",
                "DAS Event Json config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }

  // feature gate
  {
    const std::string filename{_platform->pathToResource(Util::Data::Scope::Resources, "config/features.json")};
    const std::string fileContents{Util::FileUtils::ReadFile(filename)};
    _context->GetFeatureGate()->Init(fileContents);
  }

  // A/B testing definition
  {
    const std::string filename{_platform->pathToResource(Util::Data::Scope::Resources, "config/experiments.json")};
    const std::string fileContents{Util::FileUtils::ReadFile(filename)};
    _context->GetExperiments()->GetAnkiLab().Load(fileContents);
  }

  // Inventory config
  {
    static const std::string jsonFilename = "config/engine/inventory_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _inventoryConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.InventoryConfigNotFound",
                "Inventory Config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }

  // Web server config
  {
    static const std::string jsonFilename = "webserver/webServerConfig_engine.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _webServerEngineConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.WebServerEngineConfigNotFound",
                "Web Server Engine Config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }

  // Photography config
  {
    static const std::string jsonFilename = "config/engine/photography_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _photographyConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.PhotographyConfigNotFound",
                "Photography Config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }

  // Settings config
  {
    static const std::string jsonFilename = "config/engine/settings_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _settingsConfig);
    if (!success)
    {
      LOG_ERROR("RobotDataLoader.SettingsConfigNotFound",
                "Settings Config file %s not found or failed to parse",
                jsonFilename.c_str());
    }
  }
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

bool RobotDataLoader::HasAnimationForTrigger( AnimationTrigger ev )
{
  return _animationTriggerResponses->HasKey(ev);
}
std::string RobotDataLoader::GetAnimationForTrigger( AnimationTrigger ev )
{
  return _animationTriggerResponses->GetValue(ev);
}
std::string RobotDataLoader::GetCubeAnimationForTrigger( CubeAnimationTrigger ev )
{
  return _cubeAnimationTriggerResponses->GetValue(ev);
}




}
}
