/**
* File: cubeConnectionCoordinator.h
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

#ifndef __Engine_AiComponent_BehaviorComponent_CubeConnectionCoordinator_H__
#define __Engine_AiComponent_BehaviorComponent_CubeConnectionCoordinator_H__


#include "engine/components/cubes/iCubeConnectionSubscriber.h"
#include "engine/cozmoObservableObject.h"
#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h" // Signal::SmartHandle

namespace Anki{
namespace Vector{

// Fwd Delarations
class CozmoContext;
class ActiveObject;

class CubeConnectionCoordinator : public IDependencyManagedComponent<RobotComponentID>, public Anki::Util::noncopyable
{
public:
  CubeConnectionCoordinator();
  ~CubeConnectionCoordinator();

  // Dependency Managed Component Methods
  virtual void GetInitDependencies(RobotCompIDSet& dependencies ) const override {}
  virtual void InitDependent( Robot* robot, const RobotCompMap& dependentComps ) override;
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies ) const override;
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  // Dependency Managed Component Methods

  // Note that subscriptions do not expire by default. Subscriptions with expireIn_s == 0 expire immediately, so they 
  // will trigger connections if unconnected, or reset appropriate timeouts if currently connected.
  void SubscribeToCubeConnection(ICubeConnectionSubscriber* const subscriber,
                                 const bool connectInBackground = false,
                                 const float expireIn_s = -1);
  bool UnsubscribeFromCubeConnection(ICubeConnectionSubscriber* const subscriber);

  bool IsConnectedToCube(){ return (ECoordinatorState::ConnectedInteractable          == _coordinatorState ||
                                    ECoordinatorState::ConnectedBackground            == _coordinatorState ||
                                    ECoordinatorState::ConnectedSwitchingToBackground == _coordinatorState); }

  bool IsConnectedInteractable(){ return (ECoordinatorState::ConnectedInteractable          == _coordinatorState ||
                                          ECoordinatorState::ConnectedSwitchingToBackground == _coordinatorState); }

  // Returns the ActiveID of the currently-connected cube from the cubeCommsComponent, or
  // ObservableObject::InvalidActiveID if there is no connected cube
  ActiveID GetConnectedCubeActiveId() const { return _connectedCubeActiveID; }

  // Returns the ActiveObject corresponding to the _connectedCubeActiveID returned from the CubeCommsComponent,
  // or nullptr if there is no such object
  ActiveObject* GetConnectedActiveObject() const { return _connectedActiveObject; }
  
private:
  // Private data structures
  enum class ECoordinatorState{
    UnConnected,
    Connecting,
    ConnectedInteractable,
    ConnectedSwitchingToBackground,
    ConnectedBackground,
    Disconnecting
  };

  struct SubscriberRecord{
    static constexpr float kDontExpire = -1.0f; 
    SubscriberRecord(ICubeConnectionSubscriber* const subscriberPtr,
                     bool connectInBackground = false,
                     float expireIn_s = kDontExpire);

    ICubeConnectionSubscriber* const subscriber;
    bool                       isBackgroundConnection;
    float                      expirationTime_s;

    // Define comparison to enable storage in std::set. Values will be ordered smallest to largest
    // expiration time, with permanent subscriptions stored at the end. Items with expireIn_s == 0
    // will expire on the first update after connection, or the next update if currently connected
    bool operator<(const SubscriberRecord& rhs) const 
    {
      if(this->expirationTime_s == kDontExpire){
        return rhs.expirationTime_s == kDontExpire;
      } else if(rhs.expirationTime_s == kDontExpire){
        return true;
      } else {
        return this->expirationTime_s < rhs.expirationTime_s;
      } 
    }
  };

  using SubscriptionIterator = std::set<SubscriberRecord>::iterator;

  //Private Methods
  void CheckForConnectionLoss(const RobotCompMap& dependentComps);
  void PruneExpiredSubscriptions();

  void TransitionToSwitchingToBackground(const RobotCompMap& dependentComps);
  void CancelSwitchToBackground(const RobotCompMap& dependentComps);
  void TransitionToConnectedInteractable(const RobotCompMap& dependentComps);
  void TransitionToConnectedBackground(const RobotCompMap& dependentComps);

  void RequestConnection(const RobotCompMap& dependentComps);
  void RequestDisconnect(const RobotCompMap& dependentComps);
  void HandleConnectionAttemptResult(const RobotCompMap& dependentComps);

  void SetState(ECoordinatorState newState);

  bool FindRecordBySubscriber(ICubeConnectionSubscriber* const subscriber, SubscriptionIterator& iterator);
  
  void SubscribeToWebViz();
  void SendDataToWebViz();

  // Private Member Vars
  ECoordinatorState _coordinatorState;

  std::set<SubscriberRecord> _subscriptionRecords;
  std::set<ICubeConnectionSubscriber*> _subscribersDumpedByConnectionLoss;
  int _nonBackgroundSubscriberCount;

  bool _connectionAttemptFinished;
  bool _connectionAttemptSucceeded;
  bool _connectionLostUnexpectedly;
  bool _cancelSwitchToBackground;

  float _timeToEndStandby_s;
  float _timeToSwitchToBackground_s;
  float _timeToDisconnect_s;

  ActiveID _connectedCubeActiveID;
  ActiveObject* _connectedActiveObject;

  // for webviz
#if ANKI_DEV_CHEATS
  const CozmoContext* _context = nullptr;
  float _timeToUpdateWebViz_s = 0.0f;
  std::vector<Signal::SmartHandle> _signalHandles;
#endif
  std::string _debugStateString = "Unconnected";
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_CubeConnectionCoordinator_H__
