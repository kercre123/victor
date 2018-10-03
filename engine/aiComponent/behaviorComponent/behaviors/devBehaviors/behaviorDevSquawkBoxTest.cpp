/**
 * File: BehaviorDevSquawkBoxTest.cpp
 *
 * Author: Jordan Rivas
 * Created: 2018-09-28
 *
 * Description: Behavior to test scenarios while the robot is in the squawk box.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevSquawkBoxTest.h"

#include "audioEngine/multiplexer/audioCladMessageHelper.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/actions/basicActions.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"
#include "util/console/consoleFunction.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Vector {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
#define CONSOLE_NAME "DevSquawkBoxBehavior"
  CONSOLE_VAR_RANGED(float, kLiftMovementDuration_s, CONSOLE_NAME, 0.5f, 0.1f, 2.5f);
  CONSOLE_VAR_RANGED(float, kHeadMovementDuration_s, CONSOLE_NAME, 0.5f, 0.1f, 2.5f);
  CONSOLE_VAR_RANGED(float, kTreadMovementSpeed_mmps, CONSOLE_NAME, 200.0f, 20.0f, MAX_WHEEL_SPEED_MMPS);
}

void BehaviorDevSquawkBoxTest::SetupConsoleFuncs()
{
  if( ANKI_DEV_CHEATS ) {
    auto toggleLiftOnOff = [this](ConsoleFunctionContextRef context) {
      _dVars._isLiftMoving = !_dVars._isLiftMoving;
      MoveLift();
      const auto* msg = _dVars._isLiftMoving ? "Start Lift Movement" : "Stop Lift Movment";
      context->channel->WriteLog(" %s", msg);
    };
    auto toggleHeadOnOff = [this](ConsoleFunctionContextRef context) {
      _dVars._isHeadMoving = !_dVars._isHeadMoving;
      MoveHead();
      const auto* msg = _dVars._isHeadMoving ? "Start Head Movement" : "Stop Head Movment";
      context->channel->WriteLog(" %s", msg);
    };
    auto toggleTreadOnOff = [this](ConsoleFunctionContextRef context) {
      _dVars._isMovingTreads = !_dVars._isMovingTreads;
      MoveTreads();
      const auto* msg = _dVars._isMovingTreads ? "Start Tread Movement" : "Stop Tread Movment";
      context->channel->WriteLog(" %s", msg);
    };
    _consoleFuncs.emplace_front( "ToggleLiftOnOff", std::move(toggleLiftOnOff), CONSOLE_NAME, "" );
    _consoleFuncs.emplace_front( "ToggleHeadOnOff", std::move(toggleHeadOnOff), CONSOLE_NAME, "" );
    _consoleFuncs.emplace_front( "ToggleTreadsOnOff", std::move(toggleTreadOnOff), CONSOLE_NAME, "" );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::InitBehavior()
{
  if( ANKI_DEV_CHEATS ) {
    // Engine -> Game
    SubscribeToTags({
      EngineToGameTag::RobotCompletedAction
    });
    // Robot -> Engine
    SubscribeToTags({
      RobotInterface::RobotToEngineTag::triggerWordDetected
    });
    _robot = &GetBEI().GetRobotInfo()._robot;
    SetupConsoleFuncs();
  }
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevSquawkBoxTest::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevSquawkBoxTest::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevSquawkBoxTest::BehaviorDevSquawkBoxTest(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // NOTE: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevSquawkBoxTest::~BehaviorDevSquawkBoxTest()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates         = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  // const char* list[] = {
  //   // NOTE: insert any possible root-level json keys that this class is expecting.
  //   // NOTE: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
  // };
  // expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::HandleWhileActivated(const EngineToGameEvent& event)
{
  if (event.GetData().GetTag() == EngineToGameTag::RobotCompletedAction) {
    const auto& msg = event.GetData().Get_RobotCompletedAction();
    if (_dVars._isLiftMoving && (msg.idTag == _dVars._liftActionTag)) {
      MoveLift();
    }
    else if (_dVars._isHeadMoving && (msg.idTag == _dVars._headActionTag)) {
      MoveHead();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::HandleWhileActivated(const RobotToEngineEvent& event)
{
  if (event.GetData().GetTag() == RobotInterface::RobotToEngineTag::triggerWordDetected) {
    const auto& msg = event.GetData().Get_triggerWordDetected();
    PRINT_NAMED_INFO("nameBehaviorDevSquawkBoxTest.HandleWhileActivated.RobotToEngineEvent",
                     "Trigger word detected, score %d", msg.triggerScore);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevSquawkBoxTest::WantsToBeActivatedBehavior() const
{
  return (ANKI_DEV_CHEATS != 0);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
  const auto earConBegin = AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On;
  auto postAudioEvent = AECH::CreatePostAudioEvent( earConBegin, AudioMetaData::GameObjectType::Behavior, 0 );
  SmartPushResponseToTriggerWord(AnimationTrigger::Count,
                                 postAudioEvent,
                                 StreamAndLightEffect::StreamingDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::OnBehaviorDeactivated()
{
  // Be sure to stop treads
  _dVars._isMovingTreads = false;
  MoveTreads();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::BehaviorUpdate() 
{
  if( IsActivated() ) {
    // Update tread speed with console var
    if (_dVars._isMovingTreads && (kTreadMovementSpeed_mmps != _dVars._currentTreadSpeed)) {
      MoveTreads();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::MoveLift()
{
  if (_robot == nullptr) {
    LOG_WARNING("BehaviorDevSquawkBoxTest.MoveLift", "Behavior had not been instatiated");
    return;
  }
  
  auto* liftUpAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::HIGH_DOCK);
  liftUpAction->SetDuration(kLiftMovementDuration_s);
  
  auto* liftDownAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
  liftDownAction->SetDuration(kLiftMovementDuration_s);
  
  auto* compoundSeq = new CompoundActionSequential( {liftUpAction, liftDownAction} );
  _dVars._liftActionTag = compoundSeq->GetTag();
  _robot->GetActionList().AddConcurrentAction( compoundSeq );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::MoveHead()
{
  if (_robot == nullptr) {
    LOG_WARNING("BehaviorDevSquawkBoxTest.MoveHead", "Behavior had not been instatiated");
    return;
  }
  
  auto* headUpAction = new MoveHeadToAngleAction(DEG_TO_RAD(42), DEG_TO_RAD(10.0f));
  headUpAction->SetDuration(kHeadMovementDuration_s);

  auto* headDownAction = new MoveHeadToAngleAction(DEG_TO_RAD(-20), DEG_TO_RAD(10.0f));
  headDownAction->SetDuration(kHeadMovementDuration_s);
  
  auto* compoundSeq = new CompoundActionSequential( {headUpAction, headDownAction} );
  _dVars._headActionTag = compoundSeq->GetTag();
  _robot->GetActionList().AddConcurrentAction( compoundSeq );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevSquawkBoxTest::MoveTreads()
{
  if (_robot == nullptr) {
    LOG_WARNING("BehaviorDevSquawkBoxTest.MoveTreads", "Behavior had not been instatiated");
    return;
  }
  
  _dVars._currentTreadSpeed = _dVars._isMovingTreads ? kTreadMovementSpeed_mmps : 0;
  auto& moveComp = _robot->GetMoveComponent();
  moveComp.DriveWheels(_dVars._currentTreadSpeed, _dVars._currentTreadSpeed, 0.f, 0.f);
}
  

}
}
