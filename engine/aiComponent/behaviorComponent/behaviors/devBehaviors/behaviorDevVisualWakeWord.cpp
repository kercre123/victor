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
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/micDirectionHistory.h"
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
  //CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinXThres_mm,        "Vision.VisualWakeWord", -25.f);
  //CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxXThres_mm,        "Vision.VisualWakeWord",  20.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinYThres_mm,        "Vision.VisualWakeWord", -180.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxYThres_mm,        "Vision.VisualWakeWord",  180.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMinZThres_mm,        "Vision.VisualWakeWord", -100.f);
  CONSOLE_VAR(f32,  kFaceDirectedAtRobotMaxZThres_mm,        "Vision.VisualWakeWord",  400.f); // z is always too hgih. maybe its bc the face poses are too high?
  CONSOLE_VAR(f32,  kMinTimeBetweenWakeWordTriggers_ms,      "Vision.VisualWakeWord",  500.0f);//5000.f);
  CONSOLE_VAR(bool, kDisableStateMachineToKeyCheckForGaze,   "Vision.VisualWakeWord",  false);
  CONSOLE_VAR(f32,  kGazeStimulationThreshold_ms,            "Vision.VisualWakeWord",  200.0f);//3000.f);
  CONSOLE_VAR(f32,  kGazeBreakThreshold_ms,                  "Vision.VisualWakeWord",  300.f);
  CONSOLE_VAR(f32,  kGazeDecrementMultiplier,                "Vision.VisualWakeWord",  1.2f);
  
  CONSOLE_VAR_RANGED(uint32_t, kDelayBeforeVad, "AAA", 1000, -1000, 15000);
  CONSOLE_VAR_RANGED(float, kMicDirectionDiff_deg, "AAA", 20.0f, 0.0f, 90.0f);
  CONSOLE_VAR_RANGED(int, kMicConfidence, "AAA", 3500, 0, 10000);
  CONSOLE_VAR(bool, kGazeAffectsIt, "AAA", false);
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
  
  SubscribeToTags({
                    RobotInterface::RobotToEngineTag::vadActivity,
                  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::BehaviorUpdate()
{
  if( !IsActivated() ) {
    return;
  }
  
//  const MicDirectionHistory& history = GetBEI().GetMicComponent().GetMicDirectionHistory();
//  _dVars.direction = history.GetRecentDirection();
  
  if( !IsControlDelegated() && _dVars.state != EState::Listening && _dVars.state != EState::Responding ) {
    TransitionToCheckForVisualWakeWord();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;

  // This will result in running facial part detection whenever is behavior is an activatable scope
  modifiers.visionModesForActiveScope->insert({ VisionMode::Faces_Smile, EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::Faces, EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::Faces_Gaze, EVisionUpdateFrequency::High });
  modifiers.visionModesForActivatableScope->insert({ VisionMode::Faces_Smile, EVisionUpdateFrequency::High });
  modifiers.visionModesForActivatableScope->insert({ VisionMode::Faces_Gaze, EVisionUpdateFrequency::High });
  modifiers.visionModesForActivatableScope->insert({ VisionMode::Faces, EVisionUpdateFrequency::High });
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
  lastGazeAtRobot = 0;
  lastVadActivation = 0;
  gazeTime = 0;
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
//  TransitionToCheckForVisualWakeWord();
  
  MicDirectionHistory& micHistory = GetBEI().GetMicComponent().GetMicDirectionHistory();
  micHistory.RegisterSoundReactor( [&]( double power, MicDirectionConfidence confidence, MicDirectionIndex direction ){
    if( confidence >= kMicConfidence && direction < 12 ) {
      _dVars.hasConfidentDirection = true;
      _dVars.direction = direction;
      _dVars.confidentDirectionTime = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
      return true;
    } else {
      _dVars.hasConfidentDirection = false;
      return false;
    }
  } );
  // todo: unregister based on return value
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::TransitionToCheckForVisualWakeWord()
{
  const RobotTimeStamp_t currentTimeStamp = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  
  // TODO there is probably a more accurate way get the actual timestamp of when
  // gaze happened ... because of the "slack" variable pasted into the original
  // get gaze direction pose, but alas this whole state machine is fucking retarded and this
  // isn't the reason why.
  
  bool incrementedGaze = false;
  auto incrementGaze = [&]() {
    // run once
    if( incrementedGaze ) {
      return;
    }
    incrementedGaze = true;
    if (_dVars.state == EState::DecreasingGazeStimulation) {
      SET_STATE(IncreasingGazeStimulation);
      _dVars.lastGazeAtRobot = currentTimeStamp;
    } else {
      IncrementGazeStimulation(currentTimeStamp);
    }
    
    if (_dVars.gazeStimulation > kGazeStimulationThreshold_ms) {
      _dVars.gazeTime = currentTimeStamp;
    }
  };
  
  auto makingEyeContact = GetBEI().GetFaceWorld().IsMakingEyeContact(kMaxTimeSinceTrackedFaceUpdated_ms);
  // todo: out param for IsMakingEyeContact with a _dVars.faceIDToTurnBackTo
  if( makingEyeContact && _dVars.faceIDToTurnBackTo.IsValid() ) {
    incrementGaze();
    PRINT_NAMED_WARNING("WHATNOW", "eye contact!");
  } else if( GetBEI().GetFaceWorld().GetGazeDirectionPose(kMaxTimeSinceTrackedFaceUpdated_ms,
                                                  _dVars.gazeDirectionPose, _dVars.faceIDToTurnBackTo)) {
    LOG_WARNING("BehaviorDevVisualWakeWord.TransitionToCheckForVisualWakeWord",
                "Got stable gaze direciton, now see if it's at the robot ... this"
                "might be wrapped out side of a behavior ... who fucking knows");
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    Pose3d gazeDirectionPoseWRTRobot;
    if (_dVars.gazeDirectionPose.GetWithRespectTo(robotPose, gazeDirectionPoseWRTRobot)) {

      const auto& translation = gazeDirectionPoseWRTRobot.GetTranslation();
      

      // Check to see if our gaze points is within constraints to be considered
      // "looking" at vector.
      const bool isWithinZConstraints = ( Util::InRange(translation.x(), kFaceDirectedAtRobotMinZThres_mm,
                                                        kFaceDirectedAtRobotMaxZThres_mm) );
      const bool isWithinYConstarints = ( Util::InRange(translation.y(), kFaceDirectedAtRobotMinYThres_mm,
                                                        kFaceDirectedAtRobotMaxYThres_mm) );

      if ( ( isWithinZConstraints && isWithinYConstarints ) && kGazeAffectsIt ) {
        
        incrementGaze();
      } else if( kGazeAffectsIt ) {
        DecrementStimIfGazeHasBroken();
      }
      PRINT_NAMED_WARNING("WHATNOW", "withinZ=%d (%f), withinY=%d (%f)", isWithinZConstraints, translation.z(), isWithinYConstarints, translation.y());
    } // this case is for whatever reason we fail to get a pose ... not sure how we want to handle this
    else {
      PRINT_NAMED_WARNING("WHATNOW", "no pose wrt robot");
    }
  } else {
    PRINT_NAMED_WARNING("WHATNOW", "no gaze pose");
    DecrementStimIfGazeHasBroken();
  }
  
  // todo: dont require gaze to work once. or maybe get the faceID from eye contact?
  if( _dVars.faceIDToTurnBackTo.IsValid() ) {
    bool recentActivation = (_dVars.gazeTime != 0) && abs((int64)(TimeStamp_t)_dVars.lastVadActivation-(int64)(TimeStamp_t)_dVars.gazeTime) <= kDelayBeforeVad;
    recentActivation &= (currentTimeStamp <= 5000 + std::max((TimeStamp_t)_dVars.lastVadActivation, (TimeStamp_t)_dVars.gazeTime));
    recentActivation &= std::max(abs((int64)(TimeStamp_t)_dVars.lastVadActivation - (int64)(TimeStamp_t)_dVars.confidentDirectionTime), abs((int64)(TimeStamp_t)_dVars.gazeTime - (int64)(TimeStamp_t)_dVars.confidentDirectionTime)) < 5000;
    Pose3d facePose;
    bool hasFace = GetFacePose( facePose );
    const float faceAngle = atan2f(facePose.GetTranslation().y(), facePose.GetTranslation().x());
    ANKI_VERIFY(!hasFace || fabsf(faceAngle)<M_PI_F/2, "WHATNOW", "unexpected face angle %f", faceAngle);
    //[3,2,1,0] U [10 9 8]
    float micAngle=0.0f;
    bool directionMatches = false;
    if( _dVars.direction <= 3 ) {
      // map from [0, 3] to [0.0, -pi/2]
      micAngle = -(M_PI_F*_dVars.direction)/6;
      directionMatches = fabsf(faceAngle - micAngle) <= kMicDirectionDiff_deg*M_PI_F/180;
    } else if(_dVars.direction>=8 && _dVars.direction <= 11){
      // map from 12-[9,11] to [3*pi/6, 1*pi/6]
      micAngle = (12 - _dVars.direction)*M_PI_F/6.0f;
      directionMatches = fabsf(faceAngle - micAngle) <= kMicDirectionDiff_deg*M_PI_F/180;
    }
    
    // [0,11] mapped to [-pi, -pi] is [0, -pi] U [pi, 0] is
    PRINT_NAMED_WARNING("HERENOW", "recentActivation=%d (%d,%d,%d [%lld]), directions %d x=%1.3f y=%1.3f th=%f mic=%d=>%f==%d gaze=%1.1f",
                        recentActivation, (TimeStamp_t)_dVars.lastVadActivation, (TimeStamp_t)_dVars.gazeTime, (TimeStamp_t)_dVars.confidentDirectionTime,
                        (int64)(TimeStamp_t)_dVars.lastVadActivation-(int64)(TimeStamp_t)_dVars.gazeTime,
                        hasFace, facePose.GetTranslation().x(), facePose.GetTranslation().y(), faceAngle, _dVars.direction, micAngle, directionMatches, _dVars.gazeStimulation );
    
    
    if( hasFace && recentActivation && directionMatches ) {
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevVisualWakeWord::HandleWhileActivated(const RobotToEngineEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case RobotInterface::RobotToEngineTag::vadActivity:
    {
      const bool active = event.GetData().Get_vadActivity().value;
      const RobotTimeStamp_t timestamp = event.GetData().Get_vadActivity().robotTimestamp;
      if( active ) {
        _dVars.lastVadActivation = timestamp;
      }
      _dVars.vadActive = active;
    }
      break;
      
    default:
      break;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevVisualWakeWord::GetFacePose(Pose3d& pose) const
{
  const auto* face = GetBEI().GetFaceWorld().GetFace( _dVars.faceIDToTurnBackTo );
  if( face != nullptr ) {
    const RobotTimeStamp_t ts = face->GetTimeStamp();
    const RobotTimeStamp_t currentTimeStamp = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
    if( currentTimeStamp - ts <= 10000 ) {
      const auto& facePose = face->GetHeadPose();
      const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
      const bool success = facePose.GetWithRespectTo(robotPose, pose);
      return success;
    } else {
      // too old
      return false;
    }
  }
  return false;
}

} // namespace Vector
} // namespace Anki
