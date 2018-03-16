/**
* File: severeNeedsComponent.cpp
*
* Author: Kevin M. Karol
* Created: 08/23/17
*
* Description: Component which manages Cozmo's severe needs state
* and its associated idles/activation/etc.
*
* Copyright: Anki, Inc. 2017
*
**/
#include "engine/aiComponent/severeNeedsComponent.h"

#include "engine/ankiEventUtil.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {

namespace {

  
constexpr ReactionTriggerHelpers::FullReactionArray kTriggersToDisableInSevereNeedsState = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               true},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotFalling,                 true},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  true},
  {ReactionTrigger::RobotOnFace,                  true},
  {ReactionTrigger::RobotOnSide,                  true},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kTriggersToDisableInSevereNeedsState),
              "Reaction triggers duplicate or non-sequential");

constexpr ReactionTriggerHelpers::FullReactionArray kAffectNoneArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectNoneArray),
              "Reaction triggers duplicate or non-sequential");
  
std::map<NeedId, const ReactionTriggerHelpers::FullReactionArray*> kReactionsDisabledPerNeedMap = {
  {NeedId::Energy, &kTriggersToDisableInSevereNeedsState},
  {NeedId::Repair, &kTriggersToDisableInSevereNeedsState},
  {NeedId::Play,   &kAffectNoneArray}
};

const char* kSevereNeedStateLock = "severe_need_component_lock";

} // namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SevereNeedsComponent
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SevereNeedsComponent::SevereNeedsComponent(Robot& robot)
: _robot(robot)
, _severeNeedExpression(NeedId::Count)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SevereNeedsComponent::~SevereNeedsComponent()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SevereNeedsComponent::Init()
{
  // register to possible object events
  if ( _robot.HasExternalInterface() )
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeGameToEngine<MessageGameToEngineTag::NotifyCozmoWakeup>();
  }
  else {
    PRINT_NAMED_WARNING("SevereNeedsComponent.Init", "Initialized whiteboard with no external interface. Will miss events.");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SevereNeedsComponent::Update()
{
  if( HasSevereNeedExpression() ) {
    NeedsState& currNeedState = _robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
    // If needs are paused or the current severeNeedExpression is no longer critical, clear the severe expression
    if(!currNeedState.IsNeedAtBracket(_severeNeedExpression, NeedBracketId::Critical)){
      PRINT_CH_INFO("SevereNeedsComponent", "SevereNeedsState.AutoClear",
                    "Automatically clearing currently expressed severe needs state. Was '%s'",
                    NeedIdToString(_severeNeedExpression));
      ClearSevereNeedExpression();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void SevereNeedsComponent::HandleMessage(const ExternalInterface::NotifyCozmoWakeup& msg)
{
  // Setup Cozmo's current expressed need state - on Init need state will be handled
  // by game to play Cozmo's wakeup animation, so set the current expressed need
  // here manually
  NeedsState& currNeedState = _robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isRepairCritical =
  currNeedState.IsNeedAtBracket(NeedId::Repair, NeedBracketId::Critical);
  
  const bool isEnergyCritical =
  currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
  if(isRepairCritical){
    SetSevereNeedExpression(NeedId::Repair);
  }else if(isEnergyCritical){
    SetSevereNeedExpression(NeedId::Energy);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SevereNeedsComponent::SetSevereNeedExpression(NeedId need) {
  PRINT_CH_DEBUG("SevereNeedsComponent",
                 "SevereNeedsComponent.SevereNeedExpression.Set",
                 "Set to %s (was %s)",
                 NeedIdToString(need),
                 NeedIdToString(_severeNeedExpression));

  // shouldn't already be expressing a severe need, except in the case of Play, because there may not have
  // been a get-out from play yet (e.g. play goes severe first, then hunger)
  DEV_ASSERT(_severeNeedExpression == NeedId::Count || _severeNeedExpression == NeedId::Play,
             "SevereNeedsComponent.SetSevereNeedExpression.ExpressionAlreadySet");
  
  //////
  // Setup driving and idle animation maps based on need
  //////
  static const std::map<NeedId, AnimationTrigger> idleAnimation =
     {{NeedId::Energy,AnimationTrigger::NeedsSevereLowEnergyIdle},
      {NeedId::Repair,AnimationTrigger::NeedsSevereLowRepairIdle}};
  
  
  struct DrivingAnimContainer{
    DrivingAnimContainer(AnimationTrigger getIn, AnimationTrigger loop, AnimationTrigger getOut)
    : _getIn(getIn)
    , _loop(loop)
    , _getOut(getOut){};
    
    AnimationTrigger _getIn;
    AnimationTrigger _loop;
    AnimationTrigger _getOut;
  };
  
  static const DrivingAnimContainer energyDriving(
                    AnimationTrigger::NeedsSevereLowEnergyDrivingStart,
                    AnimationTrigger::NeedsSevereLowEnergyDrivingLoop,
                    AnimationTrigger::NeedsSevereLowEnergyDrivingEnd);

  static const DrivingAnimContainer repairDriving(
                    AnimationTrigger::NeedsSevereLowRepairDrivingStart,
                    AnimationTrigger::NeedsSevereLowRepairDrivingLoop,
                    AnimationTrigger::NeedsSevereLowRepairDrivingEnd);
  
  const std::map<NeedId, DrivingAnimContainer> drivingAnimMap =
    {{NeedId::Energy,energyDriving},{NeedId::Repair,repairDriving}};
  
  
  const auto& drivingIter = drivingAnimMap.find(need);
  if(drivingIter != drivingAnimMap.end()){
    _robot.GetDrivingAnimationHandler().PushDrivingAnimations({
      drivingIter->second._getIn,
      drivingIter->second._loop,
      drivingIter->second._getOut},
      kSevereNeedStateLock);
  }
  
  const auto& idleIter = idleAnimation.find(need);
  if(idleIter != idleAnimation.end()){
    _robot.GetAnimationStreamer().PushIdleAnimation(idleIter->second,
                                                    kSevereNeedStateLock);
  }
  
  _robot.GetBehaviorManager().DisableReactionsWithLock(kSevereNeedStateLock, *(kReactionsDisabledPerNeedMap[need]));
  
  _severeNeedExpression = need;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SevereNeedsComponent::ClearSevereNeedExpression() {
  PRINT_CH_DEBUG("SevereNeedsComponent",
                 "SevereNeedsComponent.SevereNeedExpression.Clear",
                 "Cleared. (was %s)",
                 NeedIdToString(_severeNeedExpression));
  
  DEV_ASSERT(_severeNeedExpression != NeedId::Count,
             "SevereNeedsComponent.ClearSevereNeedExpression.ExpressionNotSet");
  
  _robot.GetDrivingAnimationHandler().RemoveDrivingAnimations(kSevereNeedStateLock);
  _robot.GetAnimationStreamer().RemoveIdleAnimation(kSevereNeedStateLock);

  _robot.GetBehaviorManager().RemoveDisableReactionsLock(kSevereNeedStateLock);
  
  _severeNeedExpression = NeedId::Count;
}


} // namespace Cozmo
} // namespace Anki
