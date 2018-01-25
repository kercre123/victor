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


#include "cannedAnimLib/cannedAnimationContainer.h"
#include "cannedAnimLib/cannedAnimationLoader.h"
#include "cannedAnimLib/cozmo_anim_generated.h"
#include "cannedAnimLib/faceAnimationManager.h"
#include "cannedAnimLib/proceduralFace.h"
//#include "anki/cozmo/basestation/animations/animationTransfer.h"
#include "cozmoAnim/cozmoAnimContext.h"

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
}

void RobotDataLoader::LoadNonConfigData()
{
  if (_platform == nullptr) {
    return;
  }
  CannedAnimationLoader animLoader(_platform, _loadingCompleteRatio, _abortLoad);
  _cannedAnimations.reset(animLoader.LoadAnimations());
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
