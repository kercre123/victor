/**
* File: behaviorReactToVoiceCommand.cpp
*
* Author: Jarrod Hatfield
* Created: 2/16/2018
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
#include "micDataTypes.h"
#include "util/console/consoleInterface.h"

#include "coretech/common/engine/utils/timer.h"


#define DEBUG_TRIGGER_WORD_VERBOSE 0 // add some verbose debugging if trying to track down issues

namespace Anki {
namespace Cozmo {

namespace {
  const char* kExitOnIntentsKey                 = "stopListeningOnIntents";
  const char* kEarConBegin                      = "earConAudioEventBegin";
  const char* kEarConSuccess                    = "earConAudioEventSuccess";
  const char* kEarConFail                       = "earConAudioEventNeutral";
  const char* kTriggerBehaviorKey               = "behaviorOnTrigger";
  const char* kIntentBehaviorKey                = "behaviorOnIntent";
  const char* kIntentListenGetIn                = "playListeningGetInAnim";
  const char* kProceduralBackpackLights         = "backpackLights";

  constexpr AnimationTrigger kInvalidAnimation  = AnimationTrigger::Count;
  constexpr float            kMaxRecordTime_s   = ( (float)MicData::kStreamingTimeout_ms / 1000.0f );
  constexpr float            kListeningBuffer_s = 2.0f;

  CONSOLE_VAR( bool, kRespondsToTriggerWord, "BehaviorReactToVoiceCommand", true );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::InstanceConfig::InstanceConfig() :
  earConBegin( AudioMetaData::GameEvent::GenericEvent::Invalid ),
  earConSuccess( AudioMetaData::GameEvent::GenericEvent::Invalid ),
  earConFail( AudioMetaData::GameEvent::GenericEvent::Invalid ),
  turnOnTrigger( false ),
  turnOnIntent( !turnOnTrigger ),
  playListeningGetInAnim( true ),
  exitOnIntents( true ),
  backpackLights( true )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::DynamicVariables::DynamicVariables() :
  state( EState::Positioning ),
  reactionDirection( kMicDirectionUnknown ),
  streamingBeginTime( 0.0f ),
  intentStatus( EIntentStatus::NoIntentHeard ),
  isListening( false )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::BehaviorReactToVoiceCommand( const Json::Value& config ) :
  ICozmoBehavior( config ),
  _triggerDirection( kMicDirectionUnknown )
{
  // do we exit once we've received an intent from the cloud?
  JsonTools::GetValueOptional( config, kExitOnIntentsKey, _iVars.exitOnIntents );

  // do we play ear-con sounds to notify the user when Victor is listening
  {
    std::string earConString;
    if ( JsonTools::GetValueOptional( config, kEarConBegin, earConString ) )
    {
      _iVars.earConBegin = AudioMetaData::GameEvent::GenericEventFromString( earConString );
    }

    if ( JsonTools::GetValueOptional( config, kEarConSuccess, earConString ) )
    {
      _iVars.earConSuccess = AudioMetaData::GameEvent::GenericEventFromString( earConString );
    }

    if ( JsonTools::GetValueOptional( config, kEarConFail, earConString ) )
    {
      _iVars.earConFail = AudioMetaData::GameEvent::GenericEventFromString( earConString );
    }
  }

  // do we play the backpack lights from the behavior, else assume anims will handle it
  JsonTools::GetValueOptional( config, kProceduralBackpackLights, _iVars.backpackLights );

  // by supplying either kTriggerBehaviorKey XOR kIntentBehaviorKey, we're
  // telling the behavior we want to turn to the mic direction either when hearing
  // the trigger word or when receiving the intent
  {
    _iVars.turnOnTrigger = JsonTools::GetValueOptional( config, kTriggerBehaviorKey, _iVars.reactionBehaviorString );
    _iVars.turnOnIntent = JsonTools::GetValueOptional( config, kIntentBehaviorKey, _iVars.reactionBehaviorString );

    if ( _iVars.turnOnTrigger && _iVars.turnOnIntent )
    {
      _iVars.turnOnTrigger = false;
      PRINT_NAMED_WARNING( "BehaviorReactToVoiceCommand.Init",
                          "Cannot define BOTH %s and %s in json config", kTriggerBehaviorKey, kIntentBehaviorKey );
    }
  }

  JsonTools::GetValueOptional( config, kIntentListenGetIn, _iVars.playListeningGetInAnim );

  SetRespondToTriggerWord( true );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kExitOnIntentsKey,
    kEarConBegin,
    kEarConSuccess,
    kEarConFail,
    kProceduralBackpackLights,
    kIntentBehaviorKey,
    kTriggerBehaviorKey,
    kTriggerBehaviorKey,
    kIntentListenGetIn
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::InitBehavior()
{
  // grab our reaction behavior ...
  if ( !_iVars.reactionBehaviorString.empty() )
  {
    ICozmoBehaviorPtr reactionBehavior;
    // try grabbing it from anonymous behaviors first, else we'll grab it from the behavior id
    reactionBehavior = FindAnonymousBehaviorByName( _iVars.reactionBehaviorString );
    if ( nullptr == reactionBehavior )
    {
      // no match, try behavior IDs
      const BehaviorID behaviorID = BehaviorTypesWrapper::BehaviorIDFromString( _iVars.reactionBehaviorString );
      reactionBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( behaviorID );
    }

    // downcast to a BehaviorReactToMicDirection since we're forcing all reactions to be of this behavior
    DEV_ASSERT_MSG( reactionBehavior != nullptr, "BehaviorReactToVoiceCommand.Init",
                     "Reaction behavior not found: %s", _iVars.reactionBehaviorString.c_str());
    DEV_ASSERT_MSG( reactionBehavior->GetClass() == BehaviorClass::ReactToMicDirection,
                    "BehaviorReactToVoiceCommand.Init",
                    "Reaction behavior specified is not of valid class BehaviorClass::ReactToMicDirection");

    _iVars.reactionBehavior = std::static_pointer_cast<BehaviorReactToMicDirection>(reactionBehavior);
  }

  SubscribeToTags(
  {{
    RobotInterface::RobotToEngineTag::triggerWordDetected,
    RobotInterface::RobotToEngineTag::animEvent
  }});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  if ( _iVars.reactionBehavior )
  {
    delegates.insert( _iVars.reactionBehavior.get() );
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
void BehaviorReactToVoiceCommand::HandleWhileActivated( const RobotToEngineEvent& event )
{
  if ( _dVars.state == EState::Positioning )
  {
    const RobotInterface::RobotToEngine::Tag tag = event.GetData().GetTag();
    if ( tag == RobotInterface::RobotToEngineTag::animEvent )
    {
      const AnimationEvent& animEvent = event.GetData().Get_animEvent();
      if ( animEvent.event_id == AnimEvent::LISTENING_BEGIN )
      {
        OnVictorListeningBegin();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnBehaviorActivated()
{
  _dVars = DynamicVariables();

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
  if ( _iVars.turnOnTrigger && _iVars.reactionBehavior )
  {
    const MicDirectionIndex triggerDirection = GetReactionDirection();
    _iVars.reactionBehavior->SetReactDirection( triggerDirection );

    // allow the reaction to not want to run in certain directions/states
    if ( _iVars.reactionBehavior->WantsToBeActivated() )
    {
      DelegateIfInControl( _iVars.reactionBehavior.get(), &BehaviorReactToVoiceCommand::StartListening );
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
  // in case we were interrupted before we had a chance to turn off backpack ligths, do so now ...
  if ( _iVars.backpackLights && _dVars.lightsHandle.IsValid() )
  {
    BodyLightComponent& blc = GetBEI().GetBodyLightComponent();
    blc.StopLoopingBackpackLights( _dVars.lightsHandle );
  }

  // we've done all we can, now it's up to the next behavior to consume the user intent
  GetBehaviorComp<UserIntentComponent>().SetUserIntentTimeoutEnabled( true );

  // reset this bad boy
  _triggerDirection = kMicDirectionUnknown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::BehaviorUpdate()
{
  if ( _dVars.state == EState::Listening )
  {
    // since this "listening loop" is decoupled from the actual anim process recording,
    // this means we're exiting the listening state based on a computed engine process time,
    // not the actual recording stopped event; since there can be a slight timing
    // error between the two, let's add a bit of buffer to make sure we don't compute
    // the reaction direction AFTER the anim process has "unlocked" the selected direction
    const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ( currentTime < ( _dVars.streamingBeginTime + kMaxRecordTime_s - kListeningBuffer_s ) )
    {
      // we need to constantly update our reaction direction in case the robot
      // is rotating ... there appears to be a bit of lag in the update from SE
      // which is why we need to constantly update during the listen loop (while we're still)
      ComputeReactionDirection();
    }

    const bool isIntentPending = GetBehaviorComp<UserIntentComponent>().IsAnyUserIntentPending();
    if ( _iVars.exitOnIntents && isIntentPending)
    {
      // kill delegates, we'll handle next steps with callbacks
      // note: passing true to CancelDelegatees did NOT in fact call my callback,
      //       so calling it myself
      CancelDelegates( false );
      BehaviorReactToVoiceCommand::StopListening();
    }
  }
  else if ( _dVars.state == EState::Thinking )
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
  _dVars.reactionDirection = GetSelectedDirectionFromMicHistory();

  #if DEBUG_TRIGGER_WORD_VERBOSE
  {
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Debug",
                   "Computing selected direction [%d]", (int)_dVars.reactionDirection );
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionIndex BehaviorReactToVoiceCommand::GetReactionDirection() const
{
  MicDirectionIndex direction = _dVars.reactionDirection;
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
  _dVars.streamingBeginTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnStreamingEnd()
{

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
                             - _dVars.streamingBeginTime );
    const float timeout = ( kMaxRecordTime_s - elapsed );
    DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::VC_ListeningLoop,
                                                     0, true, (uint8_t)AnimTrackFlag::NO_TRACKS,
                                                     std::max( timeout, 1.0f ) ),
                         &BehaviorReactToVoiceCommand::StopListening );

    // the _dVars.state is purposely decoupled from _dVars.isListening so that we can trigger the listening response
    // before we go into the listening state flow
    // this is our fallback in the case where we haven't begun listening by the time the listening state has begun
    _dVars.state = EState::Listening;
    if ( !_dVars.isListening )
    {
      OnVictorListeningBegin();
    }
  };

  if ( _iVars.playListeningGetInAnim )
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
  ASSERT_NAMED_EVENT( _dVars.state == EState::Listening,
                      "BehaviorReactToVoiceCommand.State",
                      "Transitioning to EState::IntentReceived from invalid state [%hhu]", _dVars.state );

  // Note: this is currently decoupled with the actual stream from the AnimProcess
  //       really should be in sync with each other
  OnStreamingEnd();

  SetUserIntentStatus();
  TransitionToThinking();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnVictorListeningBegin()
{
  using namespace AudioMetaData::GameEvent;

  static const BackpackLights kStreamingLights =
  {
    .onColors               = {{NamedColors::CYAN,NamedColors::CYAN,NamedColors::CYAN}},
    .offColors              = {{NamedColors::CYAN,NamedColors::CYAN,NamedColors::CYAN}},
    .onPeriod_ms            = {{0,0,0}},
    .offPeriod_ms           = {{0,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
  };

  if ( _iVars.backpackLights )
  {
    BodyLightComponent& blc = GetBEI().GetBodyLightComponent();
    blc.StartLoopingBackpackLights( kStreamingLights, BackpackLightSource::Behavior, _dVars.lightsHandle );
  }

  if ( GenericEvent::Invalid != _iVars.earConBegin )
  {
    // Play earcon begin audio
    GetBEI().GetRobotAudioClient().PostEvent( _iVars.earConBegin, AudioMetaData::GameObjectType::Behavior );
  }

  // technically we don't have to trigger this at the same time as EState::Listening, so we need to track this separately
  _dVars.isListening = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnVictorListeningEnd()
{
  using namespace AudioMetaData::GameEvent;

  _dVars.isListening = false;

  if ( _iVars.backpackLights && _dVars.lightsHandle.IsValid() )
  {
    BodyLightComponent& blc = GetBEI().GetBodyLightComponent();
    blc.StopLoopingBackpackLights( _dVars.lightsHandle );
  }

  // play our "earcon end" audio, which depends on if our intent was successfully heard or not
  BehaviorExternalInterface& bei = GetBEI();
  if ( EIntentStatus::IntentHeard == _dVars.intentStatus )
  {
    if ( GenericEvent::Invalid != _iVars.earConSuccess )
    {
      // Play earcon end audio
      bei.GetRobotAudioClient().PostEvent( _iVars.earConSuccess, AudioMetaData::GameObjectType::Behavior );
    }
  }
  else
  {
    if ( GenericEvent::Invalid != _iVars.earConFail )
    {
      // Play earcon end audio
      bei.GetRobotAudioClient().PostEvent( _iVars.earConFail, AudioMetaData::GameObjectType::Behavior );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::SetUserIntentStatus()
{
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
  if ( ( _dVars.intentStatus == EIntentStatus::NoIntentHeard ) && uic.IsAnyUserIntentPending() )
  {
    // next behavior is going to deal with the intent, but we still have more shit to do
    uic.SetUserIntentTimeoutEnabled( false );

    _dVars.intentStatus = EIntentStatus::IntentHeard;

    static const UserIntentTag unmatched = USER_INTENT(unmatched_intent);
    if ( uic.IsUserIntentPending( unmatched ) )
    {
      uic.ClearUserIntent( unmatched );
      _dVars.intentStatus = EIntentStatus::IntentUnknown;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::TransitionToThinking()
{
  _dVars.state = EState::Thinking;

  auto callback = [this]()
  {
    // we're keeping our "listening feedback" open until the last possible moment, since the intent can come in after we've
    // closed our recording stream.
    OnVictorListeningEnd();

    // Play a reaction behavior if we were told to ...
    // ** only in the case that we've heard a valid intent **
    const bool heardValidIntent = ( _dVars.intentStatus == EIntentStatus::IntentHeard );
    if ( heardValidIntent && _iVars.turnOnIntent && _iVars.reactionBehavior )
    {
      const MicDirectionIndex triggerDirection = GetReactionDirection();
      _iVars.reactionBehavior->SetReactDirection( triggerDirection );

      // allow the reaction to not want to run in certain directions/states
      if ( _iVars.reactionBehavior->WantsToBeActivated() )
      {
        DelegateIfInControl( _iVars.reactionBehavior.get(), &BehaviorReactToVoiceCommand::TransitionToIntentReceived );
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
  _dVars.state = EState::IntentReceived;

  AnimationTrigger intentReaction = kInvalidAnimation;

  switch ( _dVars.intentStatus )
  {
    case EIntentStatus::IntentHeard:
      // no animation for valid intent, go straight into the intent behavior
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent", "Heard valid user intent, woot!" );
      break;
    case EIntentStatus::IntentUnknown:
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent", "Heard user intent but could not understand it" );
      intentReaction = AnimationTrigger::VC_IntentNeutral;
      break;
    case EIntentStatus::NoIntentHeard:
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent", "No user intent was heard" );
      intentReaction = AnimationTrigger::VC_IntentNeutral;
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
