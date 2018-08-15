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

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/backpackLights/engineBackpackLightComponent.h"
#include "engine/components/mics/micComponent.h"
#include "micDataTypes.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"


namespace Anki {
namespace Vector {
  
namespace {
  const char* kDefaultTTSBehaviorID = "DefaultTextToSpeechLoop";

  // JSON keys
  const char* kStreamType                         = "streamType";
  const char* kEarConSuccess                      = "earConAudioEventSuccess";
  const char* kEarConFail                         = "earConAudioEventNeutral";
  // TODO:(str) Currently not in use. Rework this to use a smarter TurnToFace structure, perhaps LookAtFaceInFront
  const char* kShouldTurnToFaceKey                = "shouldTurnToFaceBeforePrompting";
  const char* kTextToSpeechBehaviorKey            = "textToSpeechBehaviorID";
  const char* kVocalPromptKey                     = "vocalPromptString";
  const char* kVocalResponseToIntentKey           = "vocalResponseToIntentString";
  const char* kVocalResponseToBadIntentKey        = "vocalResponseToBadIntentString";
  const char* kVocalRepromptKey                   = "vocalRepromptString";
  const char* kListenGetInOverrideKey             = "listenGetInOverrideTrigger";
  const char* kListenAnimOverrideKey              = "listenAnimOverrideTrigger";
  const char* kListenGetOutOverrideKey            = "listenGetOutOverrideTrigger";
  const char* kStopListeningOnIntentsKey          = "stopListeningOnIntents";
  const char* kListeningGetInKey                  = "playListeningGetIn";
  const char* kListeningGetOutKey                 = "playListeningGetOut";
  const char* kMaxRepromptKey                     = "maxNumberOfReprompts";
  const char* kProceduralBackpackLights           = "backpackLights";
  constexpr float kMaxRecordTime_s                = ( (float)MicData::kStreamingTimeout_ms / 1000.0f );
}

#define SET_STATE(s) do{ \
                          _dVars.state = EState::s; \
                          PRINT_NAMED_INFO("BehaviorPromptUserForVoiceCommand.State", "State = %s", #s); \
                        } while(0);
                        
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPromptUserForVoiceCommand::InstanceConfig::InstanceConfig()
: streamType( CloudMic::StreamType::Normal )
, earConSuccess( AudioMetaData::GameEvent::GenericEvent::Invalid )
, earConFail( AudioMetaData::GameEvent::GenericEvent::Invalid )
, ttsBehaviorID(kDefaultTTSBehaviorID)
, ttsBehavior(nullptr)
, listenGetInOverrideTrigger(AnimationTrigger::Count)
, listenAnimOverrideTrigger(AnimationTrigger::Count)
, listenGetOutOverrideTrigger(AnimationTrigger::Count)
, maxNumReprompts(0)
, shouldTurnToFace(false)
, stopListeningOnIntents(true)
, backpackLights(true)
, playListeningGetIn(true)
, playListeningGetOut(true)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPromptUserForVoiceCommand::DynamicVariables::DynamicVariables()
: state(EState::TurnToFace)
, intentStatus(EIntentStatus::NoIntentHeard)
, isListening(false)
, repromptCount(0)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPromptUserForVoiceCommand::BehaviorPromptUserForVoiceCommand(const Json::Value& config)
: ICozmoBehavior(config)
{
  // we must have a stream type supplied, else the cloud doesn't know what to do with it
  // * defaults don't make much sense at this point either
  const std::string streamTypeString = JsonTools::ParseString(config,
                                                              kStreamType,
                                                              "BehaviorPromptUserForVoiceCommand.MissingStreamType");
  _iConfig.streamType = CloudMic::StreamTypeFromString( streamTypeString );

  // ear-con vars
  {
    std::string earConString;
    if(JsonTools::GetValueOptional(config, kEarConSuccess, earConString)){
      _iConfig.earConSuccess = AudioMetaData::GameEvent::GenericEventFromString(earConString);
    }
    
    if(JsonTools::GetValueOptional(config, kEarConFail, earConString)){
      _iConfig.earConFail = AudioMetaData::GameEvent::GenericEventFromString(earConString);
    }
  }

  JsonTools::GetValueOptional(config, kShouldTurnToFaceKey, _iConfig.shouldTurnToFace);

  // Set up the TextToSpeech Behavior
  JsonTools::GetValueOptional(config, kTextToSpeechBehaviorKey, _iConfig.ttsBehaviorID);

  // If prompt string is _not_ set by JSON config, it must be set by a call to SetPrompt()
  // before the behavior wants to be activated, or an error will occur.
  _iConfig.wasPromptSetFromJson = JsonTools::GetValueOptional(config, kVocalPromptKey, _iConfig.vocalPromptString);

  JsonTools::GetValueOptional(config, kVocalResponseToIntentKey, _iConfig.vocalResponseToIntentString);
  JsonTools::GetValueOptional(config, kVocalResponseToBadIntentKey, _iConfig.vocalResponseToBadIntentString);
  _iConfig.wasRepromptSetFromJson = JsonTools::GetValueOptional(config, kVocalRepromptKey, _iConfig.vocalRepromptString);

  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  JsonTools::GetCladEnumFromJSON(config, kListenGetInOverrideKey, _iConfig.listenGetInOverrideTrigger, 
                                 debugName, false);
  JsonTools::GetCladEnumFromJSON(config, kListenAnimOverrideKey, _iConfig.listenAnimOverrideTrigger,
                                 debugName, false);
  JsonTools::GetCladEnumFromJSON(config, kListenGetOutOverrideKey, _iConfig.listenGetOutOverrideTrigger,
                                 debugName, false);

   // Should we exit the behavior as soon as an intent is pending, or finish what we're doing first?
  // TODO:(str) interrupting behaviors will prevent the ending earcon from playing
  JsonTools::GetValueOptional(config, kStopListeningOnIntentsKey, _iConfig.stopListeningOnIntents);
  // play the getIn animation for the listening anim?
  JsonTools::GetValueOptional(config, kListeningGetInKey, _iConfig.playListeningGetIn);
  // play the getOut animation for the listening anim?
  JsonTools::GetValueOptional(config, kListeningGetOutKey, _iConfig.playListeningGetOut);
  // Should we repeat the prompt if it fails? If so, how many times
  JsonTools::GetValueOptional(config, kMaxRepromptKey, _iConfig.maxNumReprompts);
  // Should behavior have fallback backpack lights - these will stay on the whole behavior lifecycle
  // while the lights triggered by the anim process to indicate streaming may turn on/off as streams
  // open/close
  JsonTools::GetValueOptional(config, kProceduralBackpackLights, _iConfig.backpackLights);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPromptUserForVoiceCommand::~BehaviorPromptUserForVoiceCommand()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPromptUserForVoiceCommand::WantsToBeActivatedBehavior() const
{
  if(!ANKI_VERIFY(!_iConfig.vocalPromptString.empty(), "BehaviorPromptUserForVoiceCommand.MissingPromptString", ""))
  {
    // Prompt was not set by JSON config or a call to SetPrompt()
    return false;
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.ttsBehavior.get());
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
    kEarConSuccess,
    kEarConFail,
    kShouldTurnToFaceKey,
    kTextToSpeechBehaviorKey,
    kVocalPromptKey,
    kVocalResponseToIntentKey,
    kVocalResponseToBadIntentKey,
    kVocalRepromptKey,
    kListenGetInOverrideKey,
    kListenAnimOverrideKey,
    kListenGetOutOverrideKey,
    kVocalResponseToIntentKey,
    kVocalResponseToBadIntentKey,
    kVocalRepromptKey,
    kStopListeningOnIntentsKey,
    kMaxRepromptKey,
    kProceduralBackpackLights,
    kListeningGetInKey,
    kListeningGetOutKey,
    kStreamType
  };

  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::SetPrompt(const std::string &text)
{
  // Don't allow programmatic override of Json-configured prompts.
  // Use a separate indicator 'wasPromptSetFromJson' to allow multiple calls to SetPrompt if
  //  the prompt was _not_ set by Json config.
  if(ANKI_VERIFY(!_iConfig.wasPromptSetFromJson,
                 "BehaviorPromptUserForVoiceCommand.SetPrompt.AlreadySetFromJson",
                 "Prompt set by Json config. Refusing to override."))
  {
    _iConfig.vocalPromptString = text;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::SetReprompt(const std::string &text)
{
  // Don't allow programmatic override of Json-configured prompts.
  // Use a separate indicator 'wasRepromptSetFromJson' to allow multiple calls to SetReprompt if
  //  the prompt was _not_ set by Json config.
  if(ANKI_VERIFY(!_iConfig.wasRepromptSetFromJson,
                 "BehaviorPromptUserForVoiceCommand.SetPrompt.AlreadySetFromJson",
                 "Prompt set by Json config. Refusing to override."))
  {
    _iConfig.vocalRepromptString = text;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::InitBehavior(){
  BehaviorID ttsID = BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.ttsBehaviorID);
  GetBEI().GetBehaviorContainer().FindBehaviorByIDAndDowncast(ttsID,
                                                              BEHAVIOR_CLASS(TextToSpeechLoop),
                                                              _iConfig.ttsBehavior);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  if(_iConfig.shouldTurnToFace){
    TransitionToTurnToFace();
  } else {
    TransitionToPrompting();
  }
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
    if((EIntentStatus::IntentHeard == _dVars.intentStatus) && (_iConfig.stopListeningOnIntents)){
      // End the listening anim, which should push us into Thinking
      CancelDelegates();
      TransitionToThinking();
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
void BehaviorPromptUserForVoiceCommand::TransitionToTurnToFace()
{
  SET_STATE(TurnToFace);
  DelegateIfInControl(new TurnTowardsLastFacePoseAction(), &BehaviorPromptUserForVoiceCommand::TransitionToPrompting);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToPrompting()
{
  SET_STATE(Prompting);
  _iConfig.ttsBehavior->SetTextToSay(_iConfig.vocalPromptString);
  if(_iConfig.ttsBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.ttsBehavior.get(), &BehaviorPromptUserForVoiceCommand::TransitionToListening);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToListening()
{
  // Turn on backpack lights to indicate streaming
  static const BackpackLightAnimation::BackpackAnimation kStreamingLights =
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
    BackpackLightComponent& blc = GetBEI().GetBackpackLightComponent();
    blc.StartLoopingBackpackAnimation(kStreamingLights, _dVars.lightsHandle);
  }


  SET_STATE(Listening);

  // Start the getIn animation, then go straight into the listening loop from the callback
  auto callback = [this](){
    GetBehaviorComp<UserIntentComponent>().StartWakeWordlessStreaming(_iConfig.streamType);

    AnimationTrigger listenTrigger = (AnimationTrigger::Count != _iConfig.listenAnimOverrideTrigger) ?
      _iConfig.listenAnimOverrideTrigger : AnimationTrigger::VC_ListeningLoop;

    DelegateIfInControl(new TriggerAnimationAction(listenTrigger,
                                                   0, true, (uint8_t)AnimTrackFlag::NO_TRACKS,
                                                   std::max(kMaxRecordTime_s, 1.0f)),
                        &BehaviorPromptUserForVoiceCommand::TransitionToThinking);
  };

  if(_iConfig.playListeningGetIn){
    AnimationTrigger listenGetInTrigger = (AnimationTrigger::Count != _iConfig.listenGetInOverrideTrigger) ?
      _iConfig.listenGetInOverrideTrigger : AnimationTrigger::VC_ListeningGetIn;

    DelegateIfInControl(new TriggerAnimationAction(listenGetInTrigger), callback);
  } else {
    callback();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToThinking()
{
  SET_STATE(Thinking);

  // Play the Listening getOut, then close out the streaming stuff in case an intent was just a little late
  auto callback = [this](){    
    // Play "earCon end"
    TurnOffBackpackLights();
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

  if(_iConfig.playListeningGetOut){
    AnimationTrigger listenGetOutTrigger = (AnimationTrigger::Count != _iConfig.listenGetOutOverrideTrigger) ?
      _iConfig.listenGetOutOverrideTrigger : AnimationTrigger::VC_ListeningGetOut;

    DelegateIfInControl(new TriggerAnimationAction(listenGetOutTrigger), callback);
  } else {
    callback();
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToIntentReceived()
{
  // Two ways we make it all the way here:
  //  1. Any resultant intent is handled by a non-interrupting behavior && !ExitOnIntents
  //  2. Any resultant intents have gone unclaimed

  SET_STATE(Thinking)

  // Play intent response anim and voice, if set
  if(EIntentStatus::IntentHeard == _dVars.intentStatus){
    if(_iConfig.vocalResponseToIntentString.empty()){
      // No prompts specified, exit so the intent can be handled elsewhere
      CancelSelf();
      return;
    } else {
      _iConfig.ttsBehavior->SetTextToSay(_iConfig.vocalResponseToIntentString);
      if(_iConfig.ttsBehavior->WantsToBeActivated()){
        DelegateIfInControl(_iConfig.ttsBehavior.get(), [this](){ CancelSelf(); });
      }
    }
  } else {
    if(_iConfig.vocalResponseToBadIntentString.empty()){
      // No prompts specified, either reprompt or exit
      TransitionToReprompt();
      return;
    } else {
      _iConfig.ttsBehavior->SetTextToSay(_iConfig.vocalResponseToBadIntentString);
      if(_iConfig.ttsBehavior->WantsToBeActivated()){
        DelegateIfInControl(_iConfig.ttsBehavior.get(), &BehaviorPromptUserForVoiceCommand::TransitionToReprompt);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TransitionToReprompt()
{
  if(_dVars.repromptCount < _iConfig.maxNumReprompts){
    _dVars.repromptCount++;

    // Reset necessary vars
    _dVars.intentStatus = EIntentStatus::NoIntentHeard;

    if(_iConfig.vocalRepromptString.empty()){
      // If we don't have any Reprompt anims or vocalizations, just reuse the prompting state
      PRINT_NAMED_INFO("BehaviorPromptUserForVoiceCommand.RepromptGeneric",
                       "Reprompting user %d of %d times with original prompt action",
                       _dVars.repromptCount,
                       _iConfig.maxNumReprompts);
      TransitionToPrompting();
      return;
    } else {
      PRINT_NAMED_INFO("BehaviorPromptUserForVoiceCommand.RepromptSpecialized",
                      "Reprompting user %d of %d times with specialized reprompt action.",
                      _dVars.repromptCount,
                      _iConfig.maxNumReprompts);
      SET_STATE(Reprompt);
      _iConfig.ttsBehavior->SetTextToSay(_iConfig.vocalRepromptString);
      if(_iConfig.ttsBehavior->WantsToBeActivated()){
        DelegateIfInControl(_iConfig.ttsBehavior.get(), &BehaviorPromptUserForVoiceCommand::TransitionToListening);
      }
      return;
    }
  }

  CancelSelf();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPromptUserForVoiceCommand::TurnOffBackpackLights()
{
  if(_iConfig.backpackLights && _dVars.lightsHandle.IsValid()){
    BackpackLightComponent& blc = GetBEI().GetBackpackLightComponent();
    blc.StopLoopingBackpackAnimation(_dVars.lightsHandle);
  }
}


} // namespace Vector 
} // namespace Anki
