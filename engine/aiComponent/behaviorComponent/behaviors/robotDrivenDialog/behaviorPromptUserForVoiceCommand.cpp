/**
 * File: BehaviorPromptUserForVoiceCommand.cpp
 *
 * Author: Sam Russell
 * Created: 2018-04-30
 *
 * Description: This behavior prompts the user for a voice command, then puts Victor into "wake-wordless streaming".
 *              To keep the prompt behavior simple, resultant UserIntents should be handled by the delegating behavior,
 *              or elsewhere in the behaviorStack.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/robotDrivenDialog/behaviorPromptUserForVoiceCommand.h"

#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/mics/micComponent.h"
#include "micDataTypes.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const char* kEarConBegin                      = "earConAudioEventBegin";
  const char* kEarConSuccess                    = "earConAudioEventSuccess";
  const char* kEarConFail                       = "earConAudioEventNeutral";
  const char* kAnimPromptKey                    = "promptAnimationTrigger";
  const char* kAnimResponseToIntentKey          = "animResponseToIntentTrigger";
  const char* kAnimResponseToBadIntentKey       = "animResponseToBadIntentTrigger";
  const char* kAnimRepromptTriggerKey           = "animRepromptTrigger";
  const char* kVocalPromptKey                   = "vocalPromptString";
  const char* kVocalResponseToIntentKey         = "vocalResponseToIntentString";
  const char* kVocalResponseToBadIntentKey      = "vocalResponseToBadIntentString";
  const char* kVocalRepromptKey                 = "vocalRepromptString";
  const char* kMaxRepromptKey                   = "maxNumberOfReprompts";
  const char* kExitOnIntentsKey                 = "stopListeningOnIntents";
  const char* kListeningGetInKey                = "playListeningGetIn";
  const char* kProceduralBackpackLights         = "backpackLights";
  constexpr float kMaxRecordTime_s              = ( (float)MicData::kStreamingTimeout_ms / 1000.0f );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPromptUserForVoiceCommand::InstanceConfig::InstanceConfig()
: earConBegin( AudioMetaData::GameEvent::GenericEvent::Invalid )
, earConSuccess( AudioMetaData::GameEvent::GenericEvent::Invalid )
, earConFail( AudioMetaData::GameEvent::GenericEvent::Invalid )
, animPromptTrigger(AnimationTrigger::Count)
, animResponseToIntentTrigger(AnimationTrigger::Count)
, animResponseToBadIntentTrigger(AnimationTrigger::Count)
, animRepromptTrigger(AnimationTrigger::Count)
, vocalPromptString("")
, vocalResponseToIntentString("")
, vocalResponseToBadIntentString("")
, vocalRepromptString("")
, maxNumReprompts(0)
, exitOnIntents(true)
, backpackLights(true)
, playListeningGetIn(true)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPromptUserForVoiceCommand::DynamicVariables::DynamicVariables()
: state(EState::Prompting)
, streamingBeginTime(0)
, intentStatus(EIntentStatus::NoIntentHeard)
, isListening(false)
, repromptCount(0)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPromptUserForVoiceCommand::BehaviorPromptUserForVoiceCommand(const Json::Value& config)
: ICozmoBehavior(config)
{
  // ear-con vars
  {
    std::string earConString;
    if(JsonTools::GetValueOptional(config, kEarConBegin, earConString)){
      _iConfig.earConBegin = AudioMetaData::GameEvent::GenericEventFromString(earConString);
    }

    if(JsonTools::GetValueOptional(config, kEarConSuccess, earConString)){
      _iConfig.earConSuccess = AudioMetaData::GameEvent::GenericEventFromString(earConString);
    }
    
    if(JsonTools::GetValueOptional(config, kEarConFail, earConString)){
      _iConfig.earConFail = AudioMetaData::GameEvent::GenericEventFromString(earConString);
    }
  }

  // Load up animation triggers
  auto loadTriggerOptional = [](const Json::Value& config, const char* key, AnimationTrigger& trigger){
    std::string debugStr = "BehaviorPromptUserForVoiceCommand.Ctor.MissingParam.";
    std::string triggerString;
    if(JsonTools::GetValueOptional(config, key, triggerString)){
      trigger = AnimationTriggerFromString(triggerString);
      ANKI_VERIFY(trigger != AnimationTrigger::Count,
                  "BehaviorPromptUserForVoiceCommand.Ctor.InvalidAnimTrigger",
                  "%s is not a valid anim trigger",
                  key);
    }
  };

  loadTriggerOptional(config, kAnimPromptKey, _iConfig.animPromptTrigger);
  loadTriggerOptional(config, kAnimResponseToIntentKey, _iConfig.animResponseToIntentTrigger);
  loadTriggerOptional(config, kAnimResponseToBadIntentKey, _iConfig.animResponseToBadIntentTrigger);
  loadTriggerOptional(config, kAnimRepromptTriggerKey, _iConfig.animRepromptTrigger);

  JsonTools::GetValueOptional(config, kVocalPromptKey, _iConfig.vocalPromptString);
  JsonTools::GetValueOptional(config, kVocalResponseToIntentKey, _iConfig.vocalResponseToIntentString);
  JsonTools::GetValueOptional(config, kVocalResponseToBadIntentKey, _iConfig.vocalResponseToBadIntentString);
  JsonTools::GetValueOptional(config, kVocalRepromptKey, _iConfig.vocalRepromptString);

  // Should we exit the behavior as soon as an intent is pending, or finish what we're doing first?
  // TODO:(str) interrupting behaviors may prevent the ending earcon from playing
  JsonTools::GetValueOptional(config, kExitOnIntentsKey, _iConfig.exitOnIntents);
  // Should we repeat the prompt if it fails? If so, how many times
  JsonTools::GetValueOptional(config, kMaxRepromptKey, _iConfig.maxNumReprompts);
  // play backpack lights from the behavior? else assume anims will handle it
  JsonTools::GetValueOptional(config, kProceduralBackpackLights, _iConfig.backpackLights);
  // play the getIn animation for the listening anim?j
  JsonTools::GetValueOptional(config, kListeningGetInKey, _iConfig.playListeningGetIn);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPromptUserForVoiceCommand::~BehaviorPromptUserForVoiceCommand()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPromptUserForVoiceCommand::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kEarConBegin,
    kEarConSuccess,
    kEarConFail,
    kAnimPromptKey,
    kAnimResponseToIntentKey,
    kAnimResponseToBadIntentKey,
    kAnimRepromptTriggerKey,
    kVocalPromptKey,
    kVocalResponseToIntentKey,
    kVocalResponseToBadIntentKey,
    kVocalRepromptKey,
    kExitOnIntentsKey,
    kMaxRepromptKey,
    kProceduralBackpackLights,
    kListeningGetInKey
  };

  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  TransitionToPrompting();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::OnBehaviorDeactivated()
{
  TurnOffBackpackLights();

  // Any resultant intents should be handled by external behaviors or transitions, let 'em roll
  GetBehaviorComp<UserIntentComponent>().SetUserIntentTimeoutEnabled(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return; 
  }

  if(EState::Listening == _dVars.state){
    CheckForPendingIntents();
    if((EIntentStatus::NoIntentHeard != _dVars.intentStatus) && (_iConfig.exitOnIntents)){
      CancelSelf();
    }
  } else if(EState::Thinking == _dVars.state){
    CheckForPendingIntents();
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::CheckForPendingIntents()
{
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
  if((EIntentStatus::NoIntentHeard == _dVars.intentStatus) && uic.IsAnyUserIntentPending()){

    // Don't dismiss unclaimed intents until this behavior exits, or other behaviors may miss their
    // chance to claim the pending intents
    uic.SetUserIntentTimeoutEnabled(false);

    _dVars.intentStatus = EIntentStatus::IntentHeard;

    // If it was an unmatched intent, note it so we can respond appropriately, then clear it.
    static const UserIntentTag unmatched = USER_INTENT(unmatched_intent);
    if(uic.IsUserIntentPending(unmatched))
    {
      SmartActivateUserIntent(unmatched);
      _dVars.intentStatus = EIntentStatus::IntentUnknown;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToPrompting()
{
  // decipher whether or not to speak or just animate the prompt
  IActionRunner* promptAction = nullptr;
  if(!_iConfig.vocalPromptString.empty()){
    SayTextAction* vocalPromptAction = new SayTextAction(_iConfig.vocalPromptString, SayTextIntent::Text);
    vocalPromptAction->SetAnimationTrigger(_iConfig.animPromptTrigger);
    promptAction = vocalPromptAction;
  } else {
    promptAction = new TriggerAnimationAction(_iConfig.animPromptTrigger);
  }

  _dVars.state = EState::Prompting;
  DelegateIfInControl(promptAction, &BehaviorPromptUserForVoiceCommand::TransitionToListening);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToListening()
{
  // Turn on backpack lights to indicate streaming
  static const BackpackLights kStreamingLights = 
  {
    .onColors               = {{NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN}},
    .offColors              = {{NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN}},
    .onPeriod_ms            = {{0,0,0}},
    .offPeriod_ms           = {{0,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
  };

  if(_iConfig.backpackLights){
    BodyLightComponent& blc = GetBEI().GetBodyLightComponent();
    blc.StartLoopingBackpackLights(kStreamingLights, BackpackLightSource::Behavior, _dVars.lightsHandle);
  }

  //Trip the earcon
  if(AudioMetaData::GameEvent::GenericEvent::Invalid != _iConfig.earConBegin){
    GetBEI().GetRobotAudioClient().PostEvent(_iConfig.earConBegin, AudioMetaData::GameObjectType::Behavior);
  }

  GetBEI().GetMicComponent().StartWakeWordlessStreaming();
  _dVars.streamingBeginTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  _dVars.state = EState::Listening;

  // Start the getIn animation, then go straight into the listening loop from the callback
  auto callback = [this](){
    const float elapsed = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _dVars.streamingBeginTime;
    const float timeout = kMaxRecordTime_s - elapsed;
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::VC_ListeningLoop,
                                                  0, true, (uint8_t)AnimTrackFlag::NO_TRACKS,
                                                  std::max(timeout, 1.0f)),
                        &BehaviorPromptUserForVoiceCommand::TransitionToThinking);
  };

  if(_iConfig.playListeningGetIn){
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::VC_ListeningGetIn), callback);
  } else {
    callback();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToThinking()
{
  _dVars.state = EState::Thinking;

  // Play the Listening getOut, then close out the streaming stuff in case an intent was just a little late
  auto callback = [this](){
    TurnOffBackpackLights();
    
    // Play "earCon end"
    Audio::EngineRobotAudioClient& rac = GetBEI().GetRobotAudioClient();

    if(EIntentStatus::IntentHeard == _dVars.intentStatus){
      if(AudioMetaData::GameEvent::GenericEvent::Invalid != _iConfig.earConSuccess){
        rac.PostEvent(_iConfig.earConSuccess, AudioMetaData::GameObjectType::Behavior);
      }
    } else {
      if(AudioMetaData::GameEvent::GenericEvent::Invalid != _iConfig.earConFail){
        rac.PostEvent(_iConfig.earConFail, AudioMetaData::GameObjectType::Behavior);
      }
    }

    TransitionToIntentReceived();
  };

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::VC_ListeningGetOut), callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToIntentReceived()
{
  // Two ways we make it all the way here:
  //  1. Any resultant intent is handled by a non-interrupting behavior && !ExitOnIntents
  //  2. Any resultant intents have gone unclaimed
  
  // Play intent response anim and voice, if set
  IActionRunner* responseAction = nullptr;
  if(EIntentStatus::IntentHeard == _dVars.intentStatus){
    if(_iConfig.vocalResponseToIntentString.empty() &&
       AnimationTrigger::Count == _iConfig.animResponseToIntentTrigger){
      // If we don't have any intent response anims or vocalizations, but we got a valid intent,
      // end this behavior. The resultant intent should be handled elsewhere after this behavior releases control.
      CancelSelf();
      return;
    } else if(!_iConfig.vocalResponseToIntentString.empty()){
      SayTextAction* vocalResponseAction = new SayTextAction(_iConfig.vocalResponseToIntentString, SayTextIntent::Text);
      vocalResponseAction->SetAnimationTrigger(_iConfig.animResponseToIntentTrigger);
      responseAction = vocalResponseAction;
    } else {
      responseAction = new TriggerAnimationAction(_iConfig.animResponseToIntentTrigger);
    }

    DelegateIfInControl(responseAction, [this](){ CancelSelf(); });
  } else {
    if(_iConfig.vocalResponseToBadIntentString.empty() &&
       AnimationTrigger::Count == _iConfig.animResponseToBadIntentTrigger){
      // If we don't have any intent response anims or vocalizations, from here we will want to
      // either re-prompt or exit
      TransitionToReprompt();
      return;
    } else if (!_iConfig.vocalResponseToBadIntentString.empty()){
      SayTextAction* vocalResponseAction = new SayTextAction(_iConfig.vocalResponseToBadIntentString, SayTextIntent::Text);
      vocalResponseAction->SetAnimationTrigger(_iConfig.animResponseToBadIntentTrigger);
      responseAction = vocalResponseAction;
    } else {
      responseAction = new TriggerAnimationAction(_iConfig.animResponseToBadIntentTrigger);
    }

    DelegateIfInControl(responseAction, &BehaviorPromptUserForVoiceCommand::TransitionToReprompt);
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToReprompt()
{
  if(_dVars.repromptCount < _iConfig.maxNumReprompts){
    _dVars.repromptCount++;

    // Reset necessary vars
    _dVars.intentStatus = EIntentStatus::NoIntentHeard;

    // decipher whether or not to speak or just animate the prompt
    IActionRunner* repromptAction = nullptr;
    if(_iConfig.vocalRepromptString.empty() &&
       AnimationTrigger::Count == _iConfig.animRepromptTrigger){
      // If we don't have any Reprompt anims or vocalizations, just reuse the prompting state
      PRINT_NAMED_INFO("BehaviorPromptUserForVoiceCommand.RepromptGeneric",
                       "Reprompting user %d of %d times with original prompt action",
                       _dVars.repromptCount,
                       _iConfig.maxNumReprompts);
      TransitionToPrompting();
      return;
    } else if (!_iConfig.vocalRepromptString.empty()){
      SayTextAction* vocalRepromptAction = new SayTextAction(_iConfig.vocalRepromptString, SayTextIntent::Text);
      vocalRepromptAction->SetAnimationTrigger(_iConfig.animRepromptTrigger);
      repromptAction = vocalRepromptAction;
    } else {
      repromptAction = new TriggerAnimationAction(_iConfig.animPromptTrigger);
    }

    PRINT_NAMED_INFO("BehaviorPromptUserForVoiceCommand.RepromptSpecialized",
                     "Reprompting user %d of %d times with specialized reprompt action.",
                     _dVars.repromptCount,
                     _iConfig.maxNumReprompts);
    _dVars.state = EState::Reprompt;
    DelegateIfInControl(repromptAction, &BehaviorPromptUserForVoiceCommand::TransitionToListening);

    return;
  }

  CancelSelf();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TurnOffBackpackLights()
{
  if(_iConfig.backpackLights && _dVars.lightsHandle.IsValid()){
    BodyLightComponent& blc = GetBEI().GetBodyLightComponent();
    blc.StopLoopingBackpackLights(_dVars.lightsHandle);
  }
}

} // namespace Cozmo 
} // namespace Anki
