/**
 * File: BehaviorOnboardingTeachWakeWord.cpp
 *
 * Author: Sam Russell
 * Created: 2018-11-06
 *
 * Description: Maintain "eye contact" with the user while awaiting a series of wakewords. Use anims to indicate
 *              successful wakeword detections
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/phases/behaviorOnboardingTeachWakeWord.h"

#include "audioEngine/multiplexer/audioCladMessageHelper.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"

#define LOG_CHANNEL "Behaviors"

#define SET_STATE(s) do{ \
  _dVars.state = TeachWakeWordState::s; \
} while(0);

namespace{
const char* kSimulatedStreamingDurationKey = "simulatedStreamingDuration_ms";
}

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingTeachWakeWord::InstanceConfig::InstanceConfig(const Json::Value& config)
: lookAtUserBehavior(nullptr)
, listenGetInAnimTrigger(AnimationTrigger::OnboardingListenGetIn)
, listenGetOutAnimTrigger(AnimationTrigger::OnboardingListenGetOut)
, celebrationAnimTrigger(AnimationTrigger::OnboardingWakeWordSuccess)
, simulatedStreamingDuration_ms(-1) // -1 will use the default setting from the anim process
, numWakeWordsToCelebrate(3)
{
  JsonTools::GetValueOptional(config, kSimulatedStreamingDurationKey, simulatedStreamingDuration_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingTeachWakeWord::DynamicVariables::DynamicVariables()
: state(TeachWakeWordState::ListenForWakeWord)
, resumeUponActivation(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingTeachWakeWord::DynamicVariables::PersistentVars::PersistentVars()
: numWakeWordDetections(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// IOnboardingPhaseWithProgress
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int BehaviorOnboardingTeachWakeWord::GetPhaseProgressInPercent() const
{
  return (int)(100.0f * ((float)_dVars.persistent.numWakeWordDetections / (float)_iConfig.numWakeWordsToCelebrate));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingTeachWakeWord::BehaviorOnboardingTeachWakeWord(const Json::Value& config)
 : ICozmoBehavior(config)
 , _iConfig(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingTeachWakeWord::~BehaviorOnboardingTeachWakeWord()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::InitBehavior()
{
  auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.lookAtUserBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(OnboardingLookAtUser) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingTeachWakeWord::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  // Must allow periods of non-delegation to handle the WaitingForWakeWordGetInToFinish state
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.lookAtUserBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSimulatedStreamingDurationKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::OnBehaviorActivated()
{
  // reset dynamic variables, accounting for persistence when necessary
  if(_dVars.resumeUponActivation){
    auto resumeVars = _dVars.persistent;
    _dVars = DynamicVariables();
    _dVars.persistent = resumeVars;
  } else {
    _dVars = DynamicVariables();
  }

  EnableWakeWordDetection();
  TransitionToListenForWakeWord();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::OnBehaviorDeactivated()
{
  DisableWakeWordDetection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::BehaviorUpdate()
{
  if( !IsActivated() ) {
    return;
  }

  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if( uic.IsTriggerWordPending() ){
    TransitionToWaitForWakeWordGetInToFinish();
    uic.ClearPendingTriggerWord();
  }

  if( TeachWakeWordState::WaitForWakeWordGetInToFinish == _dVars.state ){
    if( !GetBehaviorComp<UserIntentComponent>().WaitingForTriggerWordGetInToFinish() ){
      TransitionToReactToWakeWord();
    }
  }
  else if( !IsControlDelegated() ){
    // If the user drops vector we could end up here, just go back to listening
    TransitionToListenForWakeWord();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::TransitionToListenForWakeWord()
{
  CancelDelegates( false );
  SET_STATE( ListenForWakeWord );

  if( _iConfig.lookAtUserBehavior->WantsToBeActivated() ){
    // Just re-enter this state if we exit it for some reason
    DelegateIfInControl( _iConfig.lookAtUserBehavior.get(),
      &BehaviorOnboardingTeachWakeWord::TransitionToListenForWakeWord );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::TransitionToWaitForWakeWordGetInToFinish()
{
  CancelDelegates( false );
  SET_STATE( WaitForWakeWordGetInToFinish );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::TransitionToReactToWakeWord()
{
  CancelDelegates( false );
  SET_STATE( ReactToWakeWord );

  if( ++_dVars.persistent.numWakeWordDetections >= _iConfig.numWakeWordsToCelebrate ){
    TransitionToCelebrateSuccess();
  }
  else{
    DelegateIfInControl( new TriggerLiftSafeAnimationAction( _iConfig.listenGetOutAnimTrigger ),
                         &BehaviorOnboardingTeachWakeWord::TransitionToListenForWakeWord );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::TransitionToCelebrateSuccess()
{
  CancelDelegates( false );
  SET_STATE( CelebrateSuccess );

  DelegateIfInControl( new TriggerLiftSafeAnimationAction( _iConfig.celebrationAnimTrigger ),
    [this](){
      CancelSelf();
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::EnableWakeWordDetection()
{
  namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
  auto earConBegin = AudioMetaData::GameEvent::GenericEventFromString( "Play__Robot_Vic_Sfx__Wake_Word_On" );
  auto postAudioEvent = AECH::CreatePostAudioEvent( earConBegin, AudioMetaData::GameObjectType::Behavior, 0 );
  SmartPushResponseToTriggerWord( _iConfig.listenGetInAnimTrigger,
                                  postAudioEvent,
                                  StreamAndLightEffect::StreamingDisabledButWithLight,
                                  _iConfig.simulatedStreamingDuration_ms );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingTeachWakeWord::DisableWakeWordDetection()
{
  SmartPopResponseToTriggerWord();
}

} //namespace Vector
} //namespace Anki
