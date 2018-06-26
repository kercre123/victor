/**
 * File: BehaviorAttentionTransferIfNeeded.cpp
 *
 * Author: Brad
 * Created: 2018-06-19
 *
 * Description: This behavior will perform an attention transfer (aka "look at phone") animation and send the
 *              corresponding message to the app if needed. It can be configured to require an event happen X
 *              times in Y seconds to trigger the transfer, and if that hasn't been met it can either do a
 *              fallback animation or nothing.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/attentionTransfer/behaviorAttentionTransferIfNeeded.h"

#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/attentionTransferComponent.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kReasonKey = "attentionTransferReason";
static const char* kAnimIfNotRecentKey = "animIfNotRecent";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAttentionTransferIfNeeded::BehaviorAttentionTransferIfNeeded(const Json::Value& config)
 : ICozmoBehavior(config)
{  
  JsonTools::GetCladEnumFromJSON(config, kReasonKey, _iConfig.reason, GetDebugLabel());
  ANKI_VERIFY( RecentOccurrenceTracker::ParseConfig(config, _iConfig.numberOfTimes, _iConfig.amountOfSeconds),
               "BehaviorAttentionTransferIfNeeded.Constructor.InvalidConfig",
               "Behavior '%s' specified invalid recent occurrence config",
               GetDebugLabel().c_str() );

  const bool shouldAssert = false;
  JsonTools::GetCladEnumFromJSON(config, kAnimIfNotRecentKey, _iConfig.animIfNotRecent, GetDebugLabel(), shouldAssert);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAttentionTransferIfNeeded::~BehaviorAttentionTransferIfNeeded()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAttentionTransferIfNeeded::InitBehavior()
{
  if( ANKI_VERIFY( _iConfig.reason != AttentionTransferReason::Invalid,
                   "BehaviorAttentionTransferIfNeeded.Init.NoValidReason",
                   "Behavior '%s' does not have a valid attention transfer reason",
                   GetDebugLabel().c_str()) ) {
    _iConfig.recentOccurrenceHandle = GetBehaviorComp<AttentionTransferComponent>().GetRecentOccurrenceHandle(
      _iConfig.reason,
      _iConfig.numberOfTimes,
      _iConfig.amountOfSeconds);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAttentionTransferIfNeeded::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kReasonKey,
    kAnimIfNotRecentKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
  RecentOccurrenceTracker::GetConfigJsonKeys(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAttentionTransferIfNeeded::OnBehaviorActivated() 
{  
  GetBehaviorComp<AttentionTransferComponent>().PossibleAttentionTransferNeeded( _iConfig.reason );

  if( _iConfig.recentOccurrenceHandle->AreConditionsMet() ) {
    TransitionToAttentionTransfer();
  }
  else {
    TransitionToNoAttentionTransfer();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAttentionTransferIfNeeded::TransitionToAttentionTransfer()
{
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::LookAtDeviceGetIn), [this]() {
      GetBehaviorComp<AttentionTransferComponent>().OnAttentionTransferred( _iConfig.reason );

      CompoundActionSequential* action = new CompoundActionSequential({
            new TriggerLiftSafeAnimationAction(AnimationTrigger::LookAtDevice),
            new TriggerLiftSafeAnimationAction(AnimationTrigger::LookAtDeviceGetOut)});
      DelegateIfInControl(action);
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAttentionTransferIfNeeded::TransitionToNoAttentionTransfer()
{
  if( _iConfig.animIfNotRecent != AnimationTrigger::Count ) {
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(_iConfig.animIfNotRecent));
  }
}

}
}
