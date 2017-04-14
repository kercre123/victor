/**
 * File: needsManager
 *
 * Author: Paul Terry
 * Created: 04/12/2017
 *
 * Description: Manages the Needs for a Cozmo Robot
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {


//CONSOLE_VAR(bool, kSendMoodToViz, "VizDebug", true);



NeedsManager::NeedsManager(Robot* inRobot)
: _robot(inRobot)
, _lastUpdateTime(0.0f)
, _needsState()
, _needsConfig()
, _isPaused(false)
, _timeLastPaused(0.0f)
{
}


void NeedsManager::Init(const Json::Value& inJson)
{
  // todo:  process the config data pass in to this function, to fill out the NeedsConfig
  // xxx
  
  _needsState.Init(_needsConfig);
  
  // todo:  Init various member vars of NeedsManager as needed
  
  // todo:  Subscribe to messages
  if (_robot != nullptr && _robot->HasExternalInterface() )
  {
    //auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    //helper.SubscribeGameToEngine<MessageGameToEngineTag::MoodMessage>();
//    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotCompletedAction>();
  }
}


void NeedsManager::Update(float currentTime_s)
{
//  if (!_robot->GetContext()->GetFeatureGate()->IsFeatureEnabled(FeatureType::NeedsSystem))
//    return;
  
  if (_isPaused)
    return;
  
  // todo:  calculated elapsed time.
  // todo:  Check to see if it's time to do the per-minute decay operation, and if so, reset that timer and call _needsStata.Decay(elapsedTime (60s))
  
}


void NeedsManager::SetPaused(bool paused)
{
  // todo:  send ETG::CurrentNeedsPauseState
  _isPaused = paused;
  // todo:  If pausing, record current time in _timeLastPaused?
  // todo:  If unpausing, ?
}



// Handle various message types
template<typename T>
void HandleMessage(const T& msg);


  
} // namespace Cozmo
} // namespace Anki

