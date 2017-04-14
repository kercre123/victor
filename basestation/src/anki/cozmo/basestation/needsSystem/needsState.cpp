/**
 * File: needsState
 *
 * Author: Paul Terry
 * Created: 04/12/2017
 *
 * Description: State data for Cozmo's Needs system
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {


//CONSOLE_VAR(bool, kSendMoodToViz, "VizDebug", true);

  
NeedsState::NeedsState()
: _curNeedLevel()
, _curNeedsBrackets()
, _partIsDamaged()
, _curNeedsUnlockLevel(0)
, _numStarsAwarded(0)
, _numStarsForNextUnlock(1)
{
}
  
NeedsState::~NeedsState()
{
  Reset();
}

void NeedsState::Init(const NeedsConfig& needsConfig)
{
  Reset();
  
  _curNeedLevel[NeedId::Repair] = 100.0f; // todo make these constants come from config data
  _curNeedLevel[NeedId::Energy] = 100.0f;
  _curNeedLevel[NeedId::Play]   = 100.0f;
  
  SetCurNeedsBrackets(needsConfig);
  
  _partIsDamaged[RepairablePartId::Head]   = false;
  _partIsDamaged[RepairablePartId::Lift]   = false;
  _partIsDamaged[RepairablePartId::Treads] = false;

  _curNeedsUnlockLevel = 0;
  _numStarsAwarded = 0;
  _numStarsForNextUnlock = 3; // todo set this from config data
}
  
void NeedsState::Reset()
{
  _curNeedLevel.clear();
  _curNeedsBrackets.clear();
  _partIsDamaged.clear();
}

void NeedsState::Decay(float timeElasped_secs)
{
  // todo:
}
  
void NeedsState::DecayUnconnected(float timeElasped_secs)
{
  // todo:
}

void NeedsState::SetCurNeedsBrackets(const NeedsConfig& needsConfig)
{
  // todo: set each of the needs' 'current bracket' based on the current level for that need,
  // and configuration data.
  // To be called whenever needs level changes.
}

//  Nathan Monson This question about managing flash comes up from time to time.
//  _Avoid burn-out:_
//  Only write flash while gameplay is happening (never while on the charger/etc).
//    Only write flash when something happens (unlock, etc).  Even twice a minute (on average) is perfectly fine.
//    If you're writing something every single second of gameplay or on the charger, you should go back and fix it.
//    (Obvious) Avoid writing data that hasn't changed.
//    _Avoid stuttering:_
//    Writing can sometimes be very slow - over a second even for small amounts.
//      So, only write flash when the robot isn't doing anything.
//      *Do NOT write periodically - you may interrupt something the robot is doing.*
//      Be synchronous (just after objective completed), not asynchronous (every minute). (edited)
  
} // namespace Cozmo
} // namespace Anki

