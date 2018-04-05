/**
 * File: behaviorDispatcherRerun.h
 *
 * Author: Kevin M. Karol
 * Created: 10/24/17
 *
 * Description: A dispatcher which dispatches to its delegate the 
 * specified number of times before canceling itself - specifying -1 reruns
 * will cause the behavior to be dispatched infinitely
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherRerun.h"

#include "coretech/common/engine/jsonTools.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kConfigKeyDelegateID = "delegateID";
static const char* kConfigKeyNumRuns = "numRuns";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value BehaviorDispatcherRerun::CreateConfig(BehaviorID newConfigID, BehaviorID delegateID, const int numRuns)
{
  Json::Value config =  ICozmoBehavior::CreateDefaultBehaviorConfig(
    BEHAVIOR_CLASS(DispatcherRerun), newConfigID);
  config[kConfigKeyDelegateID] = BehaviorTypesWrapper::BehaviorIDToString(delegateID);
  config[kConfigKeyNumRuns] = numRuns;
  return config;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRerun::BehaviorDispatcherRerun(const Json::Value& config)
: ICozmoBehavior(config)
, _params()
, _numRunsRemaining(0)
{
  _params._delegateID = BehaviorTypesWrapper::BehaviorIDFromString(
    JsonTools::ParseString(config,
                           kConfigKeyDelegateID,
                           "BehaviorDispatcherRerun.Constructor.NoDelegateID"));
  
  _params._numRuns = JsonTools::ParseInt8(config, kConfigKeyNumRuns,
                                          "BehaviorDispatcherRerun.Constructor.numRunsNotSpecified");       
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRerun::~BehaviorDispatcherRerun()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRerun::InitBehavior()
{
  _delegatePtr = GetBEI().GetBehaviorContainer().FindBehaviorByID(_params._delegateID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRerun::OnBehaviorActivated()
{
  _numRunsRemaining = _params._numRuns;
  if(_delegatePtr != nullptr){
    CheckRerunState();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRerun::BehaviorUpdate()
{    
  CheckRerunState();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRerun::CheckRerunState()
{
  if(ANKI_VERIFY(GetBEI().HasDelegationComponent(),
                 "BehaviorDispatcherRerun.CheckRerunState.MissingDelegationComponent",
                 "")){
    auto& delegationComp = GetBEI().GetDelegationComponent();
    if(!delegationComp.IsControlDelegated(this)){
      if(_numRunsRemaining != 0 &&
         _delegatePtr->WantsToBeActivated() &&
         ANKI_VERIFY(delegationComp.HasDelegator(this),
                     "BehaviorDispatcherRerun.CheckRerunState.MissingDelegator",
                     ""))
      {
        delegationComp.GetDelegator(this).Delegate(this, _delegatePtr.get());
        if(_numRunsRemaining > 0){
          _numRunsRemaining--;
        }
      }else if(_numRunsRemaining == 0){
        delegationComp.CancelSelf(this);
      }
    } // end !IsControlDelegated
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRerun::OnBehaviorDeactivated()
{  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRerun::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if(_delegatePtr != nullptr){
    delegates.insert(_delegatePtr.get());    
  }
}
  
} // namespace Cozmo
} // namespace Anki
