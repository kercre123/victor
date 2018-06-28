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
#include "engine/aiComponent/behaviorComponent/attentionTransferComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMicDirection.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/backpackLights/backpackLightComponent.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/micDirectionHistory.h"
#include "engine/components/movementComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robotInterface/messageHandler.h"
#include "micDataTypes.h"
#include "osState/osState.h"
#include "util/console/consoleInterface.h"

#include "coretech/common/engine/utils/timer.h"


#define DEBUG_TRIGGER_WORD_VERBOSE 0 // add some verbose debugging if trying to track down issues

namespace Anki {
namespace Cozmo {

namespace {

  const char* kEarConBegin                      = "earConAudioEventBegin";
  const char* kEarConSuccess                    = "earConAudioEventSuccess";
  const char* kEarConFail                       = "earConAudioEventNeutral";
  const char* kIntentBehaviorKey                = "behaviorOnIntent";
  const char* kProceduralBackpackLights         = "backpackLights";
  const char* kNotifyOnErrors                   = "notifyOnErrors";

  constexpr float            kMaxRecordTime_s   = ( (float)MicData::kStreamingTimeout_ms / 1000.0f );
  constexpr float            kListeningBuffer_s = 2.0f;

#define CONSOLE_GROUP "BehaviorReactToVoiceCommand"

  CONSOLE_VAR( bool, kRespondsToTriggerWord, CONSOLE_GROUP, true );

