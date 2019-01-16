/**
 * File: BehaviorReactToTapeBoundary.cpp
 *
 * Author: Matt Michini
 * Created: 2019-01-08
 *
 * Description: Reacts to a boundary made of retro-reflective tape or other surface that dazzles the cliff sensors
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToTapeBoundary.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Boundary.h"
#include "engine/viz/vizManager.h"

#include "coretech/common/engine/math/polygon_impl.h"

#include "util/console/consoleInterface.h"

#define CONSOLE_GROUP "BehaviorReactToTapeBoundary"

namespace Anki {
namespace Vector {
  
namespace {
#ifdef SIMULATOR
  static const uint16_t kCliffBoundaryThreshold = 891;
#else
  static const uint16_t kCliffBoundaryThreshold = 1500;
#endif
  
  // Bit flags for each of the cliff sensors:
  const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
  
  CONSOLE_VAR(float, kForwardSpeed_mmps, CONSOLE_GROUP, 40.f);
  CONSOLE_VAR(float, kDeltaSpeed_mmps, CONSOLE_GROUP, 25.f);
  
  CONSOLE_VAR(float, kTooFarFromBoundaryThreshold_mm, CONSOLE_GROUP, 50.f);
  
  // Thickness of the boundary inserted into the nav map
  const float kBoundaryThickness_mm = 10.f;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToTapeBoundary::BehaviorReactToTapeBoundary(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToTapeBoundary::~BehaviorReactToTapeBoundary()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToTapeBoundary::WantsToBeActivatedBehavior() const
{
  return (GetCliffsDetectingBoundary() != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // record location of boundary here
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const auto& cliffComp = robotInfo.GetCliffSensorComponent();
  const bool result = cliffComp.ComputeCliffPose(static_cast<uint32_t>(robotInfo.GetLastMsgTimestamp()),
                                                 GetCliffsDetectingBoundary(),
                                                 _dVars.initialBoundaryPose);

  if (!result) {
    LOG_WARNING("BehaviorReactToTapeBoundary.OnBehaviorActivated.FailedComputingBoundaryPose", "");
    CancelSelf();
    return;
  }
  
  // Set the "white detect threshold" for use with the CliffAlignToWhiteAction
  CliffSensorComponent::CliffSensorDataArray whiteThresholds;
  whiteThresholds.fill(kCliffBoundaryThreshold);
  cliffComp.SendWhiteDetectThresholdsToRobot(whiteThresholds);
  
  TransitionToInitialReaction();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::OnBehaviorDeactivated()
{
  // Insert some data into the memory map if we have valid endpoints
  if (_dVars.boundaryLeftEndPoint.HasParent() &&
      _dVars.boundaryRightEndPoint.HasParent()) {
    const Point2f boundaryFrom{_dVars.boundaryLeftEndPoint.GetTranslation().x(),
                               _dVars.boundaryLeftEndPoint.GetTranslation().y()};
    
    const Point2f boundaryTo{_dVars.boundaryRightEndPoint.GetTranslation().x(),
                             _dVars.boundaryRightEndPoint.GetTranslation().y()};
    
    MemoryMapData_Boundary boundary(boundaryFrom,
                                    boundaryTo,
                                    GetBEI().GetRobotInfo().GetWorldOrigin(),
                                    kBoundaryThickness_mm,
                                    GetBEI().GetRobotInfo().GetLastMsgTimestamp());
    GetBEI().GetMapComponent().InsertData(Poly2f{boundary._boundaryQuad}, boundary);
  }
  
  auto* vizm = GetBEI().GetRobotInfo().GetContext()->GetVizManager();
  vizm->EraseSegments(GetDebugLabel());
  
  if (_dVars.lineFollowingLeft || _dVars.lineFollowingRight) {
    GetBEI().GetMovementComponent().StopBody();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::BehaviorUpdate()
{
  if (!IsActivated()) {
    return;
  }

  const auto cliffsSeeingBoundary = GetCliffsDetectingBoundary();
  const bool seeingBoundaryFL = cliffsSeeingBoundary & FL;
  const bool seeingBoundaryFR = cliffsSeeingBoundary & FR;
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const auto& robotPose = robotInfo.GetPose();
  
  if (_dVars.recordBoundaryCliffPoses) {
    
    if (!_dVars.boundaryFL.HasParent() && seeingBoundaryFL) {
      const auto& cliffFLPose = CliffSensorComponent::GetCliffSensorPoseWrtRobot(CliffSensor::CLIFF_FL, robotPose);
      _dVars.boundaryFL = cliffFLPose.GetWithRespectToRoot();
      // Also set this as the initial boundaryLeftEndPoint pose
      _dVars.boundaryLeftEndPoint = _dVars.boundaryFL;
    }
    if (!_dVars.boundaryFR.HasParent() && seeingBoundaryFR) {
      const auto& cliffFRPose = CliffSensorComponent::GetCliffSensorPoseWrtRobot(CliffSensor::CLIFF_FR, robotPose);
      _dVars.boundaryFR = cliffFRPose.GetWithRespectToRoot();
      // Also set this as the initial boundaryRightEndPoint pose
      _dVars.boundaryRightEndPoint = _dVars.boundaryFR;
    }
  }
  
  if (_dVars.lineFollowingLeft || _dVars.lineFollowingRight) {
    DEV_ASSERT(_iConfig.doLineFollowing, "BehaviorReactToTapeBoundary.BehaviorUpdate.ShouldNotBeLineFollowing");
    
    float lSpeed = kForwardSpeed_mmps;
    float rSpeed = kForwardSpeed_mmps;
    
    if (_dVars.lineFollowingLeft) {
      if (seeingBoundaryFL) {
        rSpeed -= kDeltaSpeed_mmps;
        lSpeed += kDeltaSpeed_mmps;
      } else {
        rSpeed += kDeltaSpeed_mmps;
        lSpeed -= kDeltaSpeed_mmps;
      }
      
      // While we're following the boundary on the robot's left side, update boundaryRightEndPoint each time we see the
      // boundary. Use the position of the cliff sensor, not the robot itself
      if (seeingBoundaryFL) {
        auto cliffFLPose = CliffSensorComponent::GetCliffSensorPoseWrtRobot(CliffSensor::CLIFF_FL, robotPose);
        cliffFLPose.SetRotation(Rotation3d(M_PI_2_F, Z_AXIS_3D()));
        _dVars.boundaryRightEndPoint = cliffFLPose.GetWithRespectToRoot();
      }
      
      float distFromInitialBoundary = 0.f;
      float distFromLastEndpoint = 0.f;
      ComputeDistanceBetween(robotPose, _dVars.initialBoundaryPose, distFromInitialBoundary);
      ComputeDistanceBetween(robotPose, _dVars.boundaryRightEndPoint, distFromLastEndpoint);
      if (distFromLastEndpoint > kTooFarFromBoundaryThreshold_mm &&
          distFromInitialBoundary > kTooFarFromBoundaryThreshold_mm) {
        _dVars.lineFollowingLeft = false;
        lSpeed = 0.f;
        rSpeed = 0.f;
        TransitionToViewBoundary();
      }
      
    } else if (_dVars.lineFollowingRight) {
      if (seeingBoundaryFR) {
        rSpeed += kDeltaSpeed_mmps;
        lSpeed -= kDeltaSpeed_mmps;
      } else {
        rSpeed -= kDeltaSpeed_mmps;
        lSpeed += kDeltaSpeed_mmps;
      }
      
      // While we're following the boundary on the robot's right side, update boundaryLeftEndPoint each time we see the
      // boundary. Use the position of the cliff sensor, not the robot itself
      if (seeingBoundaryFR) {
        auto cliffFRPose = CliffSensorComponent::GetCliffSensorPoseWrtRobot(CliffSensor::CLIFF_FR, robotPose);
        cliffFRPose.SetRotation(Rotation3d(-M_PI_2_F, Z_AXIS_3D()));
        _dVars.boundaryLeftEndPoint = cliffFRPose.GetWithRespectToRoot();
      }
      
      float distFromInitialBoundary = 0.f;
      float distFromLastEndpoint = 0.f;
      ComputeDistanceBetween(robotPose, _dVars.initialBoundaryPose, distFromInitialBoundary);
      ComputeDistanceBetween(robotPose, _dVars.boundaryLeftEndPoint, distFromLastEndpoint);
      if (distFromLastEndpoint > kTooFarFromBoundaryThreshold_mm &&
          distFromInitialBoundary > kTooFarFromBoundaryThreshold_mm) {
        _dVars.lineFollowingRight = false;
        lSpeed = 0.f;
        rSpeed = 0.f;
        TransitionToTraceBoundaryLeft();
      }
      
    }
    
    GetBEI().GetMovementComponent().DriveWheels(lSpeed, rSpeed, 10000.f, 10000.f);
  }
  
  if (_dVars.boundaryLeftEndPoint.HasParent() &&
      _dVars.boundaryRightEndPoint.HasParent()) {
    // Draw the estimated boundary now that the endpoints exist
    auto* vizm = GetBEI().GetRobotInfo().GetContext()->GetVizManager();
    const bool clearPrevious = true;
    vizm->DrawSegment(GetDebugLabel(),
                      _dVars.boundaryLeftEndPoint.GetTranslation() + Vec3f(0,0,10),
                      _dVars.boundaryRightEndPoint.GetTranslation() + Vec3f(0,0,10),
                      Anki::NamedColors::MAGENTA,
                      clearPrevious);
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t BehaviorReactToTapeBoundary::GetCliffsDetectingBoundary() const
{
  const auto& cliffComp = GetBEI().GetRobotInfo().GetCliffSensorComponent();
  const auto& cliffData = cliffComp.GetCliffDataRaw();
  
  uint8_t cliffsSeeingBoundary = 0;
  for (size_t i=0 ; i < cliffData.size() ; i++) {
    if (cliffData[i] > kCliffBoundaryThreshold) {
      cliffsSeeingBoundary |= (1<<i);
    }
  }
  
  return cliffsSeeingBoundary;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToInitialReaction()
{
  // Play an appropriate animation based on which cliff sensor(s) saw the white stripe
  static const std::map<uint8_t, AnimationTrigger> reactionsAnimMap {
    {(FL | FR), AnimationTrigger::ReactToCliffFront},
    { FL,       AnimationTrigger::ReactToCliffFrontLeft},
    { FR,       AnimationTrigger::ReactToCliffFrontRight},
    { BL,       AnimationTrigger::ReactToCliffBackLeft},
    { BR,       AnimationTrigger::ReactToCliffBackRight},
    {(BL | BR), AnimationTrigger::ReactToCliffBack},
  };
  
  const auto cliffsSeeingBoundary = GetCliffsDetectingBoundary();
  
  const auto it = reactionsAnimMap.find(cliffsSeeingBoundary);
  if (it == reactionsAnimMap.end()) {
    // No anim found.. simply cancel the behavior.
    LOG_WARNING("BehaviorReactToTapeBoundary.TransitionToInitialReaction.NoAnimForCliffs", "");
    CancelSelf();
    return;
  }

  const auto animToPlay = it->second;
  DelegateIfInControl(new TriggerAnimationAction(animToPlay),
                      &BehaviorReactToTapeBoundary::TransitionToApproachBoundary);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToApproachBoundary()
{
  // Enable recording of the cliff sensor poses when they encounter the boundary again
  _dVars.recordBoundaryCliffPoses = true;
  
  auto* action = new CompoundActionParallel{};
  action->SetShouldEndWhenFirstActionCompletes(true);
  action->AddAction(new DriveToPoseAction(_dVars.initialBoundaryPose));
  action->AddAction(new WaitForLambdaAction([this](Robot& robot){
    return GetCliffsDetectingBoundary() & (FL | FR);
  }));
  
  DelegateIfInControl(action,
                      &BehaviorReactToTapeBoundary::TransitionToAlignWithBoundary);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToAlignWithBoundary()
{
  auto* action = new CompoundActionParallel{};
  action->SetShouldEndWhenFirstActionCompletes(true);
  action->AddAction(new CliffAlignToWhiteAction());
  action->AddAction(new WaitForLambdaAction([this](Robot& robot){
    // Wait until we've recorded both poses
    return _dVars.boundaryFL.HasParent() && _dVars.boundaryFR.HasParent();
  }));
  
  DelegateIfInControl(action,
                      &BehaviorReactToTapeBoundary::TransitionToRefineBoundaryPose);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToRefineBoundaryPose()
{
  // Since we're now aligned with the boundary and we've recorded the cliff sensor poses upon
  // encountering it, now 'refine' the initial boundary pose
  _dVars.recordBoundaryCliffPoses = false;
  Vec3f vec;
  ComputeVectorBetween(_dVars.boundaryFL,
                       _dVars.boundaryFR,
                       GetBEI().GetRobotInfo().GetWorldOrigin(),
                       vec);
  float ang = atan2f(vec.y(), vec.x());
  
  // Set the initial boundary pose to be the point in between the two cliff sensor poses
  auto newTrans = _dVars.boundaryFR.GetTranslation();
  newTrans.x() += vec.x() / 2.f;
  newTrans.y() += vec.y() / 2.f;
  newTrans.z() += vec.z() / 2.f;
  
  // Angle should be perpendicular to boundary, pointing 'away'
  const auto& newRot = Rotation3d(ang - M_PI_2_F, Z_AXIS_3D());
  
  _dVars.initialBoundaryPose.SetTranslation(newTrans);
  _dVars.initialBoundaryPose.SetRotation(newRot);
  
  // Also adjust the rotation of the boundaryFR and boundaryFL poses for consistency
  _dVars.boundaryFL.SetRotation(newRot);
  _dVars.boundaryFR.SetRotation(newRot);
  
  // Now play a reaction to seeing the boundary again, this time from the front
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToCliffFront),
                      [this](){
                        if (_iConfig.doLineFollowing) {
                          TransitionToTraceBoundaryRight();
                        } else {
                          TransitionToApproachBoundaryLeft();
                        }
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToTraceBoundaryRight()
{
  // Drive to a pose such that the front right cliff sensor is at boundaryFL, and angled to be able to trace the line
  Pose3d poseToDriveTo(Rotation3d(M_PI_2_F, Z_AXIS_3D()),
                       Vec3f{-kCliffSensorYOffset_mm, 0.f, 0.f},
                       _dVars.boundaryFL);
  
  DelegateIfInControl(new DriveToPoseAction(poseToDriveTo),
                      [this]() {
                        _dVars.lineFollowingRight = true;
                      });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToTraceBoundaryLeft()
{
  // Drive to a pose such that the front left cliff sensor is at the last endpoint, and angled toward the initial boundary pose
  Pose3d poseToDriveTo(Rotation3d(-M_PI_2_F, Z_AXIS_3D()),
                       Vec3f{-kCliffSensorYOffset_mm, 0.f, 0.f},
                       _dVars.boundaryLeftEndPoint);
  
  DelegateIfInControl(new DriveToPoseAction(poseToDriveTo),
                      [this]() {
                        _dVars.lineFollowingLeft = true;
                      });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToViewBoundary()
{
  const float distAwayFromBoundary = 70.f;
  
  // Drive to a pose from which to view the entire boundary
  Pose3d poseToDriveTo(Rotation3d(0.f, Z_AXIS_3D()),
                       Vec3f{-distAwayFromBoundary, 0.f, 0.f},
                       _dVars.initialBoundaryPose);
  
  auto* action = new CompoundActionSequential();
  action->AddAction(new WaitForLambdaAction([this](Robot& robot){
    return !GetBEI().GetMovementComponent().AreWheelsMoving();
  }));
  action->AddAction(new DriveToPoseAction(poseToDriveTo));
  action->AddAction(new TurnTowardsPoseAction(_dVars.boundaryLeftEndPoint));
  action->AddAction(new TurnTowardsPoseAction(_dVars.boundaryRightEndPoint));
  
  DelegateIfInControl(action,
                      [this](){
                        // Done!
                        CancelSelf();
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToApproachBoundaryLeft()
{
  // Drive to a pose to the left of the initial boundary point, and just behind the boundary
  Pose3d poseToDriveTo(Rotation3d(0.f, Z_AXIS_3D()),
                       Vec3f{-30.f, 80.f, 0.f},
                       _dVars.boundaryFL);
  
  auto* action = new CompoundActionSequential();
  action->AddAction(new DriveToPoseAction(poseToDriveTo));
  action->AddAction(new DriveStraightAction(50.f, 30.f));
  
  auto* parallelAction = new CompoundActionParallel();
  parallelAction->SetShouldEndWhenFirstActionCompletes(true);
  parallelAction->AddAction(action);
  parallelAction->AddAction(new WaitForLambdaAction([this](Robot& robot){
    return GetCliffsDetectingBoundary() & (FL | FR);
  }));
  
  DelegateIfInControl(parallelAction,
                      [this]() {
                        const auto cliffsSeeingBoundary = GetCliffsDetectingBoundary();
                        const bool seeingBoundaryFL = cliffsSeeingBoundary & FL;
                        const bool seeingBoundaryFR = cliffsSeeingBoundary & FR;
                        const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
                        // if we are seeing the boundary with at least one of the front cliff sensors,
                        // update boundaryLeftEndPoint.
                        if (seeingBoundaryFL) {
                          const auto& cliffFLPose = CliffSensorComponent::GetCliffSensorPoseWrtRobot(CliffSensor::CLIFF_FL, robotPose);
                          _dVars.boundaryLeftEndPoint = cliffFLPose.GetWithRespectToRoot();
                        } else if (seeingBoundaryFR) {
                          const auto& cliffFRPose = CliffSensorComponent::GetCliffSensorPoseWrtRobot(CliffSensor::CLIFF_FR, robotPose);
                          _dVars.boundaryLeftEndPoint = cliffFRPose.GetWithRespectToRoot();
                        }
                        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToCliffFront),
                                            &BehaviorReactToTapeBoundary::TransitionToApproachBoundaryRight);
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToTapeBoundary::TransitionToApproachBoundaryRight()
{
  // Drive to a pose to the right of the initial boundary point, and just behind the boundary
  Pose3d poseToDriveTo(Rotation3d(0.f, Z_AXIS_3D()),
                       Vec3f{-30.f, -80.f, 0.f},
                       _dVars.boundaryFR);
  
  auto* action = new CompoundActionSequential();
  action->AddAction(new DriveToPoseAction(poseToDriveTo));
  action->AddAction(new DriveStraightAction(50.f, 30.f));
  
  auto* parallelAction = new CompoundActionParallel();
  parallelAction->SetShouldEndWhenFirstActionCompletes(true);
  parallelAction->AddAction(action);
  parallelAction->AddAction(new WaitForLambdaAction([this](Robot& robot){
    return GetCliffsDetectingBoundary() & (FL | FR);
  }));
  
  DelegateIfInControl(parallelAction,
                      [this](){
                        const auto cliffsSeeingBoundary = GetCliffsDetectingBoundary();
                        const bool seeingBoundaryFL = cliffsSeeingBoundary & FL;
                        const bool seeingBoundaryFR = cliffsSeeingBoundary & FR;
                        const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
                        // if we are seeing the boundary with at least one of the front cliff sensors,
                        // update boundaryRightEndPoint.
                        if (seeingBoundaryFR) {
                          const auto& cliffFRPose = CliffSensorComponent::GetCliffSensorPoseWrtRobot(CliffSensor::CLIFF_FR, robotPose);
                          _dVars.boundaryRightEndPoint = cliffFRPose.GetWithRespectToRoot();
                        } else if (seeingBoundaryFL) {
                          const auto& cliffFLPose = CliffSensorComponent::GetCliffSensorPoseWrtRobot(CliffSensor::CLIFF_FL, robotPose);
                          _dVars.boundaryRightEndPoint = cliffFLPose.GetWithRespectToRoot();
                        }
                        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToCliffFront),
                                            &BehaviorReactToTapeBoundary::TransitionToViewBoundary);
                      });
}

}
}
