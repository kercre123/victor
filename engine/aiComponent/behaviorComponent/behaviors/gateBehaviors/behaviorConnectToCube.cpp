/**
 * File: BehaviorConnectToCube.cpp
 *
 * Author: Sam Russell
 * Created: 2018-06-29
 *
 * Description: "Gate" behavior that should preceed any behavior that requiresCubeConnection in the BehaviorStack. This is enforced in ICozmoBehavior::WantsToBeActivated at runtime, and in Behavior System Unit Tests.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/gateBehaviors/behaviorConnectToCube.h"

#include "engine/actions/animActions.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"

#define SET_STATE(s) do{ \
                          _dVars.state = EState::s; \
                          PRINT_NAMED_INFO("BehaviorConnectToCube.State", "State = %s", #s); \
                        } while(0);

namespace{
const char* kDelegateBehaviorIDKey = "delegateBehaviorID";
}

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConnectToCube::InstanceConfig::InstanceConfig()
  : delegateBehavior(nullptr)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConnectToCube::DynamicVariables::DynamicVariables()
  : state(EState::Connecting)
  , connectedOnLastUpdate(false)
  , connectionFailedOnLastUpdate(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConnectToCube::BehaviorConnectToCube(const Json::Value& config)
  : ICozmoBehavior(config)
{
  JsonTools::GetValueOptional(config, kDelegateBehaviorIDKey, _iConfig.delegateBehaviorIDString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConnectToCube::~BehaviorConnectToCube()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::InitBehavior()
{
  if(!_iConfig.delegateBehaviorIDString.empty()){
    _iConfig.delegateBehavior = FindBehavior(_iConfig.delegateBehaviorIDString);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorConnectToCube::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if(nullptr != _iConfig.delegateBehavior){
    delegates.insert(_iConfig.delegateBehavior.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kDelegateBehaviorIDKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  // If we're already connected, 
  if(GetBEI().GetCubeConnectionCoordinator().IsConnectedToCube()){
    // TODO:(str) may want to do some little dance to indicate he knows he's already connected
    TransitionToConnected();
  } else {
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ConnectToCubeGetIn),
                        &BehaviorConnectToCube::TransitionToConnecting);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::BehaviorUpdate() 
{
  if(!IsActivated() ) {
    return;
  }

  if(EState::Connecting == _dVars.state){
    // Check in between each loop of the connecting animation so that we play at least one connection loop
    if(!IsControlDelegated()){
      if(_dVars.connectedOnLastUpdate){
        TransitionToConnectionSucceeded();
      } else if (_dVars.connectionFailedOnLastUpdate){
        TransitionToConnectionFailed();
      } else {
        DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ConnectToCubeLoop));
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::TransitionToConnecting()
{
  SET_STATE(Connecting);

  // TODO get appropriate animations
  // Loop the Connecting animation until we succeed or fail
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ConnectToCubeLoop));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::TransitionToConnectionSucceeded()
{
  SET_STATE(ConnectionSucceeded);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ConnectToCubeSuccess),
                      &BehaviorConnectToCube::TransitionToConnected);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::TransitionToConnectionFailed()
{
  SET_STATE(ConnectionFailed);
  // After playing the Failure anim, don't delegate, we're done
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ConnectToCubeFailure));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::TransitionToConnected()
{
  SET_STATE(Connected);
  if(nullptr != _iConfig.delegateBehavior){
    if(_iConfig.delegateBehavior->WantsToBeActivated()){
      // When our delegate behavior finishes, don't delegate, just move on silently
      DelegateIfInControl(_iConfig.delegateBehavior.get());
    } else {
      PRINT_NAMED_ERROR("BehaviorConnectToCube.DelegateDidNotWantToBeActivated",
                        "The child behavior %s of ConnectToCube did not want to be activated",
                        _iConfig.delegateBehavior->GetDebugLabel().c_str());
      // Since we don't delegate here, that will be the end of the behavior
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::TransitionToConnectionLost()
{
  SET_STATE(ConnectionLost);

  CancelDelegates(false);
  // Don't delegate in the callback so that we end behavior after animation
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ConnectToCubeLostConnection));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::ConnectedInteractableCallback()
{
  _dVars.connectedOnLastUpdate = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::ConnectionFailedCallback()
{
  if(EState::Connecting == _dVars.state){
    TransitionToConnectionFailed();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::ConnectionLostCallback()
{
  // In the event of a lost connection, always kill delegates and handle it here. There may
  // be use-cases for handling the connection loss lower in the tree eventually, in which case
  // this behavior could be linked to a json config value.
  TransitionToConnectionLost();
}

} // namespace Cozmo
} // namespace Anki