  // the behavior will always "listed" for at least this long once it hears the wakeword, even if we receive
  // an error sooner than this. Note that the behavior will also consider the intent to be an error if the
  // stream doesn't open within this amount of time, so don't lower this number too much
  CONSOLE_VAR_RANGED( float, kMinListeningTimeout_s, CONSOLE_GROUP, 5.0f, 0.0f, 30.0f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::InstanceConfig::InstanceConfig() :
  earConBegin( AudioMetaData::GameEvent::GenericEvent::Invalid ),
  earConSuccess( AudioMetaData::GameEvent::GenericEvent::Invalid ),
  earConFail( AudioMetaData::GameEvent::GenericEvent::Invalid ),
  backpackLights( true ),
  cloudErrorTracker( "VoiceCommandErrorTracker" )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::DynamicVariables::DynamicVariables() :
  state( EState::GetIn ),
  reactionDirection( kMicDirectionUnknown ),
  streamingBeginTime( -1.0f ),
  intentStatus( EIntentStatus::NoIntentHeard ),
  timestampToDisableTurnFor(0)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::BehaviorReactToVoiceCommand( const Json::Value& config ) :
  ICozmoBehavior( config ),
  _triggerDirection( kMicDirectionUnknown )
{
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

  // get the behavior to play after an intent comes in
  JsonTools::GetValueOptional( config, kIntentBehaviorKey, _iVars.reactionBehaviorString );

  int numErrorsToTriggerAnim;
  float errorTrackingWindow_s;

  ANKI_VERIFY( RecentOccurrenceTracker::ParseConfig(config[kNotifyOnErrors],
                                                    numErrorsToTriggerAnim,
                                                    errorTrackingWindow_s),
               "BehaviorReactToVoiceCommand.Constructor.InvalidConfig",
               "Behavior '%s' specified invalid recent occurrence config",
               GetDebugLabel().c_str() );

  _iVars.cloudErrorHandle = _iVars.cloudErrorTracker.GetHandle(numErrorsToTriggerAnim, errorTrackingWindow_s);

  SetRespondToTriggerWord( true );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kEarConBegin,
    kEarConSuccess,
    kEarConFail,
    kProceduralBackpackLights,
    kIntentBehaviorKey,
    kNotifyOnErrors
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::InitBehavior()
{
  // grab our reaction behavior ...
  if ( !_iVars.reactionBehaviorString.empty() )
  {
    ICozmoBehaviorPtr reactionBehavior = FindBehavior( _iVars.reactionBehaviorString );
    // downcast to a BehaviorReactToMicDirection since we're forcing all reactions to be of this behavior
    DEV_ASSERT_MSG( reactionBehavior->GetClass() == BehaviorClass::ReactToMicDirection,
                    "BehaviorReactToVoiceCommand.Init.IncorrectMicDirectionBehavior",
                    "Reaction behavior specified is not of valid class BehaviorClass::ReactToMicDirection");
    _iVars.reactionBehavior = std::static_pointer_cast<BehaviorReactToMicDirection>(reactionBehavior);
  }

  _iVars.unmatchedIntentBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( BEHAVIOR_ID(IntentUnmatched) );
  DEV_ASSERT( _iVars.unmatchedIntentBehavior != nullptr,
              "BehaviorReactToVoiceCommand.Init.UnmatchedIntentBehaviorMissing");

  _iVars.noCloudBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( BEHAVIOR_ID(NoCloud) );
  DEV_ASSERT( _iVars.noCloudBehavior != nullptr,
              "BehaviorReactToVoiceCommand.Init.NoCloudBehaviorMissing");

  _iVars.noWifiBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( BEHAVIOR_ID(NoWifi) );
  DEV_ASSERT( _iVars.noWifiBehavior != nullptr,
              "BehaviorReactToVoiceCommand.Init.NoWifiBehaviorMissing");

  SubscribeToTags(
  {
    RobotInterface::RobotToEngineTag::triggerWordDetected
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  if ( _iVars.reactionBehavior )
  {
    delegates.insert( _iVars.reactionBehavior.get() );
  }

  delegates.insert( _iVars.unmatchedIntentBehavior.get() );
  delegates.insert( _iVars.noCloudBehavior.get() );
  delegates.insert( _iVars.noWifiBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = true;

  // Since so many voice commands need faces, this helps improve the changes that a behavior following
  // this one will know about faces when the behavior starts
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
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
  _dVars = DynamicVariables();

  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* wakeWordBegin = new external_interface::WakeWordBegin;
    gi->Broadcast( ExternalMessageRouter::Wrap(wakeWordBegin) );
  }

  // cache our reaction direction at the start in case we were told to turn
  // upon hearing the trigger word
  ComputeReactionDirection();

  if ( GetBEI().HasMoodManager() )
  {
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent( "ReactToTriggerWord", MoodManager::GetCurrentTimeInSeconds() );
  }

  // Trigger word is heard (since we've been activated) ...
  PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Activated",
                 "Reacting to trigger word from direction [%d] ...",
                 (int)GetReactionDirection() );

  StartListening();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnBehaviorDeactivated()
{
  // in case we were interrupted before we had a chance to turn off backpack ligths, do so now ...
  if ( _iVars.backpackLights && _dVars.lightsHandle.IsValid() )
  {
    BackpackLightComponent& blc = GetBEI().GetBackpackLightComponent();
    blc.StopLoopingBackpackAnimation( _dVars.lightsHandle );
  }

  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* wakeWordEnd = new external_interface::WakeWordEnd;
    const bool intentHeard = (_dVars.intentStatus != EIntentStatus::NoIntentHeard);
    wakeWordEnd->set_intent_heard( intentHeard );
    if( intentHeard ) {
      UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
      // we use this dirty dirty method here instead of sending this message directly from the uic
      // since we know whether the intent was heard here, and it's nice that the same behavior
      // on activation/deactivation sends the two messages. if the uic sent the end message, it
      // might be sent without an initial message.
      const UserIntentData* intentData = uic.GetPendingUserIntent();
      if( intentData != nullptr ) {
        // ideally we'd send a proto message structured the same as the intent, but this would mean
        // duplicating the entire userIntent.clad file for proto, or converting the engine handling
        // of intents to clad, neither of which I've got time for.
        std::stringstream ss;
        ss << intentData->intent.GetJSON();
        wakeWordEnd->set_intent_json( ss.str() );
      }
    }
    gi->Broadcast( ExternalMessageRouter::Wrap(wakeWordEnd) );
  }

  // we've done all we can, now it's up to the next behavior to consume the user intent
  GetBehaviorComp<UserIntentComponent>().SetUserIntentTimeoutEnabled( true );

  // reset this bad boy
  _triggerDirection = kMicDirectionUnknown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::BehaviorUpdate()
{

  const bool wasStreaming = _dVars.streamingBeginTime >= 0.0f;
  const bool isStreaming = GetBehaviorComp<UserIntentComponent>().IsCloudStreamOpen();

  if( !wasStreaming && isStreaming ) {
    OnStreamingBegin();
  }

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
    if ( isIntentPending ) {
      // kill delegates, we'll handle next steps with callbacks
      // note: passing true to CancelDelegatees doesn't call the callback if we also delegate
      PRINT_CH_INFO("MicData", "BehaviorReactToVoiceCommand.StopListening.IntentPending",
                    "Stopping listening because an intent is pending");
      CancelDelegates( false );
      StopListening();
    }
    else {
      const bool isErrorPending = GetBehaviorComp<UserIntentComponent>().WasUserIntentError();
      const bool neverStartedStreaming = !isStreaming && !wasStreaming;
      if( isErrorPending || neverStartedStreaming )
      {
        const float timeoutStartTime = wasStreaming ? _dVars.streamingBeginTime : GetTimeActivated_s();

        // cloud either returned an error or hasn't opened a stream yet. In either case, this counts as an
        // error after our min listening time has expired
        const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if( currTime_s >= timeoutStartTime + kMinListeningTimeout_s ) {
          PRINT_CH_INFO("MicData", "BehaviorReactToVoiceCommand.StopListening.Error",
                        "Stopping listening because of a(n) %s",
                        isErrorPending ? "error" : "timeout");
          CancelDelegates( false );
          StopListening();
        }
      }
    }
  }
  else if ( _dVars.state == EState::Thinking )
  {
    // we may receive an intent AFTER we're done listening for various reasons,
    // so poll for it while we're in the thinking state
    // note: does nothing if intent is already set
    UpdateUserIntentStatus();
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
  _dVars.reactionDirection = GetDirectionFromMicHistory();

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
    PRINT_CH_INFO( "MicData", "BehaviorReactToVoiceCommand.GetReactionDirection.UsingTriggerDirection",
                   "Didn't have a reaction or trigger direction, so falling back to trigger direction" );
    direction = _triggerDirection;
  }

  // this should never happen, but fuck it
  if ( kMicDirectionUnknown == direction )
  {
    PRINT_NAMED_WARNING("BehaviorReactToVoiceCommand.GetReactionDirection.UsingSelectedHistoryDirection",
                        "Didn't have a reaction or trigger direction, so falling back to latest selected direction");

    // this is the least accurate if called post-intent
    // no difference if called pre-intent / post-trigger word
    direction = GetDirectionFromMicHistory();
  }

  return direction;
}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionIndex BehaviorReactToVoiceCommand::GetDirectionFromMicHistory() const
{
  const MicDirectionHistory& history = GetBEI().GetMicComponent().GetMicDirectionHistory();
  const auto& recentDirection = history.GetRecentDirection();
  return recentDirection;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnStreamingBegin()
{
  PRINT_CH_INFO("MicData", "BehaviorReactToVoiceCommand.OnStreamingBegin",
                "Got notice that cloud stream is open");
  _dVars.streamingBeginTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::StartListening()
{
  _dVars.state = EState::Listening;

  // to get into our listening state, we need to play our get-in anim followed by our
  // looping animation.
  OnVictorListeningBegin();

  // we don't want to enter EState::Listening until we're in our loop or else
  // we could end up exiting too soon and looking like garbage
  auto callback = [this]()
  {
    // if for some reason it doesn't look like streaming has begun yet (we didn't get a call from vic-cloud
    // saying the stream is open) consider the elapsed time to be 0
    float elapsed = 0.0f;
    if( _dVars.streamingBeginTime > 0.0f ) {
      elapsed = ( BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()
                  - _dVars.streamingBeginTime );
    }

    const float timeout = ( kMaxRecordTime_s - elapsed );
    DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::VC_ListeningLoop,
                                                     0, true, (uint8_t)AnimTrackFlag::NO_TRACKS,
                                                     std::max( timeout, 1.0f ) ),
                         &BehaviorReactToVoiceCommand::StopListening );
  };

  DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::VC_ListeningGetIn ), callback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::StopListening()
{
  ASSERT_NAMED_EVENT( _dVars.state == EState::Listening,
                      "BehaviorReactToVoiceCommand.State",
                      "Transitioning to EState::IntentReceived from invalid state [%hhu]", _dVars.state );

  UpdateUserIntentStatus();
  TransitionToThinking();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnVictorListeningBegin()
{
  using namespace AudioMetaData::GameEvent;

  static const BackpackLightAnimation::BackpackAnimation kStreamingLights =
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
    BackpackLightComponent& blc = GetBEI().GetBackpackLightComponent();
    blc.StartLoopingBackpackAnimation( kStreamingLights, BackpackLightSource::Behavior, _dVars.lightsHandle );
  }

  if ( GenericEvent::Invalid != _iVars.earConBegin )
  {
    // Play earcon begin audio
    GetBEI().GetRobotAudioClient().PostEvent( _iVars.earConBegin, AudioMetaData::GameObjectType::Behavior );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnVictorListeningEnd()
{
  using namespace AudioMetaData::GameEvent;

  if ( _iVars.backpackLights && _dVars.lightsHandle.IsValid() )
  {
    BackpackLightComponent& blc = GetBEI().GetBackpackLightComponent();
    blc.StopLoopingBackpackAnimation( _dVars.lightsHandle );
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
void BehaviorReactToVoiceCommand::UpdateUserIntentStatus()
{
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
  if ( ( _dVars.intentStatus == EIntentStatus::NoIntentHeard ) && uic.IsAnyUserIntentPending() )
  {
    // next behavior is going to deal with the intent, but we still have more shit to do
    uic.SetUserIntentTimeoutEnabled( false );

    _dVars.intentStatus = EIntentStatus::IntentHeard;

    PRINT_CH_DEBUG("MicData", "BehaviorReactToVoiceCommand.UpdateUserIntentStatus.Heard",
                   "Heard an intent");

    static const UserIntentTag unmatched = USER_INTENT(unmatched_intent);
    if ( uic.IsUserIntentPending( unmatched ) )
    {
      SmartActivateUserIntent( unmatched );
      _dVars.intentStatus = EIntentStatus::IntentUnknown;
      PRINT_CH_DEBUG("MicData", "BehaviorReactToVoiceCommand.UpdateUserIntentStatus.Unknown",
                     "Heard an intent, but it was unknown");
    }
  }
  else if( uic.WasUserIntentError() ) {
    uic.ResetUserIntentError();
    _dVars.intentStatus = EIntentStatus::Error;
    PRINT_CH_DEBUG("MicData", "BehaviorReactToVoiceCommand.UpdateUserIntentStatus.Error",
                   "latest user intent was an error");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::TransitionToThinking()
{
  _dVars.state = EState::Thinking;

  PRINT_CH_DEBUG("MicData", "BehaviorReactToVoiceCommand.TransitionToThinking", "Thinking state starting");

  auto callback = [this]()
  {
    // we're keeping our "listening feedback" open until the last possible moment, since the intent can come
    // in after we've closed our recording stream.
    OnVictorListeningEnd();

    // Play a reaction behavior if we were told to ...
    // ** only in the case that we've heard a valid intent **
    UpdateUserIntentStatus();
    const bool heardValidIntent = ( _dVars.intentStatus == EIntentStatus::IntentHeard );
    if ( heardValidIntent && _iVars.reactionBehavior )
    {
      if( IsTurnEnabled() ) {
        const MicDirectionIndex triggerDirection = GetReactionDirection();
        PRINT_NAMED_INFO("BehaviorReactToVoiceCommand.TransitionToThinking.ReactionDirection","%d",triggerDirection);
        _iVars.reactionBehavior->SetReactDirection( triggerDirection );

        PRINT_CH_DEBUG("MicData", "BehaviorReactToVoiceCommand.Thinking.SetReactionDirection",
                       "Setting reaction behavior direction to [%d]",
                       (int)triggerDirection);

        // allow the reaction to not want to run in certain directions/states
        if ( _iVars.reactionBehavior->WantsToBeActivated() )
        {
          DelegateIfInControl( _iVars.reactionBehavior.get(), &BehaviorReactToVoiceCommand::TransitionToIntentReceived );
        }
        else
        {
          PRINT_CH_DEBUG("BehaviorReactToVoiceCommand.Thinking.ReactionDoesntWantToActivate",
                         "%s: intent reaction behavior '%s' doesn't want to activate",
                         GetDebugLabel().c_str(),
                         _iVars.reactionBehavior->GetDebugLabel().c_str());
        }
      }
      else
      {
        PRINT_CH_DEBUG("MicData", "BehaviorReactToVoiceCommand.Thinking.TurnDisabled",
                       "Turn after intent is received has been disabled this tick, skipping");
      }
    }

    if ( !IsControlDelegated() )
    {
      // handle intent now
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

  switch ( _dVars.intentStatus )
  {
    case EIntentStatus::IntentHeard: {
      // no animation for valid intent, go straight into the intent behavior
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent.Heard", "Heard valid user intent, woot!" );

      // also reset the attention transfer counter for unmatched intents (since we got a good one)
      GetBehaviorComp<AttentionTransferComponent>().ResetAttentionTransfer(AttentionTransferReason::UnmatchedIntent);
      break;
    }

    case EIntentStatus::IntentUnknown: {
      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent.Unknown",
                      "Heard user intent but could not understand it" );
      GetBEI().GetMoodManager().TriggerEmotionEvent( "NoValidVoiceIntent" );
      if( _iVars.unmatchedIntentBehavior->WantsToBeActivated() ) {
        // this behavior will (should) interact directly with the attention transfer component
        DelegateIfInControl(_iVars.unmatchedIntentBehavior.get());
      }
      break;
    }

    case EIntentStatus::NoIntentHeard:
    case EIntentStatus::Error: {

      PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent.Error",
                      "Intent processing returned an error (or timeout)" );
      GetBEI().GetMoodManager().TriggerEmotionEvent( "NoValidVoiceIntent" );

      // track that an error occurred
      _iVars.cloudErrorTracker.AddOccurrence();

      if( _iVars.cloudErrorHandle &&
          _iVars.cloudErrorHandle->AreConditionsMet() ) {
        // time to let the user know

        auto errorBehaviorCallback = [this]() {
          const bool updateNow = true;
          const bool hasSSID = !OSState::getInstance()->GetSSID(updateNow).empty();

          if( hasSSID ) {
            PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent.Error.NoCloud",
                            "has wifi, so error must be on the internet or cloud side" );

            // has wifi, but no cloud
            if( ANKI_VERIFY( _iVars.noCloudBehavior && _iVars.noCloudBehavior->WantsToBeActivated(),
                             "BehaviorReactToVoiceCommand.Intent.Error.NoCloud.BehaviorWontActivate",
                             "No cloud behavior '%s' doesn't want to be activated",
                             _iVars.noCloudBehavior ? _iVars.noCloudBehavior->GetDebugLabel().c_str() : "<NULL>" ) ) {
              // this behavior will (should) interact directly with the attention transfer component
              DelegateIfInControl(_iVars.noCloudBehavior.get());
            }
          }
          else {
            PRINT_CH_DEBUG( "MicData", "BehaviorReactToVoiceCommand.Intent.Error.NoWifi",
                            "no wifi SSID, error is local" );

            // no wifi
            if( ANKI_VERIFY( _iVars.noWifiBehavior && _iVars.noWifiBehavior->WantsToBeActivated(),
                             "BehaviorReactToVoiceCommand.Intent.Error.NoWifi.BehaviorWontActivate",
                             "No wifi behavior '%s' doesn't want to be activated",
                             _iVars.noWifiBehavior ? _iVars.noWifiBehavior->GetDebugLabel().c_str() : "<NULL>" ) ) {
              // this behavior will (should) interact directly with the attention transfer component
              DelegateIfInControl(_iVars.noWifiBehavior.get());
            }
          }
        };

        const MicDirectionIndex triggerDirection = GetReactionDirection();
        _iVars.reactionBehavior->SetReactDirection( triggerDirection );

        PRINT_CH_DEBUG("MicData", "BehaviorReactToVoiceCommand.Intent.Error.SetReactionDirection",
                       "Setting reaction behavior direction to [%d]",
                       (int)triggerDirection);

        // allow the reaction to not want to run in certain directions/states
        if ( _iVars.reactionBehavior->WantsToBeActivated() )
        {
          DelegateIfInControl( _iVars.reactionBehavior.get(), errorBehaviorCallback );
        }
        else
        {
          PRINT_CH_DEBUG("BehaviorReactToVoiceCommand.Intent.Error",
                         "%s: intent reaction behavior '%s' doesn't want to activate (in case of intent error)",
                         GetDebugLabel().c_str(),
                         _iVars.reactionBehavior->GetDebugLabel().c_str());
          errorBehaviorCallback();
        }
      }
      else {
        // not time to tell the user, just play the normal unheard animation
        DelegateIfInControl( new TriggerLiftSafeAnimationAction( AnimationTrigger::VC_IntentNeutral ) );
      }
      break;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToVoiceCommand::IsTurnEnabled() const
{
  const auto ts = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  return ts != _dVars.timestampToDisableTurnFor;
}


} // namespace Cozmo
} // namespace Anki
