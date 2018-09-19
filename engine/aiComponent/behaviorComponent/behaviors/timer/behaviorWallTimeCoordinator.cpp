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
  shouldSayTime = false;
  utteranceID = 0;
  utteranceState = UtteranceState::Invalid;
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
  modifiers.behaviorAlwaysDelegates = false;
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
    TransitionToFindFaceInFront();
  }else{
    TransitionToICantDoThat();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::BehaviorUpdate()
{
  if(!IsActivated() || IsControlDelegated()){
    return;
  }

  if(!_dVars.shouldSayTime || 
     (_dVars.utteranceState == UtteranceState::Ready)){
    TransitionToShowWallTime();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::OnBehaviorDeactivated()
{
  GetBEI().GetTextToSpeechCoordinator().CancelUtterance(_dVars.utteranceID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::TransitionToICantDoThat()
{
  ANKI_VERIFY(_iConfig.iCantDoThatBehavior->WantsToBeActivated(), 
              "BehaviorWallTimeCoordinator.TransitionToICantDoThat.BehaviorDoesntWantToBeActivated", "");
  DelegateIfInControl(_iConfig.iCantDoThatBehavior.get(), [this](){
    CancelSelf();
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::TransitionToFindFaceInFront()
{
  ANKI_VERIFY(_iConfig.lookAtFaceInFront->WantsToBeActivated(),
              "BehaviorWallTimeCoordinator.TransitionToShowWallTime.BehaviorDoesntWantToBeActivated", "");
  // We should see a face during this behavior if there's one in front of us to center on
  Pose3d unused;
  const RobotTimeStamp_t lastTimeObserved_ms = GetBEI().GetFaceWorld().GetLastObservedFace(unused);
  DelegateIfInControl(_iConfig.lookAtFaceInFront.get(), [this, lastTimeObserved_ms](){
    Pose3d unused;
    const RobotTimeStamp_t nextTimeSeen = GetBEI().GetFaceWorld().GetLastObservedFace(unused);
    _dVars.shouldSayTime = (nextTimeSeen == lastTimeObserved_ms);
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWallTimeCoordinator::TransitionToShowWallTime()
{
  auto playUtteranceCallback = [this](){
    if(_dVars.shouldSayTime){
      GetBEI().GetTextToSpeechCoordinator().PlayUtterance(_dVars.utteranceID);
    }
  };
  _iConfig.showWallTime->SetShowClockCallback(playUtteranceCallback);

  _iConfig.showWallTime->SetOverrideDisplayTime(_dVars.time);

  ANKI_VERIFY(_iConfig.showWallTime->WantsToBeActivated(),
              "BehaviorWallTimeCoordinator.TransitionToShowWallTime.BehaviorDoesntWantToBeActivated", "");
  DelegateIfInControl(_iConfig.showWallTime.get(), [this](){
    CancelSelf();
  });
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
