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
namespace Cozmo {

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
                          PRINT_NAMED_INFO("BehaviorTextToSpeechLoop.State", "State = %s", #s); \
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
void BehaviorTextToSpeechLoop::SetTextToSay(const std::string& textToSay, const SayTextIntent& intent)
{
  _dVars.textToSay = textToSay;

  auto callback = [this](const UtteranceState& utteranceState)
  {
    OnUtteranceUpdated(utteranceState);
  };

  UtteranceTriggerType triggerType = UtteranceTriggerType::Manual;
  if(_iConfig.triggeredByAnim){
    triggerType = UtteranceTriggerType::KeyFrame;
  }
  _dVars.utteranceID = GetBEI().GetTextToSpeechCoordinator().CreateUtterance(_dVars.textToSay,
                                                                             intent,
                                                                             triggerType,
                                                                             callback);
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
    SetTextToSay(_iConfig.devTestUtteranceString, SayTextIntent::Unprocessed);
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
      PRINT_NAMED_INFO("BehaviorTextToSpeechLoop.Update.UtteranceCompleted",
                       "Utterance %d finished playing",
                       _dVars.utteranceID);
      CancelDelegates(false);
      TransitionToGetOut();
    } else if(!IsControlDelegated()){
      TransitionToSpeakingLoop();
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
  GetBEI().GetTextToSpeechCoordinator().CancelUtterance(_dVars.utteranceID);

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
void BehaviorTextToSpeechLoop::OnUtteranceUpdated(const UtteranceState& state)
{
  _dVars.utteranceState = state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::Interrupt()
{
  CancelDelegates(false);
  TransitionToEmergencyGetOut();
}

} // namespace Cozmo
} // namespace Anki
