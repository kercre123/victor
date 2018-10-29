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

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/activeObject.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotComponents_fwd.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"

#include "webServerProcess/src/webService.h"

#define LOG_CHANNEL "CubeConnectionCoordinator"

#define SET_STATE(s) do { \
                          SetState(ECoordinatorState::s); \
                          _debugStateString = #s; \
                        } while(0);

// Local constants
namespace{
// How long should we wait without non-background subscriptions before switching to a background connection
const float kStandbyTimeout_s = 15.0f;
// How long after the cube fade finishes should we remain interactable
const float kPostLightsGracePeriod_s = 0.5f;
// How long should we hold a background connection without subscribers
const float kBackgroundConnectionTimeout_s = 15.0f;
// How long should the CubeCommsComponent hold the connection after disconnect is requested
const float kDisconnectGracePeriodSec = 0.0f;

// Webviz
#if ANKI_DEV_CHEATS
const std::string kWebVizModuleNameCubes = "cubes";
const float kUpdateWebVizPeriod_s = 1.0f;
const float kDebugTempSubscriptionTimeout_s = 10.0f;

// Console test constants
static Anki::Vector::TestCubeConnectionSubscriber sStaticTestSubscriber1(1);
static Anki::Vector::TestCubeConnectionSubscriber sStaticTestSubscriber2(2);
static Anki::Vector::TestCubeConnectionSubscriber sStaticTestSubscriber3(3);
static Anki::Vector::TestCubeConnectionSubscriber sStaticTestSubscriber4(4);
#endif
}

