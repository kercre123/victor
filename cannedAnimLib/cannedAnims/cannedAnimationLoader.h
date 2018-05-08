/**
 * File: cannedAnimationLoader.h
 *
 * Authors: Kevin M. Karol
 * Created: 1/18/18
 *
 * Description:
 *    Class that loads animations from data on worker threads and
 *    returns the final animation container
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef ANKI_COZMO_CANNED_ANIMATION_LOADER_H
#define ANKI_COZMO_CANNED_ANIMATION_LOADER_H

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "coretech/vision/shared/spritePathMap.h"
#include "util/helpers/noncopyable.h"
#include <atomic>
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

namespace Vision{
class SpriteSequenceContainer;
}

namespace Cozmo {

// forward declaration
class Animation;
class AnimContext;

class CannedAnimationLoader : private Util::noncopyable
{
public:
  struct AnimDirInfo{
    using TimestampMap = std::unordered_map<std::string, time_t>;

    TimestampMap animFileTimestamps;
    std::vector<std::string> jsonFiles;
  };

  CannedAnimationLoader(const Util::Data::DataPlatform* platform,
                        const Vision::SpritePathMap* spriteMap,
                        Vision::SpriteSequenceContainer* spriteSequenceContainer,
                        std::atomic<float>& loadingCompleteRatio,
                        std::atomic<bool>&  abortLoad)
  : _platform(platform)
  , _spriteMap(spriteMap)
  , _spriteSequenceContainer(spriteSequenceContainer)
  , _loadingCompleteRatio(loadingCompleteRatio)
  , _abortLoad(abortLoad){}

  void LoadAnimationsIntoContainer(const AnimDirInfo& info, CannedAnimationContainer* container);
  void LoadAnimationIntoContainer(const std::string& path, CannedAnimationContainer* container);

  static AnimDirInfo CollectAnimFiles(const Util::Data::DataPlatform* platform, const std::vector<std::string>& paths);

private:
  
  // params passed in by data loader class
  const Util::Data::DataPlatform* _platform;
  const Vision::SpritePathMap* _spriteMap;
  Vision::SpriteSequenceContainer* _spriteSequenceContainer = nullptr;
  std::atomic<float>& _loadingCompleteRatio;
  std::atomic<bool>&  _abortLoad;
  
  // This gets set when we start loading animations and know the total number
  float _perAnimationLoadingRatio = 0.0f;
  std::mutex _parallelLoadingMutex;

  // pointer to the container passed in on Load calls for reference by member functions
  CannedAnimationContainer* _containerPtr = nullptr;

  static void WalkAnimationDir(const Util::Data::DataPlatform* platform,
                               const std::string& animationDir, AnimDirInfo::TimestampMap& timestamps,
                               const std::function<void(const std::string& filePath)>& walkFunc);

  void LoadAnimationsInternal(const AnimDirInfo& info);
  
  void AddToLoadingRatio(float delta);

  void LoadAnimationFile(const std::string& path);

  Result DefineFromJson(const Json::Value& jsonRoot, std::string& loadedAnimName);
  Result DefineFromFlatBuf(const CozmoAnim::AnimClip* animClip, std::string& animName);
  void SetSpriteSequenceContainerOnAnimation(Animation& animation) const;
  
  Result SanityCheck(Result lastResult, Animation& animation, std::string& animationName) const;

};

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_LOADER_H
