/**
 * File: behaviorReactToGazeDirection.cpp
 *
 * Author: Robert Cosgriff
 * Created: 2018-09-04
 *
 * Description: see header
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
#include <unistd.h>

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorReactToGazeDirection.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorSearchWithinBoundingBox.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSalientPointDetected.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {

namespace {
  // This used to be -200 just a note
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinXThres_mm,        "Vision.GazeDirection", -25.f);
  // This used to 50 just a note
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxXThres_mm,        "Vision.GazeDirection",  20.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinYThres_mm,        "Vision.GazeDirection", -100.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxYThres_mm,        "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(bool, kTurnBackToFace,                         "Vision.GazeDirection",  false);
  CONSOLE_VAR(f32,  kTurnWaitAfterFinalTurn_s,               "Vision.GazeDirection",  1.f);
  CONSOLE_VAR(f32,  kTurnWaitAfterInitialTurn_s,             "Vision.GazeDirection",  1.f);
  CONSOLE_VAR(f32,  kTurnWaitAfterInitialLookBackAtFace_s,   "Vision.GazeDirection",  2.f);
  CONSOLE_VAR(f32,  kSleepTimeAfterActionCompleted_s,        "Vision.GazeDirection",  2.f);
  CONSOLE_VAR(f32,  kMaxPanSpeed_radPerSec,                  "Vision.GazeDirection",  MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);
  CONSOLE_VAR(f32,  kMaxPanAccel_radPerSec2,                 "Vision.GazeDirection",  10.f);
  CONSOLE_VAR(bool, kUseEyeContact,                          "Vision.GazeDirection",  true);
  CONSOLE_VAR(f32,  kDistanceForAboveHorizonSearch_mm2,      "Vision.GazeDirection",  0.f);
  CONSOLE_VAR(f32,  kConeFor180TurnForFaceSearch_deg,        "Vision.GazeDirection",  40.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnRightAngle_deg,        "Vision.GazeDirection",  -90.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnLeftAngle_deg,         "Vision.GazeDirection",  90.f);
  CONSOLE_VAR(f32,  kSearchForFaceTurnAroundAngle_deg,       "Vision.GazeDirection",  180.f);
  CONSOLE_VAR(bool, kSearchForFaceUseThreeTurns,             "Vision.GazeDirection",  false);
  CONSOLE_VAR(f32,  kConeFor135TurnForFaceSearch_deg,        "Vision.GazeDirection",  60.f);
  CONSOLE_VAR(f32,  kSearchForFaceThirdAngle_deg,            "Vision.GazeDirection",  135.f);
  CONSOLE_VAR(s32,  kSearchForFaceNumberOfImagesToWait,      "Vision.GazeDirection",  5);
  CONSOLE_VAR(bool, kFindSurfacePointsUsingFaceDirection,    "Vision.GazeDirection",  false);
  CONSOLE_VAR(bool, kFindFacesUsingFaceDirection,            "Vision.GazeDirection",  true);
  CONSOLE_VAR(bool, kUseExistingFacesWhenSearchingForFaces,  "Vision.GazeDirection",  false);
  CONSOLE_VAR(s32,  kNumberOfTurnsForSurfacePoint,           "Vision.GazeDirection",  1);
}

#define LOG_CHANNEL "Behaviors"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::BehaviorReactToGazeDirection(const Json::Value& config)
 : ICozmoBehavior(config)
, _iConfig(new InstanceConfig(config))
{
  _iConfig->coolDown_sec = 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::InitBehavior()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::InstanceConfig::InstanceConfig(const Json::Value& config)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToGazeDirection::DynamicVariables::DynamicVariables()
{
  lastReactionTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToGazeDirection::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::BehaviorUpdate()
{
  if ( ! IsActivated() ) {
    return;
  }
  TransitionToCheckFaceDirection();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToGazeDirection::CheckIfShouldStop()
{
  // It might take a couple of frames to get a stability reading so set a timer here if we
  // need to or something so we don't just wait forever
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToCheckFaceDirection()
{
  Pose3d faceFocusPose;
  // SmartFaceID faceID;
  if(GetBEI().GetFaceWorld().GetGazeDirectionPose(500, faceFocusPose, _dVars.faceIDToTurnBackTo)) {
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    Pose3d faceFocusPoseWRTRobot;

    if (faceFocusPose.GetWithRespectTo(robotPose, faceFocusPoseWRTRobot)) {
      auto translation = faceFocusPoseWRTRobot.GetTranslation();
      LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.TranslationWRTRobot",
               "x: %.3f, y:%.3f, z:%.3f", translation.x(), translation.y(), translation.z());
      auto makingEyeContact = GetBEI().GetFaceWorld().IsMakingEyeContact(500);

      GetBEI().GetFaceWorldMutable().ClearGazeDirectionHistory(_dVars.faceIDToTurnBackTo);
      
      if ((translation.LengthSq() > kDistanceForAboveHorizonSearch_mm2) && kFindFacesUsingFaceDirection) {
        LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.GazeFarEnoughToLookUp", "");
        LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.DistanceFromRobot",
                 "distance: %.3f", translation.LengthSq());

        // Compute angle
        // If angle is within the turn around cone then turn around and look for face
        Radians turnAngle;
        Radians gazeAngle = atan2f(translation.y(), translation.x());
        LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.GazeAngle",
                 "angle: %.3f", RAD_TO_DEG(gazeAngle.ToFloat()));
        auto angleDifference = Radians(DEG_TO_RAD(180)) - gazeAngle;
        LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.AngleDifference",
                 "angle: %.3f", RAD_TO_DEG(angleDifference.ToFloat()));
        if ( (angleDifference <= Radians(DEG_TO_RAD(kConeFor180TurnForFaceSearch_deg/2.f))) &&
             (angleDifference >= -Radians(DEG_TO_RAD(kConeFor180TurnForFaceSearch_deg/2.f))) ) {
          if (angleDifference < 0) {
            turnAngle = DEG_TO_RAD(-kSearchForFaceTurnAroundAngle_deg);
          } else {
            turnAngle = DEG_TO_RAD(kSearchForFaceTurnAroundAngle_deg);
          }
        } else if ( (angleDifference <= Radians(DEG_TO_RAD(kConeFor135TurnForFaceSearch_deg/2.f))) &&
                    (angleDifference >= -Radians(DEG_TO_RAD(kConeFor135TurnForFaceSearch_deg/2.f))) &&
                    kSearchForFaceUseThreeTurns) {
          if (angleDifference < 0) {
            turnAngle = DEG_TO_RAD(-kSearchForFaceThirdAngle_deg);
          } else {
            turnAngle = DEG_TO_RAD(kSearchForFaceThirdAngle_deg);
          }
        } else if (translation.y() < 0) {
          turnAngle = DEG_TO_RAD(kSearchForFaceTurnRightAngle_deg);
        } else {
          turnAngle = DEG_TO_RAD(kSearchForFaceTurnLeftAngle_deg);
        }
        LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.TurnAngle",
                 "angle: %.3f", RAD_TO_DEG(turnAngle.ToFloat()));

        SmartFaceID faceToTurnTowards;
        if (GetBEI().GetFaceWorld().FaceInTurnAngle(Radians(turnAngle), _dVars.faceIDToTurnBackTo, robotPose, faceToTurnTowards)
            && kUseExistingFacesWhenSearchingForFaces) {
          LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.FoundAnExistingFaceGoingToTurnTowardsThat", "");
          if (turnAngle > 0) {
            CompoundActionSequential* turnAction = new CompoundActionSequential();
            turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesGetInLeft));

            TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(faceToTurnTowards, M_PI);
            turnTowardsFace->SetRequireFaceConfirmation(true);
            CompoundActionParallel* turnAndAnimate = new CompoundActionParallel({
              turnTowardsFace,
              new TriggerLiftSafeAnimationAction(AnimationTrigger::GazingLookAtFacesTurnLeft)
            });
            turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
            turnAction->AddAction(turnAndAnimate);

            DelegateIfInControl(turnAction, &BehaviorReactToGazeDirection::FoundNewFace);

          } else {
            CompoundActionSequential* turnAction = new CompoundActionSequential();
            turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesGetInRight));

            TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(faceToTurnTowards, M_PI);
            turnTowardsFace->SetRequireFaceConfirmation(true);
            CompoundActionParallel* turnAndAnimate = new CompoundActionParallel({
              turnTowardsFace,
              new TriggerLiftSafeAnimationAction(AnimationTrigger::GazingLookAtFacesTurnRight)
            });
            turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
            turnAction->AddAction(turnAndAnimate);

            DelegateIfInControl(turnTowardsFace, &BehaviorReactToGazeDirection::FoundNewFace);

          }

        } else {

          if (turnAngle > 0) {
            CompoundActionSequential* turnAction = new CompoundActionSequential();
            turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesGetInLeft));

            CompoundActionParallel* turnAndAnimate = new CompoundActionParallel({
              new TurnInPlaceAction(turnAngle.ToFloat(), false),
              new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtFacesTurnLeft}
            });
            turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
            turnAction->AddAction(turnAndAnimate);
            turnAction->AddAction(new WaitForImagesAction(kSearchForFaceNumberOfImagesToWait, VisionMode::DetectingFaces));
            turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));
            // TODO is this how we set it to find a new face? seems like the face it would try to find
            // is the last face it saw ... how is that not the face it was looking at last ... maybe
            // we need to add a wait
            // There is a wait in TurnTowardsFaceAction and i can change the max frames to wait
            TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(SmartFaceID(), M_PI_2);
            turnTowardsFace->SetRequireFaceConfirmation(true);
            turnAction->AddAction(turnTowardsFace);
            DelegateIfInControl(turnAction, &BehaviorReactToGazeDirection::FoundNewFace);

          } else {
            CompoundActionSequential* turnAction = new CompoundActionSequential();
            turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtFacesGetInRight));

            CompoundActionParallel* turnAndAnimate = new CompoundActionParallel({
              new TurnInPlaceAction(turnAngle.ToFloat(), false),
              new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtFacesTurnRight}
            });
            turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
            turnAction->AddAction(turnAndAnimate);
            turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));
            turnAction->AddAction(new WaitForImagesAction(kSearchForFaceNumberOfImagesToWait, VisionMode::DetectingFaces));
            // TODO is this how we set it to find a new face? seems like the face it would try to find
            // is the last face it saw ... how is that not the face it was looking at last ... maybe
            // we need to add a wait
            // There is a wait in TurnTowardsFaceAction and i can change the max frames to wait
            TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(SmartFaceID(), M_PI_2);
            turnTowardsFace->SetRequireFaceConfirmation(true);
            turnAction->AddAction(turnTowardsFace);
            DelegateIfInControl(turnAction, &BehaviorReactToGazeDirection::FoundNewFace);

          }
        }

      } else if (kFindSurfacePointsUsingFaceDirection) {

        if ( ( ( FLT_GT(translation.x(), kFaceDirectedAtRobotMinXThres_mm) &&
                 FLT_LT(translation.x(), kFaceDirectedAtRobotMaxXThres_mm) ) &&
             (FLT_GT(translation.y(), kFaceDirectedAtRobotMinYThres_mm) &&
              FLT_LT(translation.y(), kFaceDirectedAtRobotMaxYThres_mm)) )
              || ( makingEyeContact && kUseEyeContact) ) {

          CompoundActionSequential* turnAction = new CompoundActionSequential();
          turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtVectorGetIn));
          turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtVectorReaction));
          turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
          turnAction->AddAction(new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE)));

          turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
          DelegateIfInControl(turnAction, &BehaviorReactToGazeDirection::TransitionToCompleted);

        } else {

          if ( FLT_LT(translation.y(), 0.f) ) {
            LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.LookingLeftTest", "");
            faceFocusPoseWRTRobot.Print("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection", "FaceFocusPoseWRTRobot");
            CompoundActionSequential* turnAction = new CompoundActionSequential();
            for (int i = 0; i < kNumberOfTurnsForSurfacePoint; ++i) {
              turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInRight));

              // Do an initial turn if we want to
              if (kTurnBackToFace) {
                turnAction->AddAction(new TurnTowardsPoseAction(faceFocusPoseWRTRobot));
                turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialTurn_s));
                turnAction->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));
                turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialLookBackAtFace_s));
              }

              // TODO do we want this to be the same point as before
              TurnTowardsPoseAction* turnTowardsPose = new TurnTowardsPoseAction(faceFocusPoseWRTRobot);
              turnTowardsPose->SetMaxPanSpeed(kMaxPanSpeed_radPerSec);
              turnTowardsPose->SetPanAccel(kMaxPanAccel_radPerSec2);
              CompoundActionParallel* turnAndAnimate = new CompoundActionParallel({
                turnTowardsPose,
                new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfacesTurnRight}
              });
              turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
              turnAction->AddAction(turnAndAnimate);
              turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceReaction));
              turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
              turnAction->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));

              turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
            }
            DelegateIfInControl(turnAction, &BehaviorReactToGazeDirection::TransitionToCompleted);
          } else {

            LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.LookingRightTest", "");
            faceFocusPoseWRTRobot.Print("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection", "FaceFocusPoseWRTRobot");
            CompoundActionSequential* turnAction = new CompoundActionSequential();
            for (int i = 0; i < kNumberOfTurnsForSurfacePoint; ++i) {
              turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfacesGetInLeft));

              // Do an initial turn if we want to
              if (kTurnBackToFace) {
                turnAction->AddAction(new TurnTowardsPoseAction(faceFocusPoseWRTRobot));
                turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialTurn_s));
                turnAction->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));
                turnAction->AddAction(new WaitAction(kTurnWaitAfterInitialLookBackAtFace_s));
              }

              // TODO do we want this to be the same point as before
              TurnTowardsPoseAction* turnTowardsPose = new TurnTowardsPoseAction(faceFocusPoseWRTRobot);
              turnTowardsPose->SetMaxPanSpeed(kMaxPanSpeed_radPerSec);
              turnTowardsPose->SetPanAccel(kMaxPanAccel_radPerSec2);
              CompoundActionParallel* turnAndAnimate = new CompoundActionParallel({
                turnTowardsPose,
                new ReselectingLoopAnimationAction{AnimationTrigger::GazingLookAtSurfaceTurnLeft}
              });
              turnAndAnimate->SetShouldEndWhenFirstActionCompletes(true);
              turnAction->AddAction(turnAndAnimate);

              turnAction->AddAction(new TriggerAnimationAction(AnimationTrigger::GazingLookAtSurfaceReaction));
              turnAction->AddAction(new WaitAction(kTurnWaitAfterFinalTurn_s));
              turnAction->AddAction(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo));

              turnAction->AddAction(new WaitAction(kSleepTimeAfterActionCompleted_s));
            }
            DelegateIfInControl(turnAction, &BehaviorReactToGazeDirection::TransitionToCompleted);
          }
        }
      }
    } else {
      LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.GetWithRespectToFailed", "");
    }
    LOG_WARNING("BehaviorReactToGazeDirection.TransitionToCheckFaceDirection.FaceNotFocused", "");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::TransitionToCompleted()
{
}

void BehaviorReactToGazeDirection::FoundNewFace(ActionResult result)
{
  if (ActionResult::NO_FACE == result) {
    LOG_WARNING("BehaviorReactToGazeDirection.FoundNewFace.Result", "No Face %d", result);
    DelegateIfInControl(new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo), &BehaviorReactToGazeDirection::TransitionToCompleted);
  } else if (ActionResult::SUCCESS == result) {
    LOG_WARNING("BehaviorReactToGazeDirection.FoundNewFace.Result", "Success %d", result);
    BehaviorReactToGazeDirection::TransitionToCompleted();
  } else {
    LOG_WARNING("BehaviorReactToGazeDirection.FoundNewFace.Result", "Other: %d", result);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToGazeDirection::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = false;

  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::High });
}

}
}
