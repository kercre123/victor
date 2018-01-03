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

#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {
  
namespace{
const char* kSevereNeedStateLock = "severe_need_component_lock";
  
}

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
    // TODO: Restore idle animations (VIC-366)
    //_robot.GetAnimationStreamer().PushIdleAnimation(idleIter->second, kSevereNeedStateLock);
  }
    
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
  // TODO: Restore idle animations (VIC-366)
  //_robot.GetAnimationStreamer().RemoveIdleAnimation(kSevereNeedStateLock);
  
  _severeNeedExpression = NeedId::Count;
}


} // namespace Cozmo
} // namespace Anki
