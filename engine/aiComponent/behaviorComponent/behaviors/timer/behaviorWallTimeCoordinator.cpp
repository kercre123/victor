/**
* File: behaviorWallTimeCoordinator.cpp
*
* Author: Kevin M. Karol
* Created: 2018-06-15
*
* Description: Manage the designed response to a user request for the wall time
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorWallTimeCoordinator.h"

#include "clad/audio/audioSwitchTypes.h"
#include "components/textToSpeech/textToSpeechCoordinator.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorDisplayWallTime.h"
#include "engine/actions/animActions.h"
#include "engine/components/settingsManager.h"
#include "engine/faceWorld.h"
#include "osState/wallTime.h"

#include <iomanip>

namespace Anki {
namespace Vector {
  

namespace{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWallTimeCoordinator::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWallTimeCoordinator::DynamicVariables::DynamicVariables()
{
  utteranceID = kInvalidUtteranceID;
  utteranceState = UtteranceState::Generating;
  isShowingTime = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWallTimeCoordinator::BehaviorWallTimeCoordinator(const Json::Value& config)
: ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWallTimeCoordinator::~BehaviorWallTimeCoordinator()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.iCantDoThatBehavior.get());
  delegates.insert(_iConfig.showWallTime.get());
  delegates.insert(_iConfig.lookAtFaceInFront.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const 
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorWallTimeCoordinator::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::InitBehavior()
{
  auto& behaviorContainer = GetBEI().GetBehaviorContainer();

  _iConfig.iCantDoThatBehavior = behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(SingletonICantDoThat));
  _iConfig.lookAtFaceInFront   = behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(SingletonFindFaceInFrontWallTime));

  behaviorContainer.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(ShowWallTime), 
                                                BEHAVIOR_CLASS(DisplayWallTime),
                                                _iConfig.showWallTime);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::OnBehaviorActivated()
{
  _dVars = DynamicVariables();

  if(WallTime::getInstance()->GetApproximateLocalTime(_dVars.time)){
    StartTTSGeneration();
    // let's look for a face while we're generating TTS
    TransitionToFindFaceInFront();
  }

  // if we failed to start the "find face" behavior, we need to bail
  if(!IsControlDelegated()){
    TransitionToICantDoThat();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if (!_dVars.isShowingTime){
    switch (_dVars.utteranceState)
    {
      case UtteranceState::Ready:
      case UtteranceState::Invalid:
        // cancel look for face and immediately show wall clock once we're ready
        // safe to call when nothing is currently delegated
        CancelDelegates(false);
        TransitionToShowWallTime();
        break;

      default:
        break;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::OnBehaviorDeactivated()
{
  if (kInvalidUtteranceID != _dVars.utteranceID){
    GetBEI().GetTextToSpeechCoordinator().CancelUtterance(_dVars.utteranceID);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::TransitionToICantDoThat()
{
  ANKI_VERIFY(_iConfig.iCantDoThatBehavior->WantsToBeActivated(), 
              "BehaviorWallTimeCoordinator.TransitionToICantDoThat.BehaviorDoesntWantToBeActivated", "");
  DelegateIfInControl(_iConfig.iCantDoThatBehavior.get());
  // annnnnd we're done (behaviorAlwaysDelegates = false)
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::TransitionToFindFaceInFront()
{
  ANKI_VERIFY(_iConfig.lookAtFaceInFront->WantsToBeActivated(),
              "BehaviorWallTimeCoordinator.TransitionToShowWallTime.BehaviorDoesntWantToBeActivated", "");
  DelegateIfInControl(_iConfig.lookAtFaceInFront.get(), [this](){
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::LookAtUserEndearingly));
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::TransitionToShowWallTime()
{
  _dVars.isShowingTime = true;

  auto playUtteranceCallback = [this](){
    // only play TTS if it was generated, else we're fine with just the clock
    if ((kInvalidUtteranceID != _dVars.utteranceID) && (_dVars.utteranceState == UtteranceState::Ready)){
      GetBEI().GetTextToSpeechCoordinator().PlayUtterance(_dVars.utteranceID);
    } else {
      LOG_ERROR("BehaviorWallTimeCoordinator", "Attempted to play time TTS but generation failed");
    }
  };

  _iConfig.showWallTime->SetShowClockCallback(playUtteranceCallback);
  _iConfig.showWallTime->SetOverrideDisplayTime(_dVars.time);

  ANKI_VERIFY(_iConfig.showWallTime->WantsToBeActivated(),
              "BehaviorWallTimeCoordinator.TransitionToShowWallTime.BehaviorDoesntWantToBeActivated", "");
  DelegateIfInControl(_iConfig.showWallTime.get());
  // annnnnd we're done (behaviorAlwaysDelegates = false)
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::StartTTSGeneration()
{
  const auto& settingsManager = GetBEI().GetSettingsManager();
  const bool clockIs24Hour = settingsManager.GetRobotSettingAsBool(external_interface::RobotSetting::clock_24_hour);

  auto textOfTime = GetTTSStringForTime(_dVars.time, clockIs24Hour);

  const UtteranceTriggerType triggerType = UtteranceTriggerType::Manual;
  const AudioTtsProcessingStyle style = AudioTtsProcessingStyle::Default_Processed;

  auto callback = [this](const UtteranceState& utteranceState)
  {
    _dVars.utteranceState = utteranceState;
  };

  _dVars.utteranceID = GetBEI().GetTextToSpeechCoordinator().CreateUtterance(textOfTime, triggerType, style,
                                                                             1.0f, callback);

  // if we failed to create the tts, we need to let our behavior know since the callback is NOT called in this case
  if (kInvalidUtteranceID == _dVars.utteranceID){
    _dVars.utteranceState = UtteranceState::Invalid;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorWallTimeCoordinator::GetTTSStringForTime(struct tm& localTime, const bool clockIs24Hour)
{
  std::stringstream ss;
  if( clockIs24Hour ) {
    ss << std::put_time(&localTime, "%H:%M");
  }
  else {
    ss << std::put_time(&localTime, "%I:%M");
  }

  return ss.str();
}



} // namespace Vector
} // namespace Anki
