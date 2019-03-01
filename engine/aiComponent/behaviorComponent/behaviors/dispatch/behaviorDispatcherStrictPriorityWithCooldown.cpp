/**
 * File: behaviorDispatcherStrictPriorityWithCooldown.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: Simple dispatcher similar to strict priority, but also supports cooldowns on behaviors
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherStrictPriorityWithCooldown.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {
  
namespace {
const char* kBehaviorsKey = "behaviors";
const char* kLinkScopeKey = "linkScope";
const char* kResetCooldownOnDeactivationKey = "resetCooldownOnDeactivation";
  
const char* kLinkedBehaviorTimerKey = "linkedBehaviorTimer";
const char* kResetBehaviorTimerOnDispatcherActivationKey = "resetLinkedTimerOnDispatcherActivation";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriorityWithCooldown::InstanceConfig::InstanceConfig()
  : linkScope(false)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriorityWithCooldown::DynamicVariables::DynamicVariables()
{
  lastDesiredBehaviorIdx = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcherStrictPriorityWithCooldown(const Json::Value& config)
  : BaseClass(config)
{

  _iConfig.linkScope = config.get(kLinkScopeKey, false).asBool();
  
  _iConfig.resetCooldownOnDeactivation = config.get(kResetCooldownOnDeactivationKey, false).asBool();
  
  const Json::Value& behaviorArray = config[kBehaviorsKey];
  DEV_ASSERT_MSG(!behaviorArray.isNull(),
                 "BehaviorDispatcherStrictPriorityWithCooldown.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorArray.isNull()) {
    for(const auto& behaviorDefinitionGroup: behaviorArray) {      
      const std::string& behaviorIDStr = JsonTools::ParseString(
        behaviorDefinitionGroup,
        "behavior",
        "BehaviorDispatcherStrictPriorityWithCooldown.BehaviorGroup.NoBehaviorID");
      
      IBehaviorDispatcher::AddPossibleDispatch(behaviorIDStr);
      
      _iConfig.cooldownInfo.emplace_back( BehaviorCooldownInfo{behaviorDefinitionGroup} );
      
      BehaviorTimerTypes timerType = BehaviorTimerTypes::Invalid;
      bool resetTimerOnDispatchActivation = false;
      {
        std::string timerName = BehaviorTimerTypesToString(BehaviorTimerTypes::Invalid);
        if ( JsonTools::GetValueOptional(behaviorDefinitionGroup, kLinkedBehaviorTimerKey, timerName) ) {
          ANKI_VERIFY(BehaviorTimerTypesFromString(timerName, timerType),
                      "BehaviorDispatcherStrictPriorityWithCooldown.InvalidBehaviorTimer",
                      "%s is not recognized as a valid BehaviorTimer type!", timerName.c_str());
          JsonTools::GetValueOptional(behaviorDefinitionGroup,
                                      kResetBehaviorTimerOnDispatcherActivationKey,
                                      resetTimerOnDispatchActivation);
        }
      }
      _iConfig.linkedBehaviorTimerInfo.emplace_back(timerType, resetTimerOnDispatchActivation);
    }
  }

  // initialize last idx to invalid value
  _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorsKey );
  expectedKeys.insert( kLinkScopeKey );
  expectedKeys.insert( kResetCooldownOnDeactivationKey );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::GetLinkedActivatableScopeBehaviors(
  std::set<IBehavior*>& delegates) const
{
  if( _iConfig.linkScope ) {
    // all delegates are also linked in scope
    BaseClass::GetAllDelegates(delegates);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatcherStrictPriorityWithCooldown::WantsToBeActivatedBehavior() const
{
  if( !_iConfig.linkScope ) {
    // not linking, use the default implementation
    return BaseClass::WantsToBeActivatedBehavior();
  }

  const size_t numBehaviors = _iConfig.cooldownInfo.size();

  for( size_t idx = 0; idx < numBehaviors; ++idx ) {
    auto& cooldownInfo = _iConfig.cooldownInfo.at(idx);
    const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);

    if( !cooldownInfo.OnCooldown() &&
        behavior->WantsToBeActivated() ) {
      return true;
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherStrictPriorityWithCooldown::GetDesiredBehavior()
{
  DEV_ASSERT_MSG( _iConfig.cooldownInfo.size() == IBehaviorDispatcher::GetAllPossibleDispatches().size(),
                  "BehaviorDispatcherStrictPriorityWithCooldown.SizeMismatch",
                  "have %zu cooldown infos, buts %zu possible dispatches",
                  _iConfig.cooldownInfo.size(),
                  IBehaviorDispatcher::GetAllPossibleDispatches().size());

  const size_t numBehaviors = _iConfig.cooldownInfo.size();

  for( size_t idx = 0; idx < numBehaviors; ++idx ) {
    auto& cooldownInfo = _iConfig.cooldownInfo.at(idx);
    const auto& behavior = IBehaviorDispatcher::GetAllPossibleDispatches().at(idx);

    if( !cooldownInfo.OnCooldown() &&
        ( behavior->IsActivated() ||
          behavior->WantsToBeActivated() ) ) {
      _dVars.lastDesiredBehaviorIdx = idx;
      return behavior;
    }
  }

  return ICozmoBehaviorPtr{};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::DispatcherUpdate()
{
  if( IsActivated() &&
      _dVars.lastDesiredBehaviorIdx < _iConfig.cooldownInfo.size() &&
      !IsControlDelegated() ) {
    // the last behavior must have stopped, so start its cooldown now
    LOG_INFO("BehaviorDispatcherStrictPriorityWithCooldown.LastBehaviorStopped.StartCooldown",
             "Behavior idx %zu '%s' seems to have stopped, start cooldown",
             _dVars.lastDesiredBehaviorIdx,
             IBehaviorDispatcher::GetAllPossibleDispatches()[_dVars.lastDesiredBehaviorIdx]->GetDebugLabel().c_str());
    _iConfig.cooldownInfo[ _dVars.lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
    
    // Reset the appropriate behavior timer if one was specified in the configuration
    const auto& timerToReset = _iConfig.linkedBehaviorTimerInfo[ _dVars.lastDesiredBehaviorIdx ].first;
    if ( timerToReset != BehaviorTimerTypes::Invalid ) {
      LOG_INFO("BehaviorDispatcherStrictPriorityWithCooldown.LastBehaviorStopped.ResetBehaviorTimer",
               "Behavior idx %zu '%s' seems to have stopped, resetting behavior timer %s",
               _dVars.lastDesiredBehaviorIdx,
               IBehaviorDispatcher::GetAllPossibleDispatches()[_dVars.lastDesiredBehaviorIdx]->GetDebugLabel().c_str(),
               BehaviorTimerTypesToString(timerToReset));
      GetBEI().GetBehaviorTimerManager().GetTimer(timerToReset).Reset();
    }
    
    _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
  }  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcher_OnActivated()
{
  // Some behaviors may have a linked behavior timer that they request to be reset upon
  // activation of the entire dispatcher. Iterate through the linked timers and reset the
  // ones specified by the configuration.
  size_t behaviorIdx = 0;
  for( const auto& linkedTimerInfo : _iConfig.linkedBehaviorTimerInfo ) {
    const auto& timerToReset = linkedTimerInfo.first;
    if ( timerToReset != BehaviorTimerTypes::Invalid ) {
      const bool shouldResetTimerOnDispatcherActivation = linkedTimerInfo.second;
      if( shouldResetTimerOnDispatcherActivation ) {
        LOG_INFO("BehaviorDispatcherStrictPriorityWithCooldown.OnActivated.ResetBehaviorTimer",
                 "Behavior idx %zu '%s' requested a reset of behavior timer %s upon activation of this dispatcher",
                 behaviorIdx,
                 IBehaviorDispatcher::GetAllPossibleDispatches()[behaviorIdx]->GetDebugLabel().c_str(),
                 BehaviorTimerTypesToString(timerToReset));
        GetBEI().GetBehaviorTimerManager().GetTimer(timerToReset).Reset();
      }
    }
    ++behaviorIdx;
  }
  
  // reset last dispatched behavior
  _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherStrictPriorityWithCooldown::BehaviorDispatcher_OnDeactivated()
{
  // If we are stopped while a behavior was active, put it on cooldown, and reset
  // its associated behavior timer (if one was specified).
  if( _dVars.lastDesiredBehaviorIdx < _iConfig.cooldownInfo.size() ) {
    if( !_iConfig.resetCooldownOnDeactivation ) {
      _iConfig.cooldownInfo[ _dVars.lastDesiredBehaviorIdx ].StartCooldown(GetRNG());
    }
    
    // Reset the appropriate behavior timer if one was specified in the configuration
    const auto& timerToReset = _iConfig.linkedBehaviorTimerInfo[ _dVars.lastDesiredBehaviorIdx ].first;
    if ( timerToReset != BehaviorTimerTypes::Invalid ) {
      LOG_INFO("BehaviorDispatcherStrictPriorityWithCooldown.OnDeactivated.ResetBehaviorTimer",
               "Behavior idx %zu '%s' was interrupted, resetting behavior timer %s",
               _dVars.lastDesiredBehaviorIdx,
               IBehaviorDispatcher::GetAllPossibleDispatches()[_dVars.lastDesiredBehaviorIdx]->GetDebugLabel().c_str(),
               BehaviorTimerTypesToString(timerToReset));
      GetBEI().GetBehaviorTimerManager().GetTimer(timerToReset).Reset();
    }
    
    _dVars.lastDesiredBehaviorIdx = _iConfig.cooldownInfo.size();
  }
  if( _iConfig.resetCooldownOnDeactivation ) {
    for( auto& cooldown : _iConfig.cooldownInfo ) {
      cooldown.ResetCooldown();
    }
  }
}

}
}