namespace Anki{
namespace Vector{

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
, _connectionStartTime_s(0.0f)
, _nonBackgroundSubscriberCount(0)
, _connectionAttemptFinished(false)
, _connectionAttemptSucceeded(false)
, _connectionLostUnexpectedly(false)
, _cancelSwitchToBackground(false)
, _timeToEndStandby_s(0)
, _timeToSwitchToBackground_s(0)
, _timeToDisconnect_s(0)
, _connectedCubeActiveID(ObservableObject::InvalidActiveID)
, _connectedActiveObject(nullptr)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CubeConnectionCoordinator::~CubeConnectionCoordinator()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::InitDependent( Robot* robot, const RobotCompMap& dependentComps )
{
#if ANKI_DEV_CHEATS
  _context = robot->GetContext();
  SubscribeToWebViz();
#endif // ANKI_DEV_CHEATS
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::GetUpdateDependencies(RobotCompIDSet& dependencies) const
{
  dependencies.insert(RobotComponentID::BlockWorld);
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
      if(_cancelSwitchToBackground){
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

#if ANKI_DEV_CHEATS
  if(currentTime_s >= _timeToUpdateWebViz_s){
    SendDataToWebViz();
    _timeToUpdateWebViz_s = currentTime_s + kUpdateWebVizPeriod_s; 
  }
#endif //ANKI_DEV_CHEATS
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Public Interface
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::SubscribeToCubeConnection(ICubeConnectionSubscriber* const subscriber,
                                                          const bool connectInBackground, 
                                                          const float expireIn_s)
{
  if(_subscriptionRecords.empty()){
    _firstSubscriber = subscriber->GetCubeConnectionDebugName();
  }

  SubscriberRecord newRecord(subscriber, connectInBackground, expireIn_s);

  // Prevent duplicate subscriptions
  SubscriptionIterator it;
  if(FindRecordBySubscriber(subscriber, it)){
    LOG_INFO("CubeConnectionCoordinator.UpdateSubscription",
                     "Recieved new subscription for existing record, replacing record");
    UnsubscribeFromCubeConnection(subscriber);
  } 

  _subscriptionRecords.insert(newRecord);
  _timeToDisconnect_s = 0;

  if(!newRecord.isBackgroundConnection){
    ++_nonBackgroundSubscriberCount;
    _timeToEndStandby_s = 0;
    if(ECoordinatorState::ConnectedSwitchingToBackground == _coordinatorState){
      _cancelSwitchToBackground = true;
    }
  }

  // If we're already connected, invoke appropriate callbacks now since they won't be invoked by state transitions
  if(ECoordinatorState::ConnectedBackground == _coordinatorState){
    subscriber->ConnectedCallback(CubeConnectionType::Background);
  } else if(ECoordinatorState::ConnectedInteractable == _coordinatorState){
    subscriber->ConnectedCallback(CubeConnectionType::Interactable);
  }

#if ANKI_DEV_CHEATS
  SendDataToWebViz();
#endif // ANKI_DEV_CHEATS
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
      LOG_INFO("CubeConnectionCoordinator.NoInteractableSubscribers",
                        "No remaining interactable subscribers, transitioning to background in %f seconds",
                        kStandbyTimeout_s);
      _timeToEndStandby_s = currentTime_s + kStandbyTimeout_s;
    }
  }

  _subscriptionRecords.erase(it);
  if(_subscriptionRecords.empty()){
    // This timeout gets reset in TransitionToConnectedBackground, but it should be set from here so that we know
    // when to disconnect if already in ConnectedBackground state when our last subscriber drops
    _timeToDisconnect_s = currentTime_s + kBackgroundConnectionTimeout_s;
    _lastSubscriber = subscriber->GetCubeConnectionDebugName();
  }

#if ANKI_DEV_CHEATS
  SendDataToWebViz();
#endif // ANKI_DEV_CHEATS

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
    
    std::set<ICubeConnectionSubscriber*> subscribersRemovedThisTick;
    for(auto& subscriberRecord : _subscriptionRecords){
      // Track the subscribers getting dumped, the subscription record will be wiped to avoid errant state
      _subscribersDumpedByConnectionLoss.insert(subscriberRecord.subscriber);
      subscribersRemovedThisTick.insert(subscriberRecord.subscriber);
    }

    float connectionDuration_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _connectionStartTime_s; 
    DASMSG(cube_connection_lost,
           "cube_connection_coordinator.unexpected_disconnect",
           "Coordinator detected unexpected connection loss");
    DASMSG_SET(i1, !_subscriptionRecords.empty(), "1 if there were any subscribers at disconnect, 0 or null otherwise");
    DASMSG_SET(i2, std::round(connectionDuration_s), "Connection duration in seconds");
    DASMSG_SEND();

    // Clear out antequated state
    _subscriptionRecords.clear();
    _nonBackgroundSubscriberCount = 0;
    _timeToEndStandby_s = 0;
    _timeToSwitchToBackground_s = 0;
    _timeToDisconnect_s = 0;
    _connectionLostUnexpectedly = true;
    _connectedCubeActiveID = ObservableObject::InvalidActiveID;
    _connectedActiveObject = nullptr;

    // Set Status lights back to default state
    bool connectedInBackground = true;
    dependentComps.GetComponent<CubeLightComponent>().SetCubeBackgroundState(connectedInBackground);

    SET_STATE(UnConnected);
    
    // do a separate pass through the list instead of the previous loop over _subscriptionRecords,
    // since that could be modified by ConnectionLostCallback() if, e.g., the subscriber wants to reconnect
    for(auto* subscriber : subscribersRemovedThisTick){
      subscriber->ConnectionLostCallback();
    }
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
  SET_STATE(ConnectedInteractable);

  // Play connection light animation
  auto& cubeLights = dependentComps.GetComponent<CubeLightComponent>();
  bool connectedInBackground = false;
  cubeLights.SetCubeBackgroundState(connectedInBackground);

  if(nullptr != _connectedActiveObject){
    cubeLights.PlayConnectionLights(_connectedActiveObject->GetID());
  }

  // Notify subscribers
  for(auto& subscriberRecord : _subscriptionRecords){
    subscriberRecord.subscriber->ConnectedCallback(CubeConnectionType::Interactable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::TransitionToSwitchingToBackground(const RobotCompMap& dependentComps)
{
  SET_STATE(ConnectedSwitchingToBackground);
  _timeToSwitchToBackground_s = 0;

  LOG_INFO("CubeConnectionCoordinator.SwitchingToBackgroundConnection",
                   "Switching to background connection upon cube light anim completion");

  // Play disconnect light animation
  auto& cubeLights = dependentComps.GetComponent<CubeLightComponent>();
  auto animCompletedCallback = [this, &cubeLights]()
  { 
    // It's possible a subscription came in while playing disconnect lights from either the user or an internal action.
    // If that's the case, don't modify the state of the CubeConnectionCoordinator from the callback.
    if(!_cancelSwitchToBackground){
      float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _timeToSwitchToBackground_s = currentTime_s + kPostLightsGracePeriod_s;
      bool connectedInBackground = true;
      cubeLights.SetCubeBackgroundState(connectedInBackground);
    }
  };

  if(nullptr != _connectedActiveObject){
    if(!cubeLights.PlayDisconnectionLights(_connectedActiveObject->GetID(), animCompletedCallback)){
      // If the lights didn't play, the callback will never come from the light component. Invoke it now.
      animCompletedCallback();
    }
  } else {
    // We don't have a valid object to play lights on, the callback will never come, invoke it now
    animCompletedCallback();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::CancelSwitchToBackground(const RobotCompMap& dependentComps)
{
  // Don't play connection lights or notify subscribers, since the cube connection is still live
  SET_STATE(ConnectedInteractable);

  LOG_INFO("CubeConnectionCoordinator.SwitchToBackgroundInterrupted",
                   "New interactable subscription received while switching to background, returning to interactable");

  // Stop the disconnection lights if they're playing
  auto& clc = dependentComps.GetComponent<CubeLightComponent>();
  clc.CancelDisconnectionLights(_connectedActiveObject->GetID());

  _cancelSwitchToBackground = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::TransitionToConnectedBackground(const RobotCompMap& dependentComps)
{
  SET_STATE(ConnectedBackground);

  if(_subscriptionRecords.empty()){
    // Reset the timeout in case the last unsubscribe was a while ago. This ensures a minimum Background time of 
    // kBackgroundConnectionTimeout_s before disconnecting
    _timeToDisconnect_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kBackgroundConnectionTimeout_s;
  }

  // Notify subscribers
  for(auto& subscriberRecord : _subscriptionRecords){
    subscriberRecord.subscriber->ConnectedCallback(CubeConnectionType::Background);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::RequestConnection(const RobotCompMap& dependentComps)
{
  SET_STATE(Connecting);

  DASMSG(cube_connection_requested,
         "cube_connection_coordinator.connection_requested",
         "Cube connection request sent from Coordinator");
  DASMSG_SET(s1, _firstSubscriber, "First subscription for current connection");
  DASMSG_SEND();

  LOG_INFO("CubeConnectionCoordinator.RequestingConnection",
                    "Requesting cube connection from CubeCommsComponent");

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
  SET_STATE(Disconnecting);

  float connectionDuration_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _connectionStartTime_s; 
  DASMSG(cube_disconnect_requested,
         "cube_connection_coordinator.disconnect_requested",
         "No subscribers remaining. Disconnect requested by coordinator");
  DASMSG_SET(s1, _lastSubscriber, "Last subscriber before current connection closed");
  DASMSG_SET(i1, std::round(connectionDuration_s), "Connection duration in seconds");
  DASMSG_SEND();

  LOG_INFO("CubeConnectionCoordinator.RequestingDisconnect",
                    "Requesting disconnection from CubeCommsComponent");

  if(!_subscriptionRecords.empty()){
    PRINT_NAMED_ERROR("CubeConnectionCoordinator.DisconnectedWithSubscribers",
                      "Cube disconnected despite having active subscribers");
  }

  auto& cubeComms = dependentComps.GetComponent<CubeCommsComponent>();
  auto disconnectCallback = [this](bool success)
  {
    SET_STATE(UnConnected);
    _connectedCubeActiveID = ObservableObject::InvalidActiveID;
    _connectedActiveObject = nullptr;
  };
  cubeComms.RequestDisconnectFromCube(kDisconnectGracePeriodSec, disconnectCallback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::HandleConnectionAttemptResult(const RobotCompMap& dependentComps)
{
  if(!_connectionAttemptSucceeded){
    PRINT_NAMED_WARNING("CubeConnectionCoordinator.ConnectionFailed",
                        "CubeCommsComponent failed to establish a cube connection.");

    // The Connection attempt failed. Clear out the subscription record so that further connection attempts are made
    // only if subscribers drive them.
    std::set<ICubeConnectionSubscriber*> subscribersRemovedThisTick;
    for(auto& subscriberRecord : _subscriptionRecords){
      subscribersRemovedThisTick.insert(subscriberRecord.subscriber);
    }
    _subscriptionRecords.clear();
    _nonBackgroundSubscriberCount = 0;

    SET_STATE(UnConnected);

    // Notify former subscribers of the failed attempt after clearing the record so that re-subscribes aren't
    // immediately thrown out.
    for(auto* subscriber : subscribersRemovedThisTick){
      subscriber->ConnectionFailedCallback();
    }

  } else {
    _connectionStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    auto& cubeComms = dependentComps.GetComponent<CubeCommsComponent>();
    auto& bw = dependentComps.GetComponent<BlockWorld>();
    _connectedCubeActiveID = cubeComms.GetConnectedCubeActiveId();
    _connectedActiveObject = bw.GetConnectedActiveObjectByActiveID(_connectedCubeActiveID);
    ANKI_VERIFY(nullptr != _connectedActiveObject,
                "CubeConnectionCoordinator.NoObjectFoundForActiveID",
                "Block world did not have a connected active object for CubeComms connected cube activeID");
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
#if ANKI_DEV_CHEATS
  SendDataToWebViz();
#endif // ANKI_DEV_CHEATS
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::SubscribeToWebViz()
{
#if ANKI_DEV_CHEATS
  if(nullptr != _context){
    auto* webService = _context->GetWebService();
    if(nullptr != webService){
      auto onData = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {

        const auto& subscribeInteractable = data["subscribeInteractable"];
        if(subscribeInteractable.isBool()){
          if(subscribeInteractable.asBool()){
            SubscribeToCubeConnection(&sStaticTestSubscriber1, false);
          } else {
            UnsubscribeFromCubeConnection(&sStaticTestSubscriber1);
          }
        }

        const auto& subscribeTempInteractable = data["subscribeTempInteractable"];
        if(subscribeTempInteractable.isBool()){
            SubscribeToCubeConnection(&sStaticTestSubscriber2, false, kDebugTempSubscriptionTimeout_s);
        }

        const auto& subscribeBackground = data["subscribeBackground"];
        if(subscribeBackground.isBool()){
          if(subscribeBackground.asBool()){
            SubscribeToCubeConnection(&sStaticTestSubscriber3, true);
          } else {
            UnsubscribeFromCubeConnection(&sStaticTestSubscriber3);
          }
        }

        const auto& subscribeTempBackground = data["subscribeTempBackground"];
        if(subscribeTempBackground.isBool()){
            SubscribeToCubeConnection(&sStaticTestSubscriber4, true, kDebugTempSubscriptionTimeout_s);
        }

      };

      _signalHandles.emplace_back(webService->OnWebVizData(kWebVizModuleNameCubes).ScopedSubscribe(onData));
    }
  }
#endif // ANKI_DEV_CHEATS
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeConnectionCoordinator::SendDataToWebViz()
{
#if ANKI_DEV_CHEATS
  if(nullptr != _context){
    auto* webService = _context->GetWebService();
    if(nullptr != webService && webService->IsWebVizClientSubscribed(kWebVizModuleNameCubes)){
      Json::Value toSend = Json::objectValue;
      Json::Value cccInfo = Json::objectValue;

      const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

      // Display Status Info
      cccInfo["connectionState"] = _debugStateString;
      Json::Value stateCountdown = Json::arrayValue;
      if( (ECoordinatorState::ConnectedInteractable == _coordinatorState) && (_timeToEndStandby_s != 0) ){
        Json::Value blob;
        blob["SwitchingToBackgroundIn"] = (int)(_timeToEndStandby_s - currentTime_s);
        stateCountdown.append(blob);
      } else if ( (ECoordinatorState::ConnectedBackground == _coordinatorState) && (_timeToDisconnect_s != 0) ){
        Json::Value blob;
        blob["DisconnectingIn"] = (int)(_timeToDisconnect_s - currentTime_s);
        stateCountdown.append(blob);
      }
      cccInfo["stateCountdown"] = stateCountdown;
      
      // Display current subscribers
      Json::Value subscriberData = Json::arrayValue;
      for(const auto& record : _subscriptionRecords){
        Json::Value subscriberDatum;
        subscriberDatum["SubscriberName"] = record.subscriber->GetCubeConnectionDebugName();
        subscriberDatum["SubscriptionType"] = record.isBackgroundConnection ? "Background" : "Interactable";
        if(record.expirationTime_s >= 0.0f){
          subscriberDatum["ExpiresIn"] = (int)(record.expirationTime_s - currentTime_s);
        }
        subscriberData.append(subscriberDatum);
      }
      cccInfo["subscriberData"] = subscriberData;
      toSend["cccInfo"] = cccInfo;
      webService->SendToWebViz(kWebVizModuleNameCubes, toSend);
    }
  }
#endif // ANKI_DEV_CHEATS
}

} // namespace Vector
} // namespace Anki
