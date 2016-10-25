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

#include "anki/cozmo/basestation/robotDataLoader.h"

#include "anki/cozmo/basestation/animationGroup/animationGroupContainer.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/animationTriggerResponsesContainer.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/animations/animationTransfer.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/dispatchWorker/dispatchWorker.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include <json/json.h>
#include <string>
#include <sys/stat.h>


namespace Anki {
namespace Cozmo {

RobotDataLoader::RobotDataLoader(const CozmoContext* context)
: _context(context)
, _platform(_context->GetDataPlatform())
, _cannedAnimations(new CannedAnimationContainer())
, _animationGroups(new AnimationGroupContainer())
, _animationTriggerResponses(new AnimationTriggerResponsesContainer())
{
}

RobotDataLoader::~RobotDataLoader()
{
  if (_dataLoadingThread.joinable()) {
    _abortLoad = true;
    _dataLoadingThread.join();
  }
}
  
// We report some loading data info so the UI can inform the user. Ratio of time taken per section is approximate,
// based on recent profiling. Some sections below are called out specifically, the rest makes up the remainder.
// These should add up to be less than or equal to 1.0!
static constexpr float _kAnimationsLoadingRatio = 0.7f;
static constexpr float _kFaceAnimationsLoadingRatio = 0.2f;

void RobotDataLoader::LoadNonConfigData()
{
  if (_platform == nullptr) {
    return;
  }

  // Uncomment this line to enable the profiling of loading data
  //ANKI_CPU_TICK_ONE_TIME("RobotDataLoader::LoadNonConfigData");
  
  {
    ANKI_CPU_PROFILE("RobotDataLoader::CollectFiles");
    CollectJsonFiles();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadAnimations");
    LoadAnimationsInternal();
    // The threaded animation loading workers each add to the loading ratio
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadAnimationGroups");
    LoadAnimationGroups();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadFaceAnimations");
    LoadFaceAnimations();
    AddToLoadingRatio(_kFaceAnimationsLoadingRatio);
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadEmotionEvents");
    LoadEmotionEvents();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadBehaviors");
    LoadBehaviors();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadGameEventResponses");
    LoadAnimationTriggerResponses();
  }
  
  {
    // Load SayText Action Intent Config
    ANKI_CPU_PROFILE("RobotDataLoader::LoadSayTextActionIntentConfigs");
    SayTextAction::LoadMetadata(*_context->GetDataPlatform());
  }

  // this map doesn't need to be persistent
  _jsonFiles.clear();
  
  // we're done
  _loadingCompleteRatio.store(1.0f);
}
  
void RobotDataLoader::AddToLoadingRatio(float delta)
{
  // Allows for a thread to repeatedly try to update the loading ratio until it gets access
  auto current = _loadingCompleteRatio.load();
  while (!_loadingCompleteRatio.compare_exchange_weak(current, current + delta));
}
  
void RobotDataLoader::CollectJsonFiles()
{
  // animations
  {
    const std::vector<std::string> paths = {"assets/animations/", "config/basestation/animations/"};
    for (const auto& path : paths) {
      WalkAnimationDir(path, _animFileTimestamps, [this] (const std::string& filename) {
        _jsonFiles[FileType::Animation].push_back(filename);
      });
    }
  }

  // animation groups
  {
    WalkAnimationDir("assets/animationGroups/", _groupAnimFileTimestamps, [this] (const std::string& filename) {
      _jsonFiles[FileType::AnimationGroup].push_back(filename);
    });
  }

  // print results
  {
    for (const auto& fileListPair : _jsonFiles) {
      PRINT_NAMED_INFO("RobotDataLoader.CollectJsonFiles", "found %zu json files of type %d", fileListPair.second.size(), (int)fileListPair.first);
    }
  }
}

void RobotDataLoader::LoadAnimations()
{
  CollectJsonFiles();
  LoadAnimationsInternal();
  _jsonFiles.clear();
}

void RobotDataLoader::LoadAnimationsInternal()
{
  // Disable super-verbose warnings about clipping face parameters in json files
  // To help find bad/deprecated animations, try removing this.
  ProceduralFace::EnableClippingWarning(false);
  
  using MyDispatchWorker = Util::DispatchWorker<3, const std::string&>;
  MyDispatchWorker::FunctionType loadFileFunc = std::bind(&RobotDataLoader::LoadAnimationFile, this, std::placeholders::_1);
  MyDispatchWorker myWorker(loadFileFunc);

  const auto& fileList = _jsonFiles[FileType::Animation];
  const auto size = fileList.size();
  for (int i = 0; i < size; i++) {
    myWorker.PushJob(fileList[i]);
    //PRINT_NAMED_DEBUG("RobotDataLoader.LoadAnimations", "loaded regular anim %d of %zu", i, size);
  }
  _perAnimationLoadingRatio = _kAnimationsLoadingRatio * 1.0f / Util::numeric_cast<float>(size);
  myWorker.Process();
  
#if ANKI_DEV_CHEATS
  std::string test_anim = _platform->pathToResource(Util::Data::Scope::Cache, AnimationTransfer::kCacheAnimFileName);
  if (Util::FileUtils::FileExists(test_anim)) {
    LoadAnimationFile(test_anim);
  }
#endif

  ProceduralFace::EnableClippingWarning(true);
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
    //PRINT_NAMED_DEBUG("RobotDataLoader.LoadAnimationGroups", "loaded anim group %d of %zu", i, size);
  }
  myWorker.Process();
}

void RobotDataLoader::WalkAnimationDir(const std::string& animationDir, TimestampMap& timestamps, const std::function<void(const std::string&)>& walkFunc)
{
  const std::string animationFolder = _platform->pathToResource(Util::Data::Scope::Resources, animationDir);
  auto filePaths = Util::FileUtils::FilesInDirectory(animationFolder, true, "json", true);

  for (const auto& path : filePaths) {
    struct stat attrib{0};
    int result = stat(path.c_str(), &attrib);
    if (result == -1) {
      PRINT_NAMED_WARNING("RobotDataLoader.WalkAnimationDir", "could not get mtime for %s", path.c_str());
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

void RobotDataLoader::LoadAnimationFile(const std::string& path)
{
  if (_abortLoad.load(std::memory_order_relaxed)) {
    return;
  }
  Json::Value animDefs;
  // add json filename and callback (to perform load) here?
  const bool success = _platform->readAsJson(path.c_str(), animDefs);
  std::string animationId;
  if (success && !animDefs.empty()) {
    std::lock_guard<std::mutex> guard(_parallelLoadingMutex);
    _cannedAnimations->DefineFromJson(animDefs, animationId);

    if(path.find(animationId) == std::string::npos) {
      PRINT_NAMED_WARNING("RobotDataLoader.LoadAnimationFile.AnimationNameMismatch",
                          "Animation name '%s' does not match seem to match "
                          "filename '%s'", animationId.c_str(), path.c_str());
    }
  }
  AddToLoadingRatio(_perAnimationLoadingRatio);
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

    PRINT_NAMED_INFO("RobotDataLoader.LoadAnimationGroupFile", "reading %s - %s", animationGroupName.c_str(), path.c_str());

    std::lock_guard<std::mutex> guard(_parallelLoadingMutex);
    ASSERT_NAMED(nullptr != _cannedAnimations, "RobotDataLoader.LoadAnimationGroupFile.NullCannedAnimations");
    _animationGroups->DefineFromJson(animGroupDef, animationGroupName, _cannedAnimations.get());
  }
}

void RobotDataLoader::LoadEmotionEvents()
{
  const std::string emotionEventFolder = _platform->pathToResource(Util::Data::Scope::Resources, "config/basestation/config/emotionevents/");
  auto eventFiles = Util::FileUtils::FilesInDirectory(emotionEventFolder, true, ".json", false);
  for (const std::string& filename : eventFiles) {
    Json::Value eventJson;
    const bool success = _platform->readAsJson(filename, eventJson);
    if (success && !eventJson.empty())
    {
      _emotionEvents.emplace(std::piecewise_construct, std::forward_as_tuple(filename), std::forward_as_tuple(std::move(eventJson)));
      PRINT_NAMED_DEBUG("RobotDataLoader.EmotionEvents", "Loaded '%s'", filename.c_str());
    }
    else
    {
      PRINT_NAMED_WARNING("RobotDataLoader.EmotionEvents", "Failed to read '%s'", filename.c_str());
    }
  }
}

void RobotDataLoader::LoadBehaviors()
{
  // rsam: 05/02/16 we are moving behaviors to basestation, but at the moment we support both paths for legacy
  // reasons. Eventually I will move them to BS
  const std::vector<std::string> paths = {"assets/behaviors/", "config/basestation/config/behaviors/"};

  for (const std::string& path : paths) {
    const std::string behaviorFolder = _platform->pathToResource(Util::Data::Scope::Resources, path);
    auto behaviorJsonFiles = Util::FileUtils::FilesInDirectory(behaviorFolder, true, ".json", true);
    for (const auto& filename : behaviorJsonFiles)
    {
      Json::Value behaviorJson;
      const bool success = _platform->readAsJson(filename, behaviorJson);
      if (success && !behaviorJson.empty())
      {
        _behaviors.emplace(std::piecewise_construct, std::forward_as_tuple(filename), std::forward_as_tuple(std::move(behaviorJson)));
      }
      else if (!success)
      {
        PRINT_NAMED_WARNING("RobotDataLoader.Behavior", "Failed to read '%s'", filename.c_str());
      }
    }
  }
}

void RobotDataLoader::LoadFaceAnimations()
{
  FaceAnimationManager::getInstance()->ReadFaceAnimationDir(_platform);
#if ANKI_DEV_CHEATS
  FaceAnimationManager::getInstance()->ReadFaceAnimationDir(_platform, true);
#endif
}

void RobotDataLoader::LoadAnimationTriggerResponses()
{
  _animationTriggerResponses->Load(_platform, "assets/animationGroupMaps");
}

void RobotDataLoader::LoadRobotConfigs()
{
  if (_platform == nullptr) {
    return;
  }
  
  ANKI_CPU_TICK_ONE_TIME("RobotDataLoader::LoadRobotConfigs");
  // mood config
  {
    std::string jsonFilename = "config/basestation/config/mood_config.json";
    bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _robotMoodConfig);
    if (!success)
    {
      PRINT_NAMED_ERROR("RobotDataLoader.MoodConfigJsonNotFound",
                        "Mood Json config file %s not found.",
                        jsonFilename.c_str());
    }
  }

  // behavior config
  {
    std::string jsonFilename = "config/basestation/config/behavior_config.json";
    bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _robotBehaviorConfig);
    if (!success)
    {
      PRINT_NAMED_ERROR("RobotDataLoader.BehaviorConfigJsonFailed",
                        "Behavior Json config file %s failed to parse.",
                        jsonFilename.c_str());
      _robotBehaviorConfig.clear();
    }
  }

  // vision config
  {
    std::string jsonFilename = "config/basestation/config/vision_config.json";
    bool success = _platform->readAsJson(Util::Data::Scope::Resources, jsonFilename, _robotVisionConfig);
    if (!success)
    {
      PRINT_NAMED_ERROR("RobotDataLoader.VisionConfigJsonNotFound",
                        "Vision Json config file %s not found.",
                        jsonFilename.c_str());
    }
  }

  // feature gate
  {
    const std::string filename{_platform->pathToResource(Util::Data::Scope::Resources, "config/features.json")};
    const std::string fileContents{Util::FileUtils::ReadFile(filename)};
    _context->GetFeatureGate()->Init(fileContents);
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

}
}
