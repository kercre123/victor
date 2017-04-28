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
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/needsSystem/needsConfig.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {


//CONSOLE_VAR(bool, kSendMoodToViz, "VizDebug", true);

  
NeedsState::NeedsState()
: _timeLastWritten(Time())
, _robotSerialNumber(0)
, _curNeedsLevels()
, _curNeedsBracketsCache()
, _partIsDamaged()
, _curNeedsUnlockLevel(0)
, _numStarsAwarded(0)
, _numStarsForNextUnlock(1)
, _needsConfig(nullptr)
{
}

NeedsState::~NeedsState()
{
  Reset();
}

void NeedsState::Init(NeedsConfig& needsConfig, u32 serialNumber)
{
  Reset();

  _timeLastWritten = Time();  // ('never')

  _needsConfig = &needsConfig;

  _robotSerialNumber = serialNumber;

  _curNeedsLevels[NeedId::Repair] = needsConfig._initialNeedsLevels[NeedId::Repair];
  _curNeedsLevels[NeedId::Energy] = needsConfig._initialNeedsLevels[NeedId::Energy];
  _curNeedsLevels[NeedId::Play]   = needsConfig._initialNeedsLevels[NeedId::Play];
  
  _curNeedsLevels[NeedId::Repair] = 1.0f; // todo make these constants come from config data
  _curNeedsLevels[NeedId::Energy] = 1.0f;
  _curNeedsLevels[NeedId::Play]   = 1.0f;
  UpdateCurNeedsBrackets();
  
  _partIsDamaged[RepairablePartId::Head]   = false;
  _partIsDamaged[RepairablePartId::Lift]   = false;
  _partIsDamaged[RepairablePartId::Treads] = false;

  _curNeedsUnlockLevel = 0;
  _numStarsAwarded = 0;
  _numStarsForNextUnlock = 3; // todo set this from config data
}

void NeedsState::Reset()
{
  _curNeedsLevels.clear();
  _curNeedsBracketsCache.clear();
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

void NeedsState::UpdateCurNeedsBrackets()
{
  // todo: set each of the needs' 'current bracket' based on the current level for that need,
  // and configuration data (_needsConfig)
  // To be called whenever needs level changes.
  
  // temp code:
  _curNeedsBracketsCache[NeedId::Repair] = NeedBracketId::Full;
  _curNeedsBracketsCache[NeedId::Energy] = NeedBracketId::Full;
  _curNeedsBracketsCache[NeedId::Play] = NeedBracketId::Full;
}




} // namespace Cozmo
} // namespace Anki

