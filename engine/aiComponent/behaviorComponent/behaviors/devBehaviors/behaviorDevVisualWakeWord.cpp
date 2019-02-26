/**
 * File: behaviorDevVisualWakeWord.cpp
 *
 * Author: Robert Cosgriff
 * Created: 2/15/2019
 *
 * Description: see header
 *
 * Copyright: Anki, Inc. 2019
 *
 **/
#include <unistd.h>

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevVisualWakeWord.h"

// TODO should probably make sure I actually need all of these includes
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/robotDrivenDialog/behaviorPromptUserForVoiceCommand.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

#define SET_STATE(s) do{ \
                          _dVars.state = EState::s; \
                          LOG_WARNING("BehaviorDevVisualWakeWord.State", "State = %s", #s); \
                        } while(0);


namespace Anki {
namespace Vector {

namespace {
  CONSOLE_VAR(u32,  kMaxTimeSinceTrackedFaceUpdated_ms,      "Vision.VisualWakeWord",  500);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinXThres_mm,        "Vision.VisualWakeWord", -25.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxXThres_mm,        "Vision.VisualWakeWord",  20.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinYThres_mm,        "Vision.VisualWakeWord", -100.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxYThres_mm,        "Vision.VisualWakeWord",  100.f);
  CONSOLE_VAR(f32,  kMinTimeBetweenWakeWordTriggers_ms,      "Vision.VisualWakeWord",  5000.f);
  CONSOLE_VAR(bool, kDisableStateMachineToKeyCheckForGaze,   "Vision.VisualWakeWord",  false);
  CONSOLE_VAR(f32,  kGazeStimulationThreshold_ms,            "Vision.VisualWakeWord",  3000.f);
  CONSOLE_VAR(f32,  kGazeBreakThreshold_ms,                  "Vision.VisualWakeWord",  300.f);
  CONSOLE_VAR(f32,  kGazeDecrementMultiplier,                "Vision.VisualWakeWord",  1.2f);
}

namespace {
  static const UserIntentTag affirmativeIntent = USER_INTENT(imperative_affirmative);
  static const UserIntentTag negativeIntent = USER_INTENT(imperative_negative);
  static const UserIntentTag goodRobotIntent = USER_INTENT(imperative_praise);
  static const UserIntentTag badRobotIntent = USER_INTENT(imperative_scold);
}

#define LOG_CHANNEL "Behaviors"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevVisualWakeWord::BehaviorDevVisualWakeWord(const Json::Value& config)
 : ICozmoBehavior(config)
, _iConfig(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(DevTestPromptUser),
                                  BEHAVIOR_CLASS(PromptUserForVoiceCommand),
                                  _iConfig.yeaOrNayBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::BehaviorUpdate()
{
  if( !IsActivated() ) {
    return;
  }
  TransitionToCheckForVisualWakeWord();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = false;

  // This will result in running facial part detection whenever is behavior is an activatable scope
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::High });
  modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::High });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevVisualWakeWord::InstanceConfig::InstanceConfig(const Json::Value& config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevVisualWakeWord::DynamicVariables::DynamicVariables()
: state(EState::CheckingForVisualWakeWord)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.yeaOrNayBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevVisualWakeWord::WantsToBeActivatedBehavior() const
{
  // This is a demo ... so hack central!
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::OnBehaviorActivated()
{
  // TODO not sure if I need this here
  TransitionToCheckForVisualWakeWord();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::TransitionToCheckForVisualWakeWord()
{
  // This check should prevent us from trying to trigger a audio stream
  // when we have already trigger the visual wake word
  if(GetBEI().GetFaceWorld().GetGazeDirectionPose(kMaxTimeSinceTrackedFaceUpdated_ms,
                                                  _dVars.gazeDirectionPose, _dVars.faceIDToTurnBackTo)) {
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToCheckForVisualWakeWord",
                "Got stable gaze direciton, now see if it's at the robot ... this"
                "might be wrapped out side of a behavior ... who fucking knows");
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    Pose3d gazeDirectionPoseWRTRobot;
    if (_dVars.gazeDirectionPose.GetWithRespectTo(robotPose, gazeDirectionPoseWRTRobot)) {

      const auto& translation = gazeDirectionPoseWRTRobot.GetTranslation();
      auto makingEyeContact = GetBEI().GetFaceWorld().IsMakingEyeContact(kMaxTimeSinceTrackedFaceUpdated_ms);

      // Check to see if our gaze points is within constraints to be considered
      // "looking" at vector.
      const bool isWithinXConstraints = ( Util::InRange(translation.x(), kFaceDirectedAtRobotMinXThres_mm,
                                                        kFaceDirectedAtRobotMaxXThres_mm) );
      const bool isWithinYConstarints = ( Util::InRange(translation.y(), kFaceDirectedAtRobotMinYThres_mm,
                                                        kFaceDirectedAtRobotMaxYThres_mm) );
      if ( ( isWithinXConstraints && isWithinYConstarints ) || makingEyeContact ) {
        // TODO there is probably a more accurate way get the actual timestamp of when
        // gaze happened ... because of the "slack" variable pasted into the original
        // get gaze direction pose, but alas this whole state machine is fucking retarded and this
        // isn't the reason why.
        const RobotTimeStamp_t currentTimeStamp = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
        if (_dVars.state == EState::DecreasingGazeStimulation) {
          SET_STATE(IncreasingGazeStimulation);
          _dVars.lastGazeAtRobot = currentTimeStamp;
        } else {
          IncrementGazeStimulation(currentTimeStamp);
        }

        if (_dVars.gazeStimulation > kGazeStimulationThreshold_ms) {
          // Open up audio stream
          LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToCheckForVisualWakeWord.HitGazeStimulationThreshold",
                      "Gaze stimulation is now %.3f and above the threshold %.3f.",
                      _dVars.gazeStimulation, kGazeStimulationThreshold_ms);
          if (_iConfig.yeaOrNayBehavior->WantsToBeActivated()) {
            // This isn't entirely correct but whatever, fuck it, i think it's reduce some bugs so...
            SET_STATE(DetectedVisualWakeWord);
            // Now that we know we are going open up the audio stream clear the history and reset
            // the gazing stim
            GetBEI().GetFaceWorldMutable().ClearGazeDirectionHistory(_dVars.faceIDToTurnBackTo);
            ResetGazeStimulation();
            DelegateIfInControl(_iConfig.yeaOrNayBehavior.get(), &BehaviorDevVisualWakeWord::TransitionToListening);
          }
        }
      } else {
        DecrementStimIfGazeHasBroken();
      }
    } // this case is for whatever reason we fail to get a pose ... not sure how we want to handle this
  } else {
    DecrementStimIfGazeHasBroken();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::TransitionToListening()
{
  SET_STATE(Listening);
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();

  if( uic.IsUserIntentPending(affirmativeIntent) ){
    TransitionToResponding(0);
  } else if ( uic.IsUserIntentPending(negativeIntent) ) {
    TransitionToResponding(1);
  } else if ( uic.IsUserIntentPending(goodRobotIntent) ) {
    TransitionToResponding(2);
  } else if (uic.IsUserIntentPending(badRobotIntent) ) {
    TransitionToResponding(3);
  }
  TransitionToResponding(4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::TransitionToResponding(const int response)
{
  SET_STATE(Responding);
  CompoundActionSequential* reaction = new CompoundActionSequential();
  if (response == 0) {
    SET_STATE(CheckingForVisualWakeWord);
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotAffirmativeResponse", "");

    reaction->AddAction(new TriggerAnimationAction(AnimationTrigger::Feedback_GoodRobot));

    TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo, M_PI);
    turnTowardsFace->SetRequireFaceConfirmation(true);
    reaction->AddAction(turnTowardsFace);

    DelegateIfInControl(reaction, &BehaviorDevVisualWakeWord::TransitionToCheckForVisualWakeWord);
  } else if (response == 1) {
    SET_STATE(CheckingForVisualWakeWord);
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotNegativeResponse", "");
    reaction->AddAction(new TriggerAnimationAction(AnimationTrigger::Feedback_BadRobot));

    TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo, M_PI);
    turnTowardsFace->SetRequireFaceConfirmation(true);
    reaction->AddAction(turnTowardsFace);
    DelegateIfInControl(reaction, &BehaviorDevVisualWakeWord::TransitionToCheckForVisualWakeWord);
  } else if (response == 2) {
    SET_STATE(CheckingForVisualWakeWord);
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotGoodRobotResponse", "");
    reaction->AddAction(new TriggerAnimationAction(AnimationTrigger::Feedback_GoodRobot));

    TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo, M_PI);
    turnTowardsFace->SetRequireFaceConfirmation(true);
    reaction->AddAction(turnTowardsFace);
    DelegateIfInControl(reaction, &BehaviorDevVisualWakeWord::TransitionToCheckForVisualWakeWord);
  } else if (response == 3) {
    SET_STATE(CheckingForVisualWakeWord);
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotBadRobotResponse", "");
    reaction->AddAction(new TriggerAnimationAction(AnimationTrigger::Feedback_BadRobot));

    TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsFaceAction(_dVars.faceIDToTurnBackTo, M_PI);
    turnTowardsFace->SetRequireFaceConfirmation(true);
    reaction->AddAction(turnTowardsFace);
    DelegateIfInControl(reaction, &BehaviorDevVisualWakeWord::TransitionToCheckForVisualWakeWord);
  } else {
    SET_STATE(CheckingForVisualWakeWord);
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotNeitherPositiveOrNegative", "");
  }
}

void BehaviorDevVisualWakeWord::IncrementGazeStimulation(const RobotTimeStamp_t currentTimeStamp) {
  _dVars.gazeStimulation += static_cast<float>(currentTimeStamp - _dVars.lastGazeAtRobot);
  _dVars.lastGazeAtRobot = currentTimeStamp;
  LOG_WARNING("BehaviorDevVisualWakeWord.IncrementGazeStimulation.UpdatedGazeStimulation",
              "Gaze stimulation is now %.3f.", _dVars.gazeStimulation);
}

void BehaviorDevVisualWakeWord::DecrementGazeStimulation(const RobotTimeStamp_t currentTimeStamp) {
  // TODO need a multiplier so this happens faster than incrementing
  _dVars.gazeStimulation -= kGazeDecrementMultiplier * static_cast<float>(
                            currentTimeStamp - _dVars.lastGazeAtRobot);
  if (_dVars.gazeStimulation < 0.f) {
    ResetGazeStimulation();
  }
  LOG_WARNING("BehaviorDevVisualWakeWord.DecrementGazeStimulation.UpdatedGazeStimulation",
              "Gaze stimulation is now %.3f.", _dVars.gazeStimulation);
}

void BehaviorDevVisualWakeWord::ResetGazeStimulation() {
  _dVars.gazeStimulation = 0.f;
  LOG_WARNING("BehaviorDevVisualWakeWord.ResetGazeStimulation.UpdatedGazeStimulation", "");
}

void BehaviorDevVisualWakeWord::DecrementStimIfGazeHasBroken() {
  const RobotTimeStamp_t currentTimeStamp = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  if (currentTimeStamp > _dVars.lastGazeAtRobot + (kGazeStimulationThreshold_ms/1000.f)) {
    SET_STATE(DecreasingGazeStimulation);
    DecrementGazeStimulation(currentTimeStamp);
  }
}

} // namespace Vector
} // namespace Anki
