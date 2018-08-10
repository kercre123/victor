/**
 * File: cubeInteractionTracker.h
 *
 * Author: Sam Russell
 * Created: 2018-7-17
 *
 * Description: Robot component which monitors cube data and attempts to discern when a user is physically
 *              interacting with a cube.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_CubeInteractionTracker_H__
#define __Engine_Components_CubeInteractionTracker_H__

#include "coretech/common/engine/math/pose.h"
#include "engine/robotComponents_fwd.h"
#include "engine/components/cubes/iCubeConnectionSubscriber.h"
#include "engine/components/visionScheduleMediator/iVisionModeSubscriber.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki{

// Forward Declarations
class ObjectID;

namespace Vector{

// Forward Declarations
class ActiveObject;
template <typename Type>
class AnkiEvent;
class BehaviorKeepaway;
class BlockWorldFilter;
class BlockWorld;
class MessageEngineToGame;
class ObservableObject;
class Robot;

namespace ExternalInterface {
  class MessageEngineToGame;
}
// ^ Forward Declarations ^

struct TargetStatus {
  TargetStatus();
  const  ActiveObject* activeObject; // valid only if connected
  const  ObservableObject* observableObject; // valid only if located in blockWorld
  Pose3d prevPose; // Pose on last update
  float  distance_mm;
  float  angleFromRobotFwd_deg;
  float  lastObservedTime_s;
  float  lastMovedTime_s;
  float  lastMovedFarTime_s;
  float  lastHeldTime_s;
  float  lastTappedTime_s;
  float  probabilityIsHeld;
  bool   distMeasuredWithProx;
  bool   visibleThisFrame;
  bool   visibleRecently; // true if our current target was seen less than kRecentlyVisibleLimit_s ago
  bool   observedSinceMovedByRobot;
  bool   movedThisFrame;
  bool   movedRecently; // true if our current target moved less than kRecentlyMovedLimit_s ago
  bool   movedFarThisFrame;
  bool   movedFarRecently;
  bool   isHeld;
  bool   tappedDuringLastTick;
};

class CubeInteractionTracker : public IDependencyManagedComponent<RobotComponentID>,
                               public ICubeConnectionSubscriber,
                               public IVisionModeSubscriber,
                               public Anki::Util::noncopyable
{
public:
  CubeInteractionTracker();
  ~CubeInteractionTracker() = default;

  // Dependency Managed Component Methods
  virtual void GetInitDependencies(RobotCompIDSet& dependencies ) const override;
  virtual void InitDependent( Robot* robot, const RobotCompMap& dependentComps ) override;
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies ) const override;
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  // Dependency Managed Component Methods

  // CubeConnectionSubscriber Methods
  virtual std::string GetCubeConnectionDebugName() const override { return "CubeInteractionTracker"; }
  virtual void ConnectionLostCallback() override {};
  // CubeConnectionSubscriber Methods

  bool IsUserHoldingCube() const;
  const TargetStatus GetTargetStatus(){ return _targetStatus; }
  // returns uninitialized ObjectID if we don't have a target
  ObjectID GetTargetID(); 

private:

  enum class ECITState{
    Idle,
    TrackingConnected,
    TrackingUnconnected
  };

  void TransitionToIdle(const RobotCompMap& dependentComps);
  void TransitionToTrackingConnected(const RobotCompMap& dependentComps);
  void TransitionToTrackingUnconnected(const RobotCompMap& dependentComps);

  // Target Tracking State and Methods - - - - -
  void UpdateIsHeldEstimate(const RobotCompMap& dependentComps);
  void UpdateTargetStatus(const RobotCompMap& dependentComps);
  bool SelectTarget(const RobotCompMap& dependentComps);
  void UpdateTargetVisibility(const RobotCompMap& dependentComps);
  void UpdateTargetMotion(const RobotCompMap& dependentComps);
  void UpdateTargetDistance(const RobotCompMap& dependentComps);
  void UpdateTargetAngleFromRobotFwd(const RobotCompMap& dependentComps);
  // Target Tracking State and Methods - - - - - 

  void HandleEngineEvents(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);

  void SendDataToWebViz();

  ECITState _trackerState;

  // Handles for grabbing EngineToGame messages
  std::vector<Signal::SmartHandle> _signalHandles;

  Robot* _robot = nullptr;
  std::unique_ptr<BlockWorldFilter> _targetFilter;

  TargetStatus  _targetStatus;
  // TODO:(str) Make this work for multiple visible targets (dev environment/future proofing)
  // std::vector<TargetStatus> _targets;

  float _currentTimeThisTick_s;
  bool  _tapDetectedDuringTick;
  bool  _trackingAtHighRate;

  // Filter parameter vars, these vary based on ECITState 
  float _filterIncrement;
  float _filterDecrement;

// For WebViz
  float _timeToUpdateWebViz_s;
  std::string _debugStateString;
};

} //namespace Vector
} //namespace Anki

#endif // __Engine_Components_CubeInteractionTracker_H__

