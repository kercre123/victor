/**
* File: behaviorDispatcherPassThrough.cpp
*
* Author: Kevin M. Karol
* Created: 2/26/18
*
* Description: Defines the base class for a "Pass Through" behavior
*  This style of behavior has one and only one delegate which follow the lifecycle
*  of the pass through exactly. The pass through implementation can then perform
*  other updates (setting lights/coordinating between behaviors) but may not 
*  delegate to an alternative behavior or action
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherPassThrough.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kBehaviorIDConfigKey = "delegateID";
}



///////////
/// BehaviorDispatcherPassThrough
///////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherPassThrough::BehaviorDispatcherPassThrough(const Json::Value& config)
: ICozmoBehavior(config)
{
  auto debugStr = "BehaviorDispatcherPassThrough.Constructor.MissingDelegateID";
  _iConfig.delegateID = JsonTools::ParseString(config, kBehaviorIDConfigKey, debugStr);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherPassThrough::~BehaviorDispatcherPassThrough()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherPassThrough::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.delegate.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherPassThrough::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  _iConfig.delegate->GetBehaviorOperationModifiers(modifiers);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherPassThrough::InitBehavior()
{
  BehaviorID delegateID = BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.delegateID);
  _iConfig.delegate = GetBEI().GetBehaviorContainer().FindBehaviorByID(delegateID);
  InitPassThrough();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDispatcherPassThrough::WantsToBeActivatedBehavior() const 
{
  return _iConfig.delegate->WantsToBeActivated();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherPassThrough::OnBehaviorActivated() 
{
  _dVars = DynamicVariables();

  {
    // Don't allow delegation within PassThrough calls
    auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
    OnPassThroughActivated();
  }
  
  DelegateIfInControl(_iConfig.delegate.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherPassThrough::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  {
    // Don't allow delegation within PassThrough calls
    auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
    PassThroughUpdate();
  }


  if(!IsControlDelegated()){
    CancelSelf();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherPassThrough::OnBehaviorDeactivated()
{
  OnPassThroughDeactivated();
  CancelDelegates();
}
  

} // namespace Cozmo
} // namespace Anki
