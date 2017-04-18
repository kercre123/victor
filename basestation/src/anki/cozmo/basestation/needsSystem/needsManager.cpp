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
  // todo:  process the config data passed in to this function, to fill out the NeedsConfig
  // xxx
  
  _needsState.Init(_needsConfig);
  
  // todo:  Init various member vars of NeedsManager as needed
  
  // Subscribe to messages
  if (_robot != nullptr && _robot->HasExternalInterface() )
  {
    auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RegisterNeedsActionCompleted>();
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
  //SendNeedsStateToGame();
  
}


void NeedsManager::SetPaused(bool paused)
{
  if (paused == _isPaused) {
    DEV_ASSERT_MSG(paused != _isPaused, "NeedsManager.SetPaused.Redundant",
                   "Setting paused to %s but already in that state",
                   paused ? "true" : "false");
    return;
  }

  _isPaused = paused;

  // todo:  If pausing, record current time in _timeLastPaused?
  // todo:  If unpausing, ?
  
  SendNeedsPauseStateToGame();
}


template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetNeedsState& msg)
{
  SendNeedsStateToGame();
}
  
template<>
void NeedsManager::HandleMessage(const ExternalInterface::SetNeedsPauseState& msg)
{
  SetPaused(msg.pause);
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetNeedsPauseState& msg)
{
  SendNeedsPauseStateToGame();
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::RegisterNeedsActionCompleted& msg)
{
  // todo implement
}

void NeedsManager::SendNeedsStateToGame()
{
  _needsState.SetCurNeedsBrackets(_needsConfig);
  
  std::vector<float> needLevels;
  needLevels.reserve((size_t)NeedId::Count);
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    float level = _needsState.GetNeedLevelByIndex(i);
    needLevels.push_back(level);
  }
  
  std::vector<NeedBracketId> needBrackets;
  needBrackets.reserve((size_t)NeedBracketId::Count);
  for (size_t i = 0; i < (size_t)NeedBracketId::Count; i++)
  {
    NeedBracketId bracketId = _needsState.GetNeedBracketByIndex(i);
    needBrackets.push_back(bracketId);
  }
  
  std::vector<bool> partIsDamaged;
  partIsDamaged.reserve((size_t)RepairablePartId::Count);
  for (size_t i = 0; i < (size_t)RepairablePartId::Count; i++)
  {
    bool isDamaged = _needsState.GetPartIsDamagedByIndex(i);
    partIsDamaged.push_back(isDamaged);
  }
  
  ExternalInterface::NeedsState message(std::move(needLevels), std::move(needBrackets),
                                        std::move(partIsDamaged), _needsState._curNeedsUnlockLevel,
                                        _needsState._numStarsAwarded, _needsState._numStarsForNextUnlock);
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

void NeedsManager::SendNeedsPauseStateToGame()
{
  ExternalInterface::NeedsPauseState message(_isPaused);
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

void NeedsManager::SendAllNeedsMetToGame()
{
  // todo
//  message AllNeedsMet {
//    int_32 unlockLevelCompleted,
//    int_32 starsRequiredForNextUnlock,
//    // TODO:  List of rewards unlocked
//  }
  std::vector<NeedsReward> rewards;
  
  ExternalInterface::AllNeedsMet message(0, 1, std::move(rewards)); // todo fill in
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}


} // namespace Cozmo
} // namespace Anki

