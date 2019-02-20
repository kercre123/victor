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
}

namespace {
  static const UserIntentTag affirmativeIntent = USER_INTENT(imperative_affirmative);
  static const UserIntentTag negativeIntent = USER_INTENT(imperative_negative);
  static const UserIntentTag goodRobotIntent = USER_INTENT(imperative_praise);
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
  if (_dVars.state == EState::CheckingForVisualWakeWord) {
    if(GetBEI().GetFaceWorld().GetGazeDirectionPose(kMaxTimeSinceTrackedFaceUpdated_ms,
                                                    _dVars.gazeDirectionPose, _dVars.faceIDToTurnBackTo)) {
      LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToCheckForVisualWakeWord",
                  "Got stable gaze direciton, now see if it's at the robot ... this"
                  "might be wrapped out side of a behavior ... who fucking knows");
      const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
      Pose3d gazeDirectionPoseWRTRobot;
      if (_dVars.gazeDirectionPose.GetWithRespectTo(robotPose, gazeDirectionPoseWRTRobot)) {

        // Now that we know we are going to turn clear the history
        GetBEI().GetFaceWorldMutable().ClearGazeDirectionHistory(_dVars.faceIDToTurnBackTo);

        const auto& translation = gazeDirectionPoseWRTRobot.GetTranslation();
        auto makingEyeContact = GetBEI().GetFaceWorld().IsMakingEyeContact(kMaxTimeSinceTrackedFaceUpdated_ms);

        // Check to see if our gaze points is within constraints to be considered
        // "looking" at vector.
        const bool isWithinXConstraints = ( Util::InRange(translation.x(), kFaceDirectedAtRobotMinXThres_mm,
                                                          kFaceDirectedAtRobotMaxXThres_mm) );
        const bool isWithinYConstarints = ( Util::InRange(translation.y(), kFaceDirectedAtRobotMinYThres_mm,
                                                          kFaceDirectedAtRobotMaxYThres_mm) );
        if ( ( isWithinXConstraints && isWithinYConstarints ) || makingEyeContact ) {
        // Open up audio stream
        SET_STATE(DetectedVisualWakeWord);
          if (_iConfig.yeaOrNayBehavior->WantsToBeActivated()) {
            DelegateIfInControl(_iConfig.yeaOrNayBehavior.get(), &BehaviorDevVisualWakeWord::TransitionToListening);
          }
        }
      }
    }
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
  }
  TransitionToResponding(3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::TransitionToResponding(const int response)
{
  SET_STATE(Responding);
  if (response == 0) {
    SET_STATE(CheckingForVisualWakeWord);
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotAffirmativeResponse", "");
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::Feedback_GoodRobot),
                        &BehaviorDevVisualWakeWord::TransitionToCheckForVisualWakeWord);
  } else if (response == 1) {
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotNegativeResponse", "");
  } else if (response == 2) {
    SET_STATE(CheckingForVisualWakeWord);
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotGoodRobotResponse", "");
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::Feedback_GoodRobot),
                        &BehaviorDevVisualWakeWord::TransitionToCheckForVisualWakeWord);
  } else {
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToResponding.GotNeitherPositiveOrNegative", "");
  }
  // This should allow us to loop again
  SET_STATE(CheckingForVisualWakeWord);
}

} // namespace Vector
} // namespace Anki
