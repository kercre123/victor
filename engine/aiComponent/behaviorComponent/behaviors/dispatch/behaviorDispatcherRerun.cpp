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

#include "anki/common/basestation/jsonTools.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "clad/types/behaviorComponent/behaviorTypes.h"

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
                            BehaviorClass::BehaviorDispatcherRerun, newConfigID);
  config[kConfigKeyDelegateID] = BehaviorIDToString(delegateID);
  config[kConfigKeyNumRuns] = numRuns;
  return config;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherRerun::BehaviorDispatcherRerun(const Json::Value& config)
: ICozmoBehavior(config)
, _params()
, _numRunsRemaining(0)
{
  _params._delegateID = BehaviorIDFromString(JsonTools::ParseString(config,
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
void BehaviorDispatcherRerun::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  _delegatePtr = behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(_params._delegateID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDispatcherRerun::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _numRunsRemaining = _params._numRuns;
  if(_delegatePtr != nullptr){
    CheckRerunState(behaviorExternalInterface);
    return RESULT_OK;
  }

  return RESULT_FAIL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRerun::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)
{    
  CheckRerunState(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherRerun::CheckRerunState(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(ANKI_VERIFY(behaviorExternalInterface.HasDelegationComponent(),
                 "BehaviorDispatcherRerun.CheckRerunState.MissingDelegationComponent",
                 "")){
    auto& delegationComp = behaviorExternalInterface.GetDelegationComponent();
    if(!delegationComp.IsControlDelegated(this)){
      if(_numRunsRemaining != 0 &&
         _delegatePtr->WantsToBeActivated(behaviorExternalInterface) &&
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
void BehaviorDispatcherRerun::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
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
