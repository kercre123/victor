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


#include "cannedAnimLib/cannedAnims/animation.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationLoader.h"
#include "cannedAnimLib/baseTypes/cozmo_anim_generated.h"
#include "cannedAnimLib/spriteSequences/spriteSequenceContainer.h"
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
namespace Cozmo {

RobotDataLoader::RobotDataLoader(const AnimContext* context)
: _context(context)
, _platform(_context->GetDataPlatform())
, _cannedAnimations(nullptr)
{
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
}

void RobotDataLoader::LoadNonConfigData()
{
  if (_platform == nullptr) {
    return;
  }
  
  _spriteSequenceContainer = std::make_unique<SpriteSequenceContainer>();
  _spriteSequenceContainer->ReadSpriteSequenceDir(_platform);
  
  CannedAnimationLoader animLoader(_platform, _spriteSequenceContainer.get(), 
                                   _loadingCompleteRatio, _abortLoad);
  _cannedAnimations.reset(animLoader.LoadAnimations());
  
  SetupProceduralAnimation();  
}

void RobotDataLoader::LoadAnimationFile(const std::string& path)
{
  if (_platform == nullptr) {
    return;
  }
  CannedAnimationLoader animLoader(_platform, _spriteSequenceContainer.get(),
                                   _loadingCompleteRatio, _abortLoad);
  const auto& animsContainer = animLoader.LoadAnimationsFromFile(path);
  for (const auto& name : animsContainer->GetAnimationNames())
  {
    auto* anim = animsContainer->GetAnimation(name);
    _cannedAnimations->AddAnimation(anim);

    NotifyAnimAdded(name, anim->GetLastKeyFrameEndTime_ms());
  }
  animsContainer->Clear();
  delete animsContainer;
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
  _cannedAnimations->AddAnimation(SpriteSequenceContainer::ProceduralAnimName);
  
  Animation* anim = _cannedAnimations->GetAnimation(SpriteSequenceContainer::ProceduralAnimName);
  assert(anim != nullptr);
  SpriteSequenceKeyFrame kf(SpriteSequenceContainer::ProceduralAnimName);
  kf.SetSpriteSequenceContainer(_spriteSequenceContainer.get());
  if(RESULT_OK != anim->AddKeyFrameToBack(kf))
  {
    PRINT_NAMED_ERROR("RobotDataLoader.SetupProceduralAnimation.AddProceduralFailed",
                      "Failed to add keyframe to procedural animation.");
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
