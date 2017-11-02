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

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"


#include "cozmoAnim/animation/cannedAnimationContainer.h"
#include "cozmoAnim/animation/cozmo_anim_generated.h"
#include "cozmoAnim/animation/faceAnimationManager.h"
#include "cozmoAnim/animation/proceduralFace.h"
//#include "anki/cozmo/basestation/animations/animationTransfer.h"
#include "cozmoAnim/cozmoAnimContext.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/dispatchWorker/dispatchWorker.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"
#include <json/json.h>
#include <string>
#include <sys/stat.h>

namespace {
  
}

namespace Anki {
namespace Cozmo {

RobotDataLoader::RobotDataLoader(const CozmoAnimContext* context)
: _context(context)
, _platform(_context->GetDataPlatform())
, _cannedAnimations(new CannedAnimationContainer())
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

void RobotDataLoader::LoadConfigData()
{
  // Text-to-speech config
  {
    static const std::string & tts_config = "config/engine/tts_config.json";
    const bool success = _platform->readAsJson(Util::Data::Scope::Resources, tts_config, _tts_config);
    if (!success)
    {
      PRINT_NAMED_ERROR("RobotDataLoader.TextToSpeechConfigNotFound",
                        "Text-to-speech config file %s not found or failed to parse",
                        tts_config.c_str());
    }
  }
}

void RobotDataLoader::LoadNonConfigData()
{
  if (_platform == nullptr) {
    return;
  }
  
  {
    ANKI_CPU_PROFILE("RobotDataLoader::CollectFiles");
    CollectAnimFiles();
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadAnimations");
    LoadAnimationsInternal();
    // The threaded animation loading workers each add to the loading ratio
  }

  {
    ANKI_CPU_PROFILE("RobotDataLoader::LoadFaceAnimations");
    LoadFaceAnimations();
    AddToLoadingRatio(_kFaceAnimationsLoadingRatio);
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
  
void RobotDataLoader::CollectAnimFiles()
{
  // animations
  {
    const std::vector<std::string> paths = {"assets/animations/", "config/engine/animations/"};
    for (const auto& path : paths) {
      WalkAnimationDir(path, _animFileTimestamps, [this] (const std::string& filename) {
        _jsonFiles[FileType::Animation].push_back(filename);
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

void RobotDataLoader::LoadAnimations()
{
  CollectAnimFiles();
  LoadAnimationsInternal();
  _jsonFiles.clear();
}

void RobotDataLoader::LoadAnimationsInternal()
{
  const double startTime = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();

  // Disable super-verbose warnings about clipping face parameters in json files
  // To help find bad/deprecated animations, try removing this.
  ProceduralFace::EnableClippingWarning(false);
  
  using MyDispatchWorker = Util::DispatchWorker<3, const std::string&>;
  MyDispatchWorker::FunctionType loadFileFunc = std::bind(&RobotDataLoader::LoadAnimationFile, this, std::placeholders::_1);
  MyDispatchWorker myWorker(loadFileFunc);

  const auto& fileList = _jsonFiles[FileType::Animation];
  unsigned long size = fileList.size();
  for (int i = 0; i < size; i++) {
    myWorker.PushJob(fileList[i]);
    //PRINT_NAMED_DEBUG("RobotDataLoader.LoadAnimations", "loaded regular anim %d of %zu", i, size);
  }
  
  _perAnimationLoadingRatio = _kAnimationsLoadingRatio * 1.0f / Util::numeric_cast<float>(size);
  myWorker.Process();

  ProceduralFace::EnableClippingWarning(true);

  const double endTime = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
  double loadTime = endTime - startTime;
  PRINT_CH_INFO("Animations", "RobotDataLoader.LoadAnimationsInternal.LoadTime",
                "Time to load animations = %.2f ms", loadTime);

  auto animNames = _cannedAnimations->GetAnimationNames();
  PRINT_CH_INFO("Animations", "RobotDataLoader.LoadAnimations.CannedAnimationsCount",
                "Total number of canned animations available = %lu", (unsigned long) animNames.size());
}


void RobotDataLoader::WalkAnimationDir(const std::string& animationDir, TimestampMap& timestamps, const std::function<void(const std::string&)>& walkFunc)
{
  const std::string animationFolder = _platform->pathToResource(Util::Data::Scope::Resources, animationDir);
  const std::vector<const char*> fileExts = {"json", "bin"};
  auto filePaths = Util::FileUtils::FilesInDirectory(animationFolder, true, fileExts, true);

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

  ANKI_VERIFY( !_context->IsMainThread(), "RobotDataLoader.AnimFileOnMainThread", "" );

  //PRINT_CH_DEBUG("Animations", "RobotDataLoader.LoadAnimationFile.LoadingAnimationsFromBinaryOrJson",
  //               "Loading animations from %s", path.c_str());

  const bool binFile = Util::FileUtils::FilenameHasSuffix(path.c_str(), "bin");

  if (binFile) {

    // Read the binary file
    auto binFileContents = Util::FileUtils::ReadFileAsBinary(path);
    if (binFileContents.size() == 0) {
      PRINT_NAMED_ERROR("RobotDataLoader.LoadAnimationFile.BinaryDataEmpty", "Found no data in %s", path.c_str());
      return;
    }
    unsigned char *binData = binFileContents.data();
    if (nullptr == binData) {
      PRINT_NAMED_ERROR("RobotDataLoader.LoadAnimationFile.BinaryDataNull", "Found no data in %s", path.c_str());
      return;
    }
    auto animClips = CozmoAnim::GetAnimClips(binData);
    if (nullptr == animClips) {
      PRINT_NAMED_ERROR("RobotDataLoader.LoadAnimationFile.AnimClipsNull", "Found no animations in %s", path.c_str());
      return;
    }
    auto allClips = animClips->clips();
    if (nullptr == allClips) {
      PRINT_NAMED_ERROR("RobotDataLoader.LoadAnimationFile.AllClipsNull", "Found no animations in %s", path.c_str());
      return;
    }
    if (allClips->size() == 0) {
      PRINT_NAMED_ERROR("RobotDataLoader.LoadAnimationFile.AnimClipsEmpty", "Found no animations in %s", path.c_str());
      return;
    }

    for (int clipIdx=0; clipIdx < allClips->size(); clipIdx++) {
      auto animClip = allClips->Get(clipIdx);
      auto animName = animClip->Name()->c_str();
      //PRINT_CH_DEBUG("Animations", "RobotDataLoader.LoadAnimationFile.LoadingSpecificAnimFromBinary",
      //              "Loading '%s' from %s", animName, path.c_str());
      std::string strName = animName;

      // TODO: Should this mutex lock happen here or immediately before this for loop (COZMO-8766)?
      std::lock_guard<std::mutex> guard(_parallelLoadingMutex);

      _cannedAnimations->DefineFromFlatBuf(animClip, strName);
    }

  } else {
    Json::Value animDefs;
    // add json filename and callback (to perform load) here?
    const bool success = _platform->readAsJson(path.c_str(), animDefs);
    std::string animationId;
    if (success && !animDefs.empty()) {
      std::lock_guard<std::mutex> guard(_parallelLoadingMutex);
      _cannedAnimations->DefineFromJson(animDefs, animationId);

      // TODO: This warning is useful, but it causes a crash when we use the current mechanism for
      //       animators to preview their work in Maya on the robot. We plan on changing that
      //       preview-on-robot to use the SDK, so this warning should be tested and potentially
      //       enabled after that. See COZMO-9251 for some related info (nishkar, 2/2/2017).
      //
      //if(path.find(animationId) == std::string::npos) {
      //  PRINT_NAMED_WARNING("RobotDataLoader.LoadAnimationFile.AnimationNameMismatch",
      //                      "Animation name '%s' does not seem to match filename '%s'",
      //                      animationId.c_str(), path.c_str());
      //}

    }
  }
  AddToLoadingRatio(_perAnimationLoadingRatio);
}
  

void RobotDataLoader::LoadFaceAnimations()
{
  FaceAnimationManager::getInstance()->ReadFaceAnimationDir(_platform);
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
