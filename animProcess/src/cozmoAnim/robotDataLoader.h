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

#ifndef ANKI_COZMO_ANIM_DATA_LOADER_H
#define ANKI_COZMO_ANIM_DATA_LOADER_H

#include "util/helpers/noncopyable.h"
#include <json/json.h>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Anki {

namespace Util {
namespace Data {
class DataPlatform;
}
}

namespace Cozmo {

class CannedAnimationContainer;
class CozmoContext;

class RobotDataLoader : private Util::noncopyable
{
public:
  //RobotDataLoader(const CozmoContext* context);
  RobotDataLoader(const Util::Data::DataPlatform* dataPlatform);
  ~RobotDataLoader();

  // loads all data excluding configs, using DispatchWorker to parallelize.
  // Blocks until the data is loaded.
  void LoadNonConfigData();
  
  // Starts a thread to handle loading non-config data if it hasn't been done yet.
  // Can be repeatedly called to get an updated loading complete ratio. Returns
  // false if loading is ongoing, otherwise returns true
  bool DoNonConfigDataLoading(float& loadingCompleteRatio_out);

  // refresh individual data pieces after initial load
  void LoadAnimations();
  void LoadFaceAnimations();


  using FileJsonMap       = std::unordered_map<std::string, const Json::Value>;
  
  CannedAnimationContainer* GetCannedAnimations() const { return _cannedAnimations.get(); }

  bool IsCustomAnimLoadEnabled() const;
  
private:
  void CollectAnimFiles();
  
  void LoadAnimationsInternal();
  void LoadAnimationFile(const std::string& path);
  
  void AddToLoadingRatio(float delta);

  using TimestampMap = std::unordered_map<std::string, time_t>;
  void WalkAnimationDir(const std::string& animationDir, TimestampMap& timestamps,
                        const std::function<void(const std::string& filePath)>& walkFunc);


//  const CozmoContext* const _context;
  const Util::Data::DataPlatform* _platform;

  enum FileType {
      Animation,
  };
  std::unordered_map<int, std::vector<std::string>> _jsonFiles;

  // animation data
  std::unique_ptr<CannedAnimationContainer>           _cannedAnimations;
  TimestampMap _animFileTimestamps;

  std::string _test_anim;

  
  bool                  _isNonConfigDataLoaded = false;
  std::mutex            _parallelLoadingMutex;
  std::atomic<float>    _loadingCompleteRatio{0};
  std::thread           _dataLoadingThread;
  std::atomic<bool>     _abortLoad{false};
  
  // This gets set when we start loading animations and know the total number
  float _perAnimationLoadingRatio = 0.0f;
};

}
}

#endif
