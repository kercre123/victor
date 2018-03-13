/**
* File: behaviorReactToVoiceCommand.cpp
*
* Author: Lee Crippen
* Created: 2/16/2017
*
* Description: Simple behavior to immediately respond to the voice command keyphrase, while waiting for further commands.
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToVoiceCommand.h"

#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/pose.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMicDirection.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/micDirectionHistory.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/voiceCommands/voiceCommandComponent.h"
#include "util/console/consoleInterface.h"

#include "coretech/common/engine/utils/timer.h"


#define DEBUG_TRIGGER_WORD_VERBOSE 0 // add some verbose debugging if trying to track down issues

namespace Anki {
namespace Cozmo {

namespace {
  const char* kLeesFeelings                     = "leesFeelings";
  const char* kExitOnIntentsKey                 = "exitOnIntents";
  const char* kEarConBegin                      = "earConAudioEventBegin";
  const char* kEarConEnd                        = "earConAudioEventEnd";
  const char* kTriggerBehaviorKey               = "behaviorOnTrigger";
  const char* kIntentBehaviorKey                = "behaviorOnIntent";
  const char* kIntentListenGetIn                = "playListeningGetInAnim";
  const char* kProceduralBackpackLights         = "backpackLights";

  constexpr AnimationTrigger kInvalidAnimation  = AnimationTrigger::Count;
  // if you change MicDataInfo::kMaxRecordTime_ms, you must change this!
  // ** need a way to coordinate this with AnimProcess **
  constexpr size_t           kMaxRecordTime_ms  = 10000;
  constexpr float            kMaxRecordTime_s   = ( (float)kMaxRecordTime_ms / 1000.0f );
  constexpr float            kListeningBuffer_s = 2.0f;

  CONSOLE_VAR( bool, kRespondsToTriggerWord, "BehaviorReactToVoiceCommand", true );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::InstanceConfig::InstanceConfig() :
  earConBegin( AudioMetaData::GameEvent::GenericEvent::Invalid ),
  earConEnd( AudioMetaData::GameEvent::GenericEvent::Invalid ),
  turnOnTrigger( true ),
  turnOnIntent( !turnOnTrigger ),
  playListeningGetInAnim( true ),
  exitOnIntents( true )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::DynamicVariables::DynamicVariables() :
  state( EState::Positioning ),
  reactionDirection( kMicDirectionUnknown ),
  streamingBeginTime( 0.0f ),
  intentStatus( EIntentStatus::NoIntentHeard )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::BehaviorReactToVoiceCommand( const Json::Value& config ) :
  ICozmoBehavior( config ),
  _triggerDirection( kMicDirectionUnknown )
{
  // do we exit once we've received an intent from the cloud?
  _instanceVars.exitOnIntents = config.get( kExitOnIntentsKey, true ).asBool();

  // do we play ear-con sounds to notify the user when Victor is listening
  {
    std::string earConString;
    if ( JsonTools::GetValueOptional( config, kEarConBegin, earConString ) )
    {
      _instanceVars.earConBegin = AudioMetaData::GameEvent::GenericEventFromString( earConString );
    }

    if ( JsonTools::GetValueOptional( config, kEarConEnd, earConString ) )
    {
      _instanceVars.earConEnd = AudioMetaData::GameEvent::GenericEventFromString( earConString );
    }
  }

  // do we play the backpack lights from the behavior, else assume anims will handle it
  _instanceVars.backpackLights = config.get( kProceduralBackpackLights, false ).asBool();

  // by supplying either kTriggerBehaviorKey XOR kIntentBehaviorKey, we're
  // telling the behavior we want to turn to the mic direction either when hearing
  // the trigger word or when receiving the intent
  {
    _instanceVars.turnOnIntent = JsonTools::GetValueOptional( config, kIntentBehaviorKey, _instanceVars.reactionBehaviorString );
    _instanceVars.turnOnTrigger = JsonTools::GetValueOptional( config, kTriggerBehaviorKey, _instanceVars.reactionBehaviorString );

    if ( _instanceVars.turnOnTrigger && _instanceVars.turnOnIntent )
    {
      _instanceVars.turnOnIntent = false;
      PRINT_NAMED_WARNING( "BehaviorReactToVoiceCommand.Init",
                          "Cannot define BOTH %s and %s", kTriggerBehaviorKey, kIntentBehaviorKey );
    }
  }

  _instanceVars.playListeningGetInAnim = config.get( kIntentListenGetIn, true ).asBool();

  // this will possibly override all of our loaded values
  LoadLeeHappinessValues( config );
  SetRespondToTriggerWord( true );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::LoadLeeHappinessValues( const Json::Value& config )
{
  using namespace AudioMetaData::GameEvent;

  std::string leesFeelings;
  if ( JsonTools::GetValueOptional( config, kLeesFeelings, leesFeelings ) )
  {
    if ( leesFeelings == "lee_happy" )
    {
      // no noise or movement prior to hearing the intent
      // movement and noise after hearing the intent

      _instanceVars.earConBegin             = GenericEvent::Invalid;
      _instanceVars.earConEnd               = GenericEvent::Stop__Robot_Vic_Sfx__Scan_Loop_Stop;
      _instanceVars.turnOnTrigger           = false;
      _instanceVars.turnOnIntent            = true;
      _instanceVars.playListeningGetInAnim  = false;
    }
    else if ( leesFeelings == "lee_meh" )
    {
      // noise but no movement prior to hearing the intent
      // movement and noise after hearing the intent

      _instanceVars.earConBegin             = GenericEvent::Play__Robot_Vic_Sfx__Scan_Loop_Play;
      _instanceVars.earConEnd               = GenericEvent::Stop__Robot_Vic_Sfx__Scan_Loop_Stop;
      _instanceVars.turnOnTrigger           = false;
      _instanceVars.turnOnIntent            = true;
      _instanceVars.playListeningGetInAnim  = true;
    }
    else if ( leesFeelings == "lee_sad" )
    {
      // movement and noise prior to hearing the intent
      // noise but no movement after hearing the intent

      _instanceVars.earConBegin             = GenericEvent::Play__Robot_Vic_Sfx__Scan_Loop_Play;
      _instanceVars.earConEnd               = GenericEvent::Stop__Robot_Vic_Sfx__Scan_Loop_Stop;
      _instanceVars.turnOnTrigger           = true;
      _instanceVars.turnOnIntent            = false;
      _instanceVars.playListeningGetInAnim  = true;
    }
    else
    {
      PRINT_NAMED_ERROR( "BehaviorReactToVoiceCommand.Init",
                         "Config supplied invalid feelings for Lee [%s] (options are lee_happy, lee_meh or lee_sad)",
                         leesFeelings.c_str() );
    }

    // make sure we have a reaction behavior if none was specified
    // this default will be a simple procedural turn towards the mic direction
    if ( _instanceVars.reactionBehaviorString.empty() )
    {
      _instanceVars.reactionBehaviorString = "ProceduralTurnToMicDirection";
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::InitBehavior()
{
  // grab our reaction behavior ...
  if ( !_instanceVars.reactionBehaviorString.empty() )
  {
    ICozmoBehaviorPtr reactionBehavior;
    // try grabbing it from anonymous behaviors first, else we'll grab it from the behavior id
    reactionBehavior = FindAnonymousBehaviorByName( _instanceVars.reactionBehaviorString );
    if ( nullptr == reactionBehavior )
    {
      // no match, try behavior IDs
      const BehaviorID behaviorID = BehaviorTypesWrapper::BehaviorIDFromString( _instanceVars.reactionBehaviorString );
      reactionBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( behaviorID );
    }

    // downcast to a BehaviorReactToMicDirection since we're forcing all reactions to be of this behavior
    DEV_ASSERT_MSG( reactionBehavior != nullptr, "BehaviorReactToVoiceCommand.Init",
                     "Reaction behavior not found: %s", _instanceVars.reactionBehaviorString.c_str());
    DEV_ASSERT_MSG( reactionBehavior->GetClass() == BehaviorClass::ReactToMicDirection,
                    "BehaviorReactToVoiceCommand.Init",
                    "Reaction behavior specified is not of valid class BehaviorClass::ReactToMicDirection");

    _instanceVars.reactionBehavior = std::static_pointer_cast<BehaviorReactToMicDirection>(reactionBehavior);
  }

  SubscribeToTags({
    RobotInterface::RobotToEngineTag::triggerWordDetected
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  if ( _instanceVars.reactionBehavior )
  {
    delegates.insert( _instanceVars.reactionBehavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToVoiceCommand::WantsToBeActivatedBehavior() const
{
  return kRespondsToTriggerWord;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::AlwaysHandleInScope( const RobotToEngineEvent& event )
{
  if ( event.GetData().GetTag() == RobotInterface::RobotToEngineTag::triggerWordDetected )
  {
    _triggerDirection = event.GetData().Get_triggerWordDetected().direction;

    #if DEBUG_TRIGGER_WORD_VERBOSE
    {
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Debug",
                     "Received TriggerWordDetected event with diretion [%d]", (int)_triggerDirection );
    }
    #endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnBehaviorActivated()
{
  _dynamicVars = DynamicVariables();

  // cache our reaction direction at the start in case we were told to turn
  // upon hearing the trigger word
  ComputeReactionDirection();

  if ( GetBEI().HasMoodManager() )
  {
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent( "ReactToTriggerWord", MoodManager::GetCurrentTimeInSeconds() );
  }
  
  // Stop all movement so we can listen for a command
  const auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetMoveComponent().StopAllMotors();

  // Trigger word is heard (since we've been activated) ...
  PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Activated",
                 "Reacting to trigger word from direction [%d] ...",
                 (int)GetReactionDirection() );

  // we start streaming audio as soon as we've played the trigger word
  OnStreamingBegin();

  // Play a reaction behavior if we were told to ...
  if ( _instanceVars.turnOnTrigger && _instanceVars.reactionBehavior )
  {
    const MicDirectionIndex triggerDirection = GetReactionDirection();
    _instanceVars.reactionBehavior->SetReactDirection( triggerDirection );

    // allow the reaction to not want to run in certain directions/states
    if ( _instanceVars.reactionBehavior->WantsToBeActivated() )
    {
      DelegateIfInControl( _instanceVars.reactionBehavior.get(), &BehaviorReactToVoiceCommand::StartListening );
    }
  }

  if ( !IsControlDelegated() )
  {
    StartListening();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnBehaviorDeactivated()
{
  using namespace Anki::Cozmo::VoiceCommand;

  // note: do we need these?
  VoiceCommandComponent* voiceComponent = GetBEI().GetRobotInfo().GetContext()->GetVoiceCommandComponent();
  voiceComponent->ForceListenContext(VoiceCommandListenContext::TriggerPhrase);
  voiceComponent->BroadcastVoiceEvent(RespondingToCommandEnd(VoiceCommandType::HeyCozmo));

  // we've done all we can, now it's up to the next behavior to consume the user intent
  GetBehaviorComp<UserIntentComponent>().SetUserIntentTimeoutEnabled( true );

  // reset this bad boy
  _triggerDirection = kMicDirectionUnknown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::BehaviorUpdate()
{
  if ( _dynamicVars.state == EState::Listening )
  {
    // since this "listening loop" is decoupled from the actual anim process recording,
    // this means we're exiting the listening state based on a computed engine process time,
    // not the actual recording stopped event; since there can be a slight timing
    // error between the two, let's add a bit of buffer to make sure we don't compute
    // the reaction direction AFTER the anim process has "unlocked" the selected direction
    const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ( currentTime < ( _dynamicVars.streamingBeginTime + kMaxRecordTime_s - kListeningBuffer_s ) )
    {
      // we need to constantly update our reaction direction in case the robot
      // is rotating ... there appears to be a bit of lag in the update from SE
      // which is why we need to constantly update during the listen loop (while we're still)
      ComputeReactionDirection();
    }

    const bool isIntentPending = GetBehaviorComp<UserIntentComponent>().IsAnyUserIntentPending();
    if ( _instanceVars.exitOnIntents && isIntentPending)
    {
      // kill delegates, we'll handle next steps with callbacks
      // note: passing true to CancelDelegatees did NOT in fact call my callback,
      //       so calling it myself
      CancelDelegates( false );
      BehaviorReactToVoiceCommand::StopListening();
    }
  }
  else if ( _dynamicVars.state == EState::Thinking )
  {
    // we may receive an intent AFTER we're done listening for various reasons,
    // so poll for it while we're in the thinking state
    // note: does nothing if intent is already set
    SetUserIntentStatus();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::ComputeReactionDirection()
{
  // note:
  // the robot may have moved between the time we heard the trigger word direction
  // and the time we go to respond, so we need to update the direction based on the
  // robots new pose

  // soooooo, the anim process should be doing this automatically by sending us an
  // updated "selected direction" after the robot is done moving, so let's just use that
  // if we find this is not working, we can do a bit of pose math and figure shit out
  _dynamicVars.reactionDirection = GetSelectedDirectionFromMicHistory();

  #if DEBUG_TRIGGER_WORD_VERBOSE
  {
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Debug",
                   "Computing selected direction [%d]", (int)_dynamicVars.reactionDirection );
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionIndex BehaviorReactToVoiceCommand::GetReactionDirection() const
{
  MicDirectionIndex direction = _dynamicVars.reactionDirection;
  if ( kMicDirectionUnknown == direction )
  {
    // fallback to our trigger direction
    // accuracy is generally off by the amount the robot has turned
    // (see comment in ComputeReactionDirection())
    direction = _triggerDirection;
  }

  // this should never happen, but fuck it
  if ( kMicDirectionUnknown == direction )
  {
    // this is the least accurate if called post-intent
    // no difference if called pre-intent / post-trigger word
    direction = GetSelectedDirectionFromMicHistory();
  }

  return direction;
}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionIndex BehaviorReactToVoiceCommand::GetSelectedDirectionFromMicHistory() const
{
  const MicDirectionHistory& history = GetBEI().GetMicDirectionHistory();
  return history.GetSelectedDirection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnStreamingBegin()
{
  static const BackpackLights kStreamingLights =
  {
    .onColors               = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
    .offColors              = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
    .onPeriod_ms            = {{0,0,0}},
    .offPeriod_ms           = {{0,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
  };

  if ( _instanceVars.backpackLights )
  {
    BodyLightComponent& blc = GetBEI().GetBodyLightComponent();
    blc.StartLoopingBackpackLights( kStreamingLights, BackpackLightSource::Behavior, _dynamicVars.lightsHandle );
  }

  _dynamicVars.streamingBeginTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnStreamingEnd()
{
  if ( _instanceVars.backpackLights )
  {
    BodyLightComponent& blc = GetBEI().GetBodyLightComponent();
    blc.StopLoopingBackpackLights( _dynamicVars.lightsHandle );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::StartListening()
{
  // to get into our listening state, we need to play our get-in anim followed by our
  // looping animation.

  // we don't want to enter EState::Listening until we're in our loop or else
  // we could end up exiting too soon and looking like garbage
  auto callback = [this]()
  {
    // have our looping anim abort 10s after streaming started
    const float elapsed = ( BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()
                             - _dynamicVars.streamingBeginTime );
    const float timeout = ( kMaxRecordTime_s - elapsed );
    DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::VC_ListeningLoop,
                                                     0, true, (uint8_t)AnimTrackFlag::NO_TRACKS,
                                                     std::max( timeout, 1.0f ) ),
                         &BehaviorReactToVoiceCommand::StopListening );

    _dynamicVars.state = EState::Listening;
    OnVictorListeningBegin();
  };

  if ( _instanceVars.playListeningGetInAnim )
  {
    DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::VC_ListeningGetIn ), callback );
  }
  else
  {
    callback();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::StopListening()
{
  ASSERT_NAMED_EVENT( _dynamicVars.state == EState::Listening,
                      "BehaviorReactToVoiceCommand.State",
                      "Transitioning to EState::IntentReceived from invalid state [%hhu]", _dynamicVars.state );

  OnVictorListeningEnd();
  SetUserIntentStatus();

  TransitionToThinking();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnVictorListeningBegin()
{
  using namespace AudioMetaData::GameEvent;

  if ( GenericEvent::Invalid != _instanceVars.earConBegin )
  {
    // Play earcon begin audio
    GetBEI().GetRobotAudioClient().PostEvent( _instanceVars.earConBegin, AudioMetaData::GameObjectType::SFX );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnVictorListeningEnd()
{
  using namespace AudioMetaData::GameEvent;

  if ( GenericEvent::Invalid != _instanceVars.earConEnd )
  {
    // Play earcon end audio
    GetBEI().GetRobotAudioClient().PostEvent( _instanceVars.earConEnd, AudioMetaData::GameObjectType::SFX );
  }

  // Note: this is currently decoupled with the actual stream from the AnimProcess
  //       really should be in sync with each other
  OnStreamingEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::SetUserIntentStatus()
{
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
  if ( ( _dynamicVars.intentStatus == EIntentStatus::NoIntentHeard ) && uic.IsAnyUserIntentPending() )
  {
    // next behavior is going to deal with the intent, but we still have more shit to do
    uic.SetUserIntentTimeoutEnabled( false );

    _dynamicVars.intentStatus = EIntentStatus::IntentHeard;

    static const UserIntentTag unmatched = USER_INTENT(unmatched_intent);
    if ( uic.IsUserIntentPending( unmatched ) )
    {
      uic.ClearUserIntent( unmatched );
      _dynamicVars.intentStatus = EIntentStatus::IntentUnknown;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::TransitionToThinking()
{
  _dynamicVars.state = EState::Thinking;

  auto callback = [this]()
  {
    // Play a reaction behavior if we were told to ...
    if ( _instanceVars.turnOnIntent && _instanceVars.reactionBehavior )
    {
      const MicDirectionIndex triggerDirection = GetReactionDirection();
      _instanceVars.reactionBehavior->SetReactDirection( triggerDirection );

      // allow the reaction to not want to run in certain directions/states
      if ( _instanceVars.reactionBehavior->WantsToBeActivated() )
      {
        DelegateIfInControl( _instanceVars.reactionBehavior.get(), &BehaviorReactToVoiceCommand::TransitionToIntentReceived );
      }
    }

    if ( !IsControlDelegated() )
    {
      TransitionToIntentReceived();
    }
  };

  // we need to get out of our listening loop anim before we react
  DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::VC_ListeningGetOut ), callback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::TransitionToIntentReceived()
{
  _dynamicVars.state = EState::IntentReceived;

  AnimationTrigger intentReaction = kInvalidAnimation;

  switch ( _dynamicVars.intentStatus )
  {
    case EIntentStatus::IntentHeard:
      // no animation for valid intent, go straight into the intent behavior
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent", "Heard valid user intent, woot!" );
      break;
    case EIntentStatus::IntentUnknown:
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent", "Heard user intent but could not understand it" );
      intentReaction = AnimationTrigger::VC_IntentUnknown;
      break;
    case EIntentStatus::NoIntentHeard:
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent", "No user intent was heard" );
      intentReaction = AnimationTrigger::VC_NoIntentHeard;
      break;
  }

  if ( kInvalidAnimation != intentReaction )
  {
    // NOTE: What about if we're on the charger?
    DelegateIfInControl( new TriggerLiftSafeAnimationAction( intentReaction ) );
  }
}

} // namespace Cozmo
} // namespace Anki
