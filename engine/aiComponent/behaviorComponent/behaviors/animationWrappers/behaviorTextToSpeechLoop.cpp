/**
 * File: BehaviorTextToSpeechLoop.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-05-17
 *
 * Description: Play a looping animation while reciting text to speech
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/textToSpeech/textToSpeechCoordinator.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

namespace Anki {
namespace Vector {

namespace {
const char* kIdleAnimationKey            = "idleAnimation";
const char* kGetInAnimationKey           = "getInAnimation";
const char* kLoopAnimationKey            = "loopAnimation";
const char* kGetOutAnimationKey          = "getOutAnimation";
const char* kEmergencyGetOutAnimationKey = "emergencyGetOutAnimation";
// TODO:(str) we will eventually need to support anim keyframe driven tts
// Uncomment when the TTSCoordinator can handle that case
// const char* kTriggeredByAnimKey          = "utteranceTriggeredByAnim";
const char* kLockTreadsKey               = "lockTreads";

// DEV TESTING Allow simple test behavior construction from JSON only
const char* kDevTestUtteranceKey         = "DEV_TEST_UTTERANCE";
}

#define SET_STATE(s) do{ \
                          _dVars.state = State::s; \
                          PRINT_CH_INFO("Behaviors", "BehaviorTextToSpeechLoop.State", "State = %s", #s); \
                        } while(0);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTextToSpeechLoop::InstanceConfig::InstanceConfig()
{
  idleTrigger                 = AnimationTrigger::Count;
  getInTrigger                = AnimationTrigger::Count;
  loopTrigger                 = AnimationTrigger::Count;
  getOutTrigger               = AnimationTrigger::Count;
  emergencyGetOutTrigger      = AnimationTrigger::Count;
  devTestUtteranceString      = "";
  triggeredByAnim             = false;
  idleDuringTTSGeneration     = false;
  tracksToLock                = static_cast<uint8_t>(AnimTrackFlag::NO_TRACKS);
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTextToSpeechLoop::DynamicVariables::DynamicVariables() 
: textToSay("")
, state(State::IdleLoop)
, utteranceID(kInvalidUtteranceID)
, utteranceState(UtteranceState::Invalid)
, hasSentPlayCommand(false)
, cancelOnNextUpdate(false)
, cancelOnNextLoop(false)
{
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTextToSpeechLoop::BehaviorTextToSpeechLoop(const Json::Value& config)
: ICozmoBehavior(config)
{
  _iConfig = InstanceConfig();

  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  JsonTools::GetCladEnumFromJSON(config, kIdleAnimationKey, _iConfig.idleTrigger, debugName, false);
  if(AnimationTrigger::Count != _iConfig.idleTrigger){
    _iConfig.idleDuringTTSGeneration = true;
  }

  JsonTools::GetCladEnumFromJSON(config, kGetInAnimationKey, _iConfig.getInTrigger, debugName, false);
  JsonTools::GetCladEnumFromJSON(config, kLoopAnimationKey, _iConfig.loopTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kGetOutAnimationKey, _iConfig.getOutTrigger, debugName, false);
  JsonTools::GetCladEnumFromJSON(config, kEmergencyGetOutAnimationKey, _iConfig.emergencyGetOutTrigger, debugName, false);

  // TODO:(str) we will eventually need to support anim keyframe driven tts
  // Uncomment when the TTSCoordinator can handle that case
  // JsonTools::GetValueOptional(config, kTriggeredByAnimKey, _iConfig.triggeredByAnim);

  bool lockTreads = false;
  JsonTools::GetValueOptional( config, kLockTreadsKey, lockTreads );
  _iConfig.tracksToLock = lockTreads
                          ? static_cast<uint8_t>(AnimTrackFlag::BODY_TRACK)
                          : static_cast<uint8_t>(AnimTrackFlag::NO_TRACKS);
  
  JsonTools::GetValueOptional(config, kDevTestUtteranceKey, _iConfig.devTestUtteranceString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kIdleAnimationKey,
    kGetInAnimationKey,
    kLoopAnimationKey,
    kGetOutAnimationKey,
    kEmergencyGetOutAnimationKey,
    // TODO:(str) we will eventually need to support anim keyframe driven tts
    // Uncomment when the TTSCoordinator can handle that case
    // kTriggeredByAnimKey,
    kLockTreadsKey,
    kDevTestUtteranceKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTextToSpeechLoop::~BehaviorTextToSpeechLoop()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::SetTextToSay(const std::string& textToSay,
                                            const UtteranceReadyCallback readyCallback,
                                            const AudioTtsProcessingStyle style)
{
  _dVars.textToSay = textToSay;

  auto callback = [this, readyCallback{std::move(readyCallback)}](const UtteranceState& utteranceState)
  {
    const bool wasGenerating = (UtteranceState::Generating == _dVars.utteranceState);
    OnUtteranceUpdated(utteranceState);

    // could move this into OnUtteranceUpdated(), but felt it didn't make any difference for now ...
    // if we were generating the utterance and now we've moved on, send the callback to let our user know
    const bool isGenerating = (UtteranceState::Generating == _dVars.utteranceState);
    if(wasGenerating && !isGenerating){
      DEV_ASSERT_MSG((UtteranceState::Ready == _dVars.utteranceState) || (UtteranceState::Invalid == _dVars.utteranceState),
                     "BehaviorTextToSpeechLoop.CreateUtterance",
                     "Utterance state changed from [Generating] to [%d], this should not be possible",
                     (int)_dVars.utteranceState);

      if(readyCallback) {
        readyCallback(UtteranceState::Ready == _dVars.utteranceState);
      }
    }
  };

  UtteranceTriggerType triggerType = UtteranceTriggerType::Manual;
  if(_iConfig.triggeredByAnim){
    triggerType = UtteranceTriggerType::KeyFrame;
  }
  _dVars.utteranceID = GetBEI().GetTextToSpeechCoordinator().CreateUtterance(_dVars.textToSay,
                                                                             triggerType,
                                                                             style,
                                                                             1.0f,
                                                                             callback);

  // if we failed to created the utterance, let the callback know so they aren't waiting forever ...
  if((kInvalidUtteranceID == _dVars.utteranceID) && readyCallback){
    readyCallback(false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::ClearTextToSay()
{
  if(kInvalidUtteranceID != _dVars.utteranceID) {
    GetBEI().GetTextToSpeechCoordinator().CancelUtterance(_dVars.utteranceID);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTextToSpeechLoop::WantsToBeActivatedBehavior() const 
{
  return ((kInvalidUtteranceID != _dVars.utteranceID) || !_iConfig.devTestUtteranceString.empty());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::OnBehaviorActivated()
{
  // For a standalone test behavior, exampleTextToSpeechLoop
  if(!_iConfig.devTestUtteranceString.empty()){
    SetTextToSay(_iConfig.devTestUtteranceString, {}, AudioTtsProcessingStyle::Unprocessed);
  }

  if(kInvalidUtteranceID == _dVars.utteranceID) {
    PRINT_NAMED_WARNING("BehaviorTextToSpeechLoop.InvalidUtteranceID",
                        "Utterance text must be set before this behavior is activated");
    // In practice, we should never be here, but for unit tests and realworld MISuse-cases, its best if 
    // we exit smoothly rather than outright CancelSelf() here
    if(AnimationTrigger::Count != _iConfig.emergencyGetOutTrigger){
      TransitionToEmergencyGetOut();
    } else {
      _dVars.cancelOnNextUpdate = true;
    }
    return;
  }

  if( !_iConfig.idleDuringTTSGeneration || (UtteranceState::Ready == _dVars.utteranceState) ){
    TransitionToGetIn();
  } else {
    TransitionToIdleLoop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::OnBehaviorDeactivated()
{
  // make sure we clean up our utterance data else we'll leak the wav data
  if((UtteranceState::Invalid != _dVars.utteranceState) && (UtteranceState::Finished != _dVars.utteranceState)){
    GetBEI().GetTextToSpeechCoordinator().CancelUtterance(_dVars.utteranceID);
  }

  _dVars = DynamicVariables();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if(_dVars.cancelOnNextUpdate){
    CancelSelf();
  }

  if(State::EmergencyGetOut == _dVars.state){
    return;
  }

  if( (State::IdleLoop == _dVars.state || State::SpeakingLoop == _dVars.state) &&
      UtteranceState::Invalid == _dVars.utteranceState){
    PRINT_NAMED_WARNING("BehaviorTextToSpeechLoop.Update.InvalidUtterance",
                        "Behavior %s will exit as utterance has returned invalid",
                        GetDebugLabel().c_str());
    CancelDelegates(false);
    TransitionToEmergencyGetOut();
    return;
  }

  if(State::IdleLoop == _dVars.state){
    if(UtteranceState::Ready == _dVars.utteranceState){
      CancelDelegates(false);
      TransitionToGetIn();
    } else if(!IsControlDelegated()){
      TransitionToIdleLoop();
    }
  }

  if(State::SpeakingLoop == _dVars.state){
    if(UtteranceState::Ready == _dVars.utteranceState &&
       !_iConfig.triggeredByAnim &&
       !_dVars.hasSentPlayCommand){
      PlayUtterance();
    } else if (UtteranceState::Finished == _dVars.utteranceState){
      PRINT_CH_INFO("Behaviors", "BehaviorTextToSpeechLoop.Update.UtteranceCompleted",
                       "Utterance %d finished playing",
                       _dVars.utteranceID);
      CancelDelegates(false);
      TransitionToGetOut();
    } else if(!IsControlDelegated()){
      if(!_dVars.cancelOnNextLoop ){
        TransitionToSpeakingLoop();
      } else {
        // in speaking loop but we were told to interrupt, so exit gracefully
        TransitionToGetOut();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::TransitionToIdleLoop()
{
  SET_STATE(IdleLoop);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(_iConfig.idleTrigger, 1, true, _iConfig.tracksToLock));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::TransitionToGetIn()
{
  SET_STATE(GetIn);
  auto callback = [this](){
    if( (UtteranceState::Ready == _dVars.utteranceState) && !_iConfig.triggeredByAnim ){
      PlayUtterance();
    }
    TransitionToSpeakingLoop();
  };

  if(AnimationTrigger::Count == _iConfig.getInTrigger){
    callback();
  } else {
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(_iConfig.getInTrigger, 1, true, _iConfig.tracksToLock),
                        callback);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::TransitionToSpeakingLoop()
{
  SET_STATE(SpeakingLoop);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(_iConfig.loopTrigger, 1, true, _iConfig.tracksToLock));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::TransitionToGetOut()
{
  SET_STATE(GetOut);
  if(AnimationTrigger::Count == _iConfig.getOutTrigger){
    CancelSelf();
  } else {
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(_iConfig.getOutTrigger, 1, true, _iConfig.tracksToLock),
                        [this](){ CancelSelf(); } );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::TransitionToEmergencyGetOut()
{
  SET_STATE(EmergencyGetOut);
  ClearTextToSay();

  if(AnimationTrigger::Count == _iConfig.emergencyGetOutTrigger){
    CancelSelf();
  } else {
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(_iConfig.emergencyGetOutTrigger,
                                                           1, true, _iConfig.tracksToLock),
                        [this](){ CancelSelf(); } );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::PlayUtterance()
{
  ANKI_VERIFY(false == _dVars.hasSentPlayCommand,
              "BehaviorTextToSpeechLoop.PlayUtterance",
              "Command to play utterance has already been sent. Should not be sent again");

  if(GetBEI().GetTextToSpeechCoordinator().PlayUtterance(_dVars.utteranceID)){
    _dVars.hasSentPlayCommand = true;
  } else {
    TransitionToEmergencyGetOut();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTextToSpeechLoop::IsUtteranceReady() const
{
  return (UtteranceState::Ready == _dVars.utteranceState);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::OnUtteranceUpdated(const UtteranceState& state)
{
  _dVars.utteranceState = state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::Interrupt( bool immediate )
{
  if(immediate){
    CancelDelegates(false);
    TransitionToEmergencyGetOut();
  } else {
    // we've been told to cancel our TTS, but there's no hurry, so ...
    // + cancel TTS immediately so it feels responsive
    // + transition into the get out anim after the next looping anim so that the anims transition properly
    ClearTextToSay();
    _dVars.cancelOnNextLoop = true;;
  }
}

} // namespace Vector
} // namespace Anki
