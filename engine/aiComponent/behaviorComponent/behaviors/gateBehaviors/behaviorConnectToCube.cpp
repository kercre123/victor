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
                          PRINT_CH_INFO("Behaviors", "BehaviorConnectToCube.State", "State = %s", #s); \
                        } while(0);

namespace{
const char* kDelegateBehaviorIDKey = "delegateBehaviorID";

// Note: this should only be used when delegating to Queue's or the like which have at least one
// delegation option which is does not have RequiredLazy or RequiredManaged cubeConnectionRequirements
const char* kDelegateOnFailedConnectionKey = "delegateOnFailedConnection";
}

namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConnectToCube::InstanceConfig::InstanceConfig()
  : delegateBehavior(nullptr)
  , delegateOnFailedConnection(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConnectToCube::DynamicVariables::DynamicVariables()
  : state(EState::Connecting)
  , connectedInBackground(false)
  , subscribedInteractable(false)
  , connectedOnLastUpdate(false)
  , connectionFailedOnLastUpdate(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorConnectToCube::BehaviorConnectToCube(const Json::Value& config)
  : ICozmoBehavior(config)
{
  JsonTools::GetValueOptional(config, kDelegateBehaviorIDKey, _iConfig.delegateBehaviorIDString);
  JsonTools::GetValueOptional(config, kDelegateOnFailedConnectionKey, _iConfig.delegateOnFailedConnection);
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
    kDelegateOnFailedConnectionKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  // If we're already connected interactable, 
  if(GetBEI().GetCubeConnectionCoordinator().IsConnectedInteractable()){
    // Go ahead and delegate to our child without making any noise about connecting
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
    // Only once we've started the connection process, check if we've achieved a background connection
    if(_dVars.connectedInBackground && !_dVars.subscribedInteractable){
      // Once we've got a connection, switch the subscription to foreground to enable lights
      GetBEI().GetCubeConnectionCoordinator().SubscribeToCubeConnection(this);
    }

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
  // Convert our extant background connection to foreground
  GetBEI().GetCubeConnectionCoordinator().SubscribeToCubeConnection(this);
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
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ConnectToCubeFailure),
    [this](){
      if(_iConfig.delegateOnFailedConnection){
        TransitionToDelegatedWithoutConnection();
      }
    });
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
void BehaviorConnectToCube::TransitionToDelegatedWithoutConnection()
{
  SET_STATE(DelegatedWithoutConnection);
  PRINT_NAMED_WARNING("BehaviorConnectToCube.DelegatedWithoutConnection",
                      "Connection attempt failed, but config indicated child behaviors with optional connections");

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
void BehaviorConnectToCube::ConnectedCallback(CubeConnectionType connectionType)
{
  if(CubeConnectionType::Background == connectionType){
    _dVars.connectedInBackground = true;
  }
  else if(CubeConnectionType::Interactable == connectionType){
    _dVars.connectedInBackground = false;
    _dVars.connectedOnLastUpdate = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::ConnectionFailedCallback()
{
  _dVars.connectionFailedOnLastUpdate = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorConnectToCube::ConnectionLostCallback()
{
  // In the event of a lost connection, always kill delegates and handle it here. There may
  // be use-cases for handling the connection loss lower in the tree eventually, in which case
  // this behavior could be linked to a json config value.
  TransitionToConnectionLost();
}

} // namespace Vector
} // namespace Anki
