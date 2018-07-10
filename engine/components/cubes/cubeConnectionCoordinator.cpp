/**
* File: cubeConnectionCoordinator.cpp
*
* Author: Sam Russell 
* Created: 6/20/18
*
* Description: Component to which behaviors can submit cube connection subscriptions and cube connection state queries.
*              Based on subscriptions, opens and closes cube connections and manages "fundamental" cube connection light
*              animations
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/components/cubes/cubeConnectionCoordinator.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/robot.h"
#include "engine/robotComponents_fwd.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#define CONNECTION_DEBUG_LIGHTS 1

// Local constants
namespace{
// How long should we wait without non-bakcground subscriptions before switching to a background connection 
const float kStandbyTimeout_s = 15.0f;
// How long after the cube fade finishes should we remain interactable
const float kPostLightsGracePeriod_s = 0.5f;
// How long should we hold a background connection without subscribers
const float kBackgroundConnectionTimeout_s = 15.0f;
// How long should the CubeCommsComponent hold the connection after disconnect is requested
const float kDisconnectGracePeriodSec = 0.0f;

// Console test constants
#if ANKI_DEV_CHEATS
static Anki::Cozmo::CubeConnectionCoordinator* sThis = nullptr;
static Anki::Cozmo::TestCubeConnectionSubscriber sStaticTestSubscriber1(1);
static Anki::Cozmo::TestCubeConnectionSubscriber sStaticTestSubscriber2(2);
static Anki::Cozmo::TestCubeConnectionSubscriber sStaticTestSubscriber3(3);
#endif
}

namespace Anki{
namespace Cozmo{

#if ANKI_DEV_CHEATS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BackgroundSubscribe(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    bool connectInBackground = true;
    sThis->SubscribeToCubeConnection(&sStaticTestSubscriber1, connectInBackground);
  }
}
CONSOLE_FUNC(BackgroundSubscribe, "CubeConnectionCoordinator.TestSubscriptions");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BackgroundUnsubscribe(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->UnsubscribeFromCubeConnection(&sStaticTestSubscriber1);
  }
}
CONSOLE_FUNC(BackgroundUnsubscribe, "CubeConnectionCoordinator.TestSubscriptions");


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InteractableSubscribe(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    bool connectInBackground = false;
    sThis->SubscribeToCubeConnection(&sStaticTestSubscriber2, connectInBackground);
  }
}
CONSOLE_FUNC(InteractableSubscribe, "CubeConnectionCoordinator.TestSubscriptions");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InteractableUnsubscribe(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->UnsubscribeFromCubeConnection(&sStaticTestSubscriber2);
  }
}
CONSOLE_FUNC(InteractableUnsubscribe, "CubeConnectionCoordinator.TestSubscriptions");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TempInteractableSubscribe(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    bool connectInBackground = false;
    float expireIn_s = 5.0f;
    sThis->SubscribeToCubeConnection(&sStaticTestSubscriber3, connectInBackground, expireIn_s);
  }
}
CONSOLE_FUNC(TempInteractableSubscribe, "CubeConnectionCoordinator.TestSubscriptions");
#endif // ANKI_DEV_CHEATS

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CubeConnectionCoordinator::SubscriberRecord::SubscriberRecord(ICubeConnectionSubscriber* const subscriberPtr,
                                                              bool connectInBackground,
                                                              float expireIn_s)
  : subscriber(subscriberPtr)
  , isBackgroundConnection(connectInBackground)
  , expirationTime_s(expireIn_s == kDontExpire ? 
                                   kDontExpire :
                                   BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + expireIn_s)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CubeConnectionCoordinator::CubeConnectionCoordinator()
  : IDependencyManagedComponent( this, RobotComponentID::CubeConnectionCoordinator )
  , _coordinatorState(ECoordinatorState::UnConnected)
{
#if ANKI_DEV_CHEATS
  sThis = this;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CubeConnectionCoordinator::~CubeConnectionCoordinator()
{
#if ANKI_DEV_CHEATS
  sThis = nullptr;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::GetUpdateDependencies(RobotCompIDSet& dependencies) const
{
  dependencies.insert(RobotComponentID::CubeComms);
  dependencies.insert(RobotComponentID::CubeLights);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::UpdateDependent(const RobotCompMap& dependentComps)
{
  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  switch(_coordinatorState){
    case ECoordinatorState::UnConnected:
    {
      if(!_subscriptionRecords.empty()){
        RequestConnection(dependentComps);
      }
     break;
    }
    case ECoordinatorState::Connecting:
    {
      if(_connectionAttemptFinished){
        HandleConnectionAttemptResult(dependentComps);
      }
      break;
    }
    case ECoordinatorState::Disconnecting:
    {
      // Wait for the connection state to solidify
      break;
    }
    case ECoordinatorState::ConnectedInteractable:
    {
      CheckForConnectionLoss(dependentComps);
      PruneExpiredSubscriptions();

      // If we've been without subscribers for long enough, convert to a background connection
      if(_timeToEndStandby_s != 0 && 
         _timeToEndStandby_s < currentTime_s){
        TransitionToSwitchingToBackground(dependentComps);
        return;
      }
      break;
    }
    case ECoordinatorState::ConnectedSwitchingToBackground:
    {
      CheckForConnectionLoss(dependentComps);
      if(_nonBackgroundSubscriberCount != 0){
        CancelSwitchToBackground(dependentComps);
        return;
      }

      if(_timeToSwitchToBackground_s != 0 &&
         _timeToSwitchToBackground_s < currentTime_s){
        TransitionToConnectedBackground(dependentComps);
        return;
      }

      break;
    }
    case ECoordinatorState::ConnectedBackground:
    {
      CheckForConnectionLoss(dependentComps);
      PruneExpiredSubscriptions();

      if(_nonBackgroundSubscriberCount > 0){
        TransitionToConnectedInteractable(dependentComps);
        return;
      }

      if(_timeToDisconnect_s != 0 &&
         _timeToDisconnect_s < currentTime_s){
        RequestDisconnect(dependentComps);
      }
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Public Interface
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::SubscribeToCubeConnection(ICubeConnectionSubscriber* const subscriber,
                                                          const bool connectInBackground, 
                                                          const float expireIn_s)
{
  SubscriberRecord newRecord(subscriber, connectInBackground, expireIn_s);

  // Prevent duplicate subscriptions
  SubscriptionIterator it;
  if(FindRecordBySubscriber(subscriber, it)){
    PRINT_NAMED_INFO("CubeConnectionCoordinator.UpdateSubscription",
                     "Recieved new subscription for existing record, replacing record");
    UnsubscribeFromCubeConnection(subscriber);
  } 

  _subscriptionRecords.insert(newRecord);
  _timeToDisconnect_s = 0;

  if(!newRecord.isBackgroundConnection){
    ++_nonBackgroundSubscriberCount;
    _timeToEndStandby_s = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CubeConnectionCoordinator::UnsubscribeFromCubeConnection(ICubeConnectionSubscriber* const subscriber)
{
  SubscriptionIterator it;
  if(!FindRecordBySubscriber(subscriber, it)){
    bool subscriberWasDumped = false;
    if(_connectionLostUnexpectedly){
      // If we recently lost connection, allow unsubscribes from dumped subscribers without errors 
      auto it = _subscribersDumpedByConnectionLoss.find(subscriber);
      subscriberWasDumped = (it != _subscribersDumpedByConnectionLoss.end());
      if(subscriberWasDumped){
        // Only allow this once, more than one unsubscribe is indeed an error
        _subscribersDumpedByConnectionLoss.erase(subscriber);
      }
    }
    if(!subscriberWasDumped){
      PRINT_NAMED_ERROR("CubeConnectionCoordinator.InvalidUnsubscribeRequest",
                          "Unsubscribe requested from subscriber with no subscription record");
    }
    return false;
  }
 
  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(!it->isBackgroundConnection){
    if(--_nonBackgroundSubscriberCount == 0){
      _timeToEndStandby_s = currentTime_s + kStandbyTimeout_s;
    }
  }

  _subscriptionRecords.erase(it);
  if(_subscriptionRecords.empty()){
    // This timeout gets reset in TransitionToConnectedBackground, but it should be set from here so that we know
    // when to disconnect if already in ConnectedBackground state when our last subscriber drops
    _timeToDisconnect_s = currentTime_s + kBackgroundConnectionTimeout_s;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::CheckForConnectionLoss(const RobotCompMap& dependentComps)
{
  if(!dependentComps.GetComponent<CubeCommsComponent>().IsConnectedToCube()){
    PRINT_NAMED_WARNING("CubeConnectionCoordinator.ConnectionDropped",
                        "Connection lost unexpectedly. Notifying subscribers and dumping subscription record");
    for(auto& subscriberRecord : _subscriptionRecords){
      subscriberRecord.subscriber->ConnectionLostCallback();
      // Track the subscribers getting dumped, the subscription record will be wiped to avoid errant state
      _subscribersDumpedByConnectionLoss.insert(subscriberRecord.subscriber);
    }

    // Clear out antequated state
    _subscriptionRecords.clear();
    _nonBackgroundSubscriberCount = 0;
    _timeToEndStandby_s = 0;
    _timeToSwitchToBackground_s = 0;
    _timeToDisconnect_s = 0;
    _connectionLostUnexpectedly = true;

    // Set Status lights back to default state
    dependentComps.GetComponent<CubeLightComponent>().EnableStatusAnims(false);

    SetState(ECoordinatorState::UnConnected);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::PruneExpiredSubscriptions()
{
  // Prune expired subscriptions
  {
    auto it = _subscriptionRecords.begin();
    while( (it != _subscriptionRecords.end()) &&
           (it->expirationTime_s != SubscriberRecord::kDontExpire) &&
           (it->expirationTime_s < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) ){
      UnsubscribeFromCubeConnection(it->subscriber);
      it = _subscriptionRecords.begin();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::TransitionToConnectedInteractable(const RobotCompMap& dependentComps)
{
  SetState(ECoordinatorState::ConnectedInteractable);

  // Play connection light animation
  ObjectID connectedCube = dependentComps.GetComponent<CubeCommsComponent>().GetConnectedCubeActiveId(); 

  auto& cubeLights = dependentComps.GetComponent<CubeLightComponent>();
  cubeLights.EnableStatusAnims(true);
  cubeLights.PlayConnectionLights(connectedCube);

  // Notify subscribers
  for(auto& subscriberRecord : _subscriptionRecords){
    subscriberRecord.subscriber->ConnectedInteractableCallback();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::TransitionToSwitchingToBackground(const RobotCompMap& dependentComps)
{
  SetState(ECoordinatorState::ConnectedSwitchingToBackground);
  _timeToSwitchToBackground_s = 0;

  // Play disconnect light animation
  ObjectID connectedCube = dependentComps.GetComponent<CubeCommsComponent>().GetConnectedCubeActiveId();
  auto& cubeLights = dependentComps.GetComponent<CubeLightComponent>();
  auto animCompletedCallback = [this, &cubeLights]()
  { 
    // It's possible a subscription came in while playing disconnect lights from either the user or an internal action.
    // If that's the case, don't modify the state of either the CubeLightComponent or the CubeConnectionCoordinator from
    // the callback.
    if(ECoordinatorState::ConnectedSwitchingToBackground == _coordinatorState){
      float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _timeToSwitchToBackground_s = currentTime_s + kPostLightsGracePeriod_s;
      // Tell the CubeLightComponent not to display status lights since we're functionally disconnected from the user's
      // perspective
      cubeLights.EnableStatusAnims(false);
    }
  };
  cubeLights.PlayDisconnectionLights(connectedCube, animCompletedCallback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::CancelSwitchToBackground(const RobotCompMap& dependentComps)
{
  // Don't play connection lights or notify subscribers, since the cube connection is still live
  SetState(ECoordinatorState::ConnectedInteractable);
  // Do re-enable status lights though, since they are disabled at the start of the postLights grace period
  dependentComps.GetComponent<CubeLightComponent>().EnableStatusAnims(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::TransitionToConnectedBackground(const RobotCompMap& dependentComps)
{
  SetState(ECoordinatorState::ConnectedBackground);

  if(_subscriptionRecords.empty()){
    // Reset the timeout in case the last unsubscribe was a while ago. This ensures a minimum Background time of 
    // kBackgroundConnectionTimeout_s before disconnecting
    _timeToDisconnect_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kBackgroundConnectionTimeout_s;
  }

  // Notify subscribers
  for(auto& subscriberRecord : _subscriptionRecords){
    subscriberRecord.subscriber->ConnectedBackgroundCallback();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::RequestConnection(const RobotCompMap& dependentComps)
{
  SetState(ECoordinatorState::Connecting);

  _connectionAttemptFinished = false;
  _connectionAttemptSucceeded = false;

  auto& cubeComms = dependentComps.GetComponent<CubeCommsComponent>();
  auto callback = [this](bool success)
  {
    _connectionAttemptFinished = true;
    _connectionAttemptSucceeded = success;
  };
  if(cubeComms.IsConnectedToCube()){
    callback(true);
  } else {
    if(!cubeComms.RequestConnectToCube(callback)){
      callback(false);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::RequestDisconnect(const RobotCompMap& dependentComps)
{
  SetState(ECoordinatorState::Disconnecting);

  if(!_subscriptionRecords.empty()){
    PRINT_NAMED_ERROR("CubeConnectionCoordinator.DisconnectedWithSubscribers",
                      "Cube disconnected despite having active subscribers");
  }

  auto& cubeComms = dependentComps.GetComponent<CubeCommsComponent>();
  auto disconnectCallback = [this](bool success)
  {
    SetState(ECoordinatorState::UnConnected);
  };
  cubeComms.RequestDisconnectFromCube(kDisconnectGracePeriodSec, disconnectCallback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::HandleConnectionAttemptResult(const RobotCompMap& dependentComps)
{
  if(!_connectionAttemptSucceeded){
    PRINT_NAMED_WARNING("CubeConnectionCoordinator.ConnectionFailed",
                        "CubeCommsComponent failed to establish a cube connection.");
    SetState(ECoordinatorState::UnConnected);
    for(auto& subscriberRecord : _subscriptionRecords){
      subscriberRecord.subscriber->ConnectionFailedCallback();
    }
  } else {
    if(_nonBackgroundSubscriberCount == 0){
      TransitionToConnectedBackground(dependentComps);
    } else {
      TransitionToConnectedInteractable(dependentComps);
    }

  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::SetState(ECoordinatorState newState)
{
  _coordinatorState = newState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CubeConnectionCoordinator::FindRecordBySubscriber(ICubeConnectionSubscriber* const subscriber,
                                                       SubscriptionIterator& iterator)
{
  iterator = _subscriptionRecords.begin();
  while(iterator != _subscriptionRecords.end()){
    if(iterator->subscriber == subscriber){
      return true;
    }
    ++iterator;
  }
  return false;
}

} // namespace Cozmo
} // namespace Anki
