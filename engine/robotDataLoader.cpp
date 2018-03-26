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

#include "cannedAnimLib/cannedAnimationContainer.h"
#include "cannedAnimLib/cannedAnimationLoader.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/sayTextAction.h"
#include "engine/animations/animationContainers/backpackLightAnimationContainer.h"
#include "engine/animations/animationContainers/cubeLightAnimationContainer.h"
#include "engine/animations/animationGroup/animationGroupContainer.h"
#include "engine/animations/animationTransfer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/cubes/cubeLightComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/animationTriggerResponsesContainer.h"
#include "engine/utils/cozmoExperiments.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "threadedPrintStressTester.h"
#include "util/ankiLab/ankiLab.h"
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


}

namespace Anki {
namespace Cozmo {

RobotDataLoader::RobotDataLoader(const CozmoContext* context)
: _context(context)
, _platform(_context->GetDataPlatform())
, _cubeLightAnimations(new CubeLightAnimationContainer())
, _animationGroups(new AnimationGroupContainer(*context->GetRandom()))
, _animationTriggerResponses(new AnimationTriggerResponsesContainer())
, _cubeAnimationTriggerResponses(new AnimationTriggerResponsesContainer())
, _backpackLightAnimations(new BackpackLightAnimationContainer())
, _dasBlacklistedAnimationTriggers()
{
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
    ANKI_CPU_PROFILE("RobotDataLoader::LoadAnimationGroups");
    LoadAnimationGroups();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadBackpackLightAnimations");
    LoadBackpackLightAnimations();
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
    ANKI_CPU_PROFILE("RobotDataLoader::LoadFacePNGPaths");
    LoadFacePNGPaths();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadDasBlacklistedAnimationTriggers");
    LoadDasBlacklistedAnimationTriggers();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadBackpackLightAnimations");
    LoadBackpackLightAnimations();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadBehaviors");
    LoadBehaviors();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadAnimationTriggerResponses");
    LoadAnimationTriggerResponses();
  }

  {
    // Load SayText Action Intent Config
    ANKI_CPU_PROFILE("RobotDataLoader::LoadSayTextActionIntentConfigs");
    SayTextAction::LoadMetadata(*_context->GetDataPlatform());
  }

  {
    // Load animations into engine - disabled for the time being to save the 30 MB hit
    // of loading animations into engine in addition to anim process
    //CannedAnimationLoader animLoader(_platform, _loadingCompleteRatio, _abortLoad);
    //_cannedAnimations.reset(animLoader.LoadAnimations());
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
  const double startTime = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();

  using MyDispatchWorker = Util::DispatchWorker<3, const std::string&>;
  MyDispatchWorker::FunctionType loadFileFunc = std::bind(&RobotDataLoader::LoadCubeLightAnimationFile, this, std::placeholders::_1);
  MyDispatchWorker myWorker(loadFileFunc);

  const auto& fileList = _jsonFiles[FileType::CubeLightAnimation];
  const auto size = fileList.size();
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
    _cubeLightAnimations->DefineFromJson(animDefs);
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

    std::string fullName(path);

    // remove path
    auto slashIndex = fullName.find_last_of("/");
    std::string jsonName = slashIndex == std::string::npos ? fullName : fullName.substr(slashIndex + 1);
    // remove extension
    auto dotIndex = jsonName.find_last_of(".");
    std::string animationGroupName = dotIndex == std::string::npos ? jsonName : jsonName.substr(0, dotIndex);

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


void RobotDataLoader::LoadFacePNGPaths()
{
  const std::string facePNGFolder = _platform->pathToResource(Util::Data::Scope::Resources, "config/facePNGs/");
  auto pngs = Util::FileUtils::FilesInDirectory(facePNGFolder, true, ".png", true);
  for (const std::string& fullPath : pngs) {
    std::string fileName = Util::FileUtils::GetFileName(fullPath);
    auto dotIndex = fileName.find_last_of(".");
    std::string strippedName = (dotIndex == std::string::npos) ? fileName : fileName.substr(0, dotIndex);
    _facePNGPaths.emplace(std::piecewise_construct, std::forward_as_tuple(strippedName), std::forward_as_tuple(std::move(fullPath)));
  }
}


void RobotDataLoader::LoadAnimationTriggerResponses()
{
  _animationTriggerResponses->Load(_platform, "assets/animationGroupMaps");
}

void RobotDataLoader::LoadCubeAnimationTriggerResponses()
{
  _cubeAnimationTriggerResponses->Load(_platform, "assets/cubeAnimationGroupMaps");
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
  return _animationTriggerResponses->HasResponse(ev);
}
std::string RobotDataLoader::GetAnimationForTrigger( AnimationTrigger ev )
{
  return _animationTriggerResponses->GetResponse(ev);
}
std::string RobotDataLoader::GetCubeAnimationForTrigger( CubeAnimationTrigger ev )
{
  return _cubeAnimationTriggerResponses->GetResponse(ev);
}




}
}
