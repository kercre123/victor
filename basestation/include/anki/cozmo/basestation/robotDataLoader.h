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

#ifndef ANKI_COZMO_BASESTATION_ROBOT_DATA_LOADER_H
#define ANKI_COZMO_BASESTATION_ROBOT_DATA_LOADER_H

#include "util/helpers/noncopyable.h"
#include <json/json.h>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Anki {

namespace Util {
namespace Data {
class DataPlatform;
}
}

namespace Cozmo {

class AnimationGroupContainer;
class CannedAnimationContainer;
class CozmoContext;
class AnimationTriggerResponsesContainer;

class RobotDataLoader : private Util::noncopyable
{
public:
  RobotDataLoader(const CozmoContext* context);
  ~RobotDataLoader();

  // loads all data
  void LoadData();

  // refresh individual data pieces after initial load
  void LoadAnimations();
  void LoadFaceAnimations();

  using FileJsonMap = std::unordered_map<std::string, const Json::Value>;
  const FileJsonMap& GetEmotionEventJsons() const { return _emotionEvents; }
  const FileJsonMap& GetBehaviorJsons() const { return _behaviors; }

  CannedAnimationContainer* GetCannedAnimations() const { return _cannedAnimations.get(); }
  AnimationGroupContainer* GetAnimationGroups() const { return _animationGroups.get(); }
  AnimationTriggerResponsesContainer* GetAnimationTriggerResponses() const { return _animationTriggerResponses.get(); }

  // robot configuration json files
  const Json::Value& GetRobotMoodConfig() const { return _robotMoodConfig; }
  const Json::Value& GetRobotBehaviorConfig() const { return _robotBehaviorConfig; }
  const Json::Value& GetRobotVisionConfig() const { return _robotVisionConfig; }

private:
  void CollectJsonFiles();
  void LoadAnimationsInternal();
  void LoadAnimationFile(const std::string& path);
  void LoadAnimationGroups();
  void LoadAnimationGroupFile(const std::string& path);
  void LoadAnimationTriggerResponses();
  void LoadRobotConfigs();

  using TimestampMap = std::unordered_map<std::string, time_t>;
  void WalkAnimationDir(const std::string& animationDir, TimestampMap& timestamps,
                        const std::function<void(const std::string& filePath)>& walkFunc);

  void LoadEmotionEvents();
  void LoadBehaviors();

  const CozmoContext* const _context;
  const Util::Data::DataPlatform* _platform;

  FileJsonMap _emotionEvents;
  FileJsonMap _behaviors;

  enum FileType {
      Animation
    , AnimationGroup
  };
  std::unordered_map<int, std::vector<std::string>> _jsonFiles;

  // animation data
  std::unique_ptr<CannedAnimationContainer> _cannedAnimations;
  std::unique_ptr<AnimationGroupContainer> _animationGroups;
  std::unique_ptr<AnimationTriggerResponsesContainer> _animationTriggerResponses;
  TimestampMap _animFileTimestamps;
  TimestampMap _groupAnimFileTimestamps;

  // robot configs
  Json::Value _robotMoodConfig;
  Json::Value _robotBehaviorConfig;
  Json::Value _robotVisionConfig;
  
  std::mutex  _animationAddMutex;
};

}
}

#endif
