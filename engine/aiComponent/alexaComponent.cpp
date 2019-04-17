/**
* File: alexaComponent.cpp
*
* Author: ross
* Created: Oct 16 2018
*
* Description: Component that manages Alexa interactions among anim, engine, and app
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/alexaComponent.h"

#include "audioEngine/multiplexer/audioCladMessageHelper.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/animationComponent.h"
#include "engine/components/settingsManager.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "proto/external_interface/shared.pb.h"
#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"
#include "util/string/stringUtils.h"

namespace Anki {
namespace Vector {
  
namespace {
  #define LOG_CHANNEL "Alexa"
  
  // This component also handles the VC for sign in and sign out to hide from any behaviors whether it was
  // app or voice command that requested sign in/out, since app intents got cut.
  const UserIntentTag kSignInIntent = USER_INTENT(amazon_signin);
  const UserIntentTag kSignOutIntent = USER_INTENT(amazon_signout);
  
  // If a request to sign in/out is not claimed in this many ticks, this component will perform the sign in/out.
  // Note that voice command-initiated requests wait until the intent is cleared in the uic before signing in/out,
  // so that BehaviorReactToVoiceCommand has time to finish
  const float kRequestDelayTicks = 4;
  
  const float kAnimTimeout_s = 3.0f;
  const AnimationTag kInvalidAnimationTag = AnimationComponent::GetInvalidTag();
  
  // this class assumes that Listening, Thinking, Speaking, and Error are 0,1,2,3 when sending indices to animProcess
  // and when getting a tag array from AnimationComponent, so it doesn't need to know about AlexaUXState.
  // IMPORTANT: If you hit this, please also change AnimationComponent's _tagForAlexa*, which only use indices and
  // not AlexaUXState's.
  static_assert( static_cast<uint8_t>(AlexaUXState::Listening) == 0, "Expected 0");
  static_assert( static_cast<uint8_t>(AlexaUXState::Thinking)  == 1, "Expected 1");
  static_assert( static_cast<uint8_t>(AlexaUXState::Speaking)  == 2, "Expected 2");
  static_assert( static_cast<uint8_t>(AlexaUXState::Error)     == 3, "Expected 3");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaComponent::AlexaComponent(Robot& robot)
  : IDependencyManagedComponent<AIComponentID>(this, AIComponentID::AlexaComponent)
  , _robot(robot)
  , _authState( AlexaAuthState::Uninitialized )
  , _uxState( AlexaUXState::Idle )
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::InitDependent(Robot *robot, const AICompMap& dependentComps)
{
  // setup event handlers
  
  if( robot->HasGatewayInterface() ) {
    auto* gi = robot->GetGatewayInterface();
    auto callback = std::bind( &AlexaComponent::HandleAppEvents, this, std::placeholders::_1 );
    _signalHandles.push_back( gi->Subscribe( external_interface::GatewayWrapperTag::kAlexaAuthStateRequest, callback ) );
    _signalHandles.push_back( gi->Subscribe( external_interface::GatewayWrapperTag::kAlexaOptInRequest, callback ) );
    _signalHandles.push_back( gi->Subscribe( external_interface::GatewayWrapperTag::kAppDisconnected, callback ) );
    _signalHandles.push_back( gi->Subscribe( external_interface::GatewayWrapperTag::kOnboardingStateRequest, callback ) );
  }
  auto* ri = robot->GetRobotMessageHandler();
  if( ri != nullptr ) {
    auto callback = std::bind( &AlexaComponent::HandleAnimEvents, this, std::placeholders::_1 );
    _signalHandles.push_back( ri->Subscribe( RobotInterface::RobotToEngineTag::alexaAuthChanged, callback ) );
    _signalHandles.push_back( ri->Subscribe( RobotInterface::RobotToEngineTag::alexaUXChanged, callback ) );
  }
  auto forceOptIn = [this](ConsoleFunctionContextRef context) {
    SetRequest( Request::SignInApp );
  };
  _consoleFuncs.emplace_front( "ForceAlexaOptIn", std::move(forceOptIn), "Alexa", "" );
  auto forceOptOut = [this](ConsoleFunctionContextRef context) {
    SetRequest( Request::SignOutApp );
  };
  _consoleFuncs.emplace_front( "ForceAlexaOptOut", std::move(forceOptOut), "Alexa", "" );
  
  
  
  // check Alexa feature flag
  const auto* ctx = robot->GetContext();
  if( ctx != nullptr ) {
    const auto* featureGate = ctx->GetFeatureGate();
    _featureFlagEnabled = featureGate->IsFeatureEnabled( FeatureType::Alexa );
    // if !_featureFlagEnabled, now would be a good time to tell anim that alexa is disabled, just in case the
    // feature flag was disabled, but animProcess still has a file indicating that we used to be
    // authenticated. However, messaging to anim might not be init'd yet. Instead, SendAuthStateToApp will
    // check the feature flag and disable alexa if it some how ends up initialized without the feature flag.
  }
  
  // setup anim tags for getins to various ux states
  auto& animComponent = robot->GetAnimationComponent();
  _animTags = animComponent.SetAlexaUXResponseCallback([this](unsigned int idx, bool playing){
    const auto uxState = static_cast<AlexaUXState>(idx);
    if( playing ) {
      // only set to wait if this animation was passed as a get-in. the same anim may be used elsewhere
      _uxResponseInfo[uxState].SetWaiting( _uxResponseInfo[uxState].hasAnim );
    } else {
      _uxResponseInfo[uxState].SetWaiting( false );
    }
  });

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastUxStateTransition_s = currTime_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::AdditionalUpdateAccessibleComponents(AICompIDSet& components) const
{
  components.insert( AIComponentID::BehaviorComponent );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::UpdateDependent(const AICompMap& dependentComps)
{
  
  // no behavior handles the intents kSignInIntent or kSignOutIntent. We just message the anim process
  // of the request to sign in/out, and depending on its state, it will either not do anything, or
  // send a message back to the engine switch stacks into a face screen-style Wait behavior for alexa pairing
  if( _uic == nullptr ) {
    auto& BC = dependentComps.GetComponent<BehaviorComponent>();
    _uic = &BC.GetComponent<UserIntentComponent>();
  }
  
  if( _uic->IsUserIntentPending(kSignInIntent) ) {
    if( _featureFlagEnabled ) {
      SetRequest( Request::SignInVC );
    }
  } else if( _uic->IsUserIntentPending(kSignOutIntent) ) {
    if( _featureFlagEnabled ) {
      SetRequest( Request::SignOutVC );
    }
  }
  
  // check timeout for waitingForGetInCompletion
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  for( auto& response : _uxResponseInfo ) {
    if( (response.second.timeout_s > 0.0f) && (currTime_s > response.second.timeout_s) ) {
      response.second.SetWaiting( false );
    }
  }
  
  if( (_requestTimeout != 0) && (_requestTimeout < BaseStationTimer::getInstance()->GetTickCount()) ) {
    // Clear app requests when the timer expires.
    // Clear VC requests when the intent is gone from the uic ==> this implies it expired in the uic, or
    // someone else handled the intent. We have to wait at least as long as the uic timeout, because
    // that timeout is dynamically adjusted by BehaviorReactToVoiceCommand
    if( _request == Request::SignInApp ) {
      SignIn();
    } else if( _request == Request::SignOutApp ) {
      SignOut();
    } else if( (_request == Request::SignInVC) && !_uic->IsUserIntentPending(kSignInIntent) ) {
      SignIn();
    } else if( (_request == Request::SignOutVC) && !_uic->IsUserIntentPending(kSignOutIntent) ) {
      SignOut();
    }
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaComponent::IsSignInPending() const
{
  const bool pending = (_request == Request::SignInApp) || (_request == Request::SignInVC);
  return pending && (_requestTimeout != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaComponent::IsSignOutPending() const
{
  const bool pending = (_request == Request::SignOutApp) || (_request == Request::SignOutVC);
  return pending && (_requestTimeout != 0);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::ClaimRequest()
{
  _requestTimeout = 0;
  if( _uic != nullptr ) {
    if( (_request == Request::SignInVC) && _uic->IsUserIntentPending(kSignInIntent) ) {
      _uic->DropUserIntent( kSignInIntent );
    } else if( (_request == Request::SignOutVC) && _uic->IsUserIntentPending(kSignOutIntent) ) {
      _uic->DropUserIntent( kSignOutIntent );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SignIn()
{
  _request = Request::None;
  _requestTimeout = 0;
  SetAlexaOption( true );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SignOut()
{
  _request = Request::None;
  _requestTimeout = 0;
  SetAlexaOption( false );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SetAlexaOption( bool optedIn )
{
  _pendingAuthIsFromOptIn = optedIn;
  // send anim message to opt IN/OUT
  _robot.SendMessage( RobotInterface::EngineToRobot( RobotInterface::SetAlexaUsage(optedIn) ) );

  if( !optedIn ) {
    // make sure backpack button setting is back to hey vector
    // TEMP: DAS for this
    static const bool kButtonIsAlexa = false;
    ToggleButtonWakewordSetting( kButtonIsAlexa );
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::HandleAppEvents( const AnkiEvent<external_interface::GatewayWrapper>& event )
{
  const auto& msg = event.GetData();
  switch( msg.GetTag() ) {
    case external_interface::GatewayWrapperTag::kAlexaAuthStateRequest:
    {
      // respond to the app with the latest and greatest auth state received from anim
      const bool isResponse = true;
      SendAuthStateToApp( isResponse );
    }
      break;
    case external_interface::GatewayWrapperTag::kAlexaOptInRequest:
    {
      if( _featureFlagEnabled ) {
        const bool optIn = msg.alexa_opt_in_request().opt_in();
        const auto request = optIn ? Request::SignInApp : Request::SignOutApp;
        SetRequest( request );
      }
    }
      break;
    default:
      break;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::HandleAnimEvents( const AnkiEvent<RobotInterface::RobotToEngine>& event )
{
  const auto& msg = event.GetData();
  if( msg.GetTag() == RobotInterface::RobotToEngineTag::alexaAuthChanged ) {
    const auto newState = msg.Get_alexaAuthChanged().state;
    const auto newExtra = msg.Get_alexaAuthChanged().extra;
    
    // tie button functionality to auth state even if it doesn't change
    if( (newState == AlexaAuthState::Authorized) && _pendingAuthIsFromOptIn ) {
      // the user opted in, and now we're authorized. Design calls for switching the backpack button
      // functionality to Alexa, so save that setting now. Note that the app may be displaying a
      // successful login screen that contains a toggle for this setting. In case things occur out of
      // order, the app assumes that a successful authentication implies that the setting is now Alexa.
      // If the below code is removed, the app will need to change as well
      _pendingAuthIsFromOptIn = false;
      static const bool kButtonIsAlexa = true;
      ToggleButtonWakewordSetting( kButtonIsAlexa );
    } else if( newState == AlexaAuthState::Uninitialized ) {
      static const bool kButtonIsAlexa = false;
      ToggleButtonWakewordSetting( kButtonIsAlexa );
    }
    
    if( (newState != _authState) || (newExtra != _authStateExtra) ) {
      _authState = newState;
      _authStateExtra = newExtra;
      const bool isResponse = false;
      SendAuthStateToApp( isResponse );
    }
  } else if( msg.GetTag() == RobotInterface::RobotToEngineTag::alexaUXChanged ) {
    HandleNewUXState( msg.Get_alexaUXChanged().state );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::ToggleButtonWakewordSetting( bool isAlexa ) const
{
  auto* settings = _robot.GetComponentPtr<SettingsManager>();
  const bool updateSettingsJdoc = true;
  bool ignoredDueToNoChange;
  const auto newSetting = isAlexa ? external_interface::BUTTON_WAKEWORD_ALEXA : external_interface::BUTTON_WAKEWORD_HEY_VECTOR;
  const bool success = settings->SetRobotSetting( external_interface::RobotSetting::button_wakeword,
                                                  newSetting,
                                                  updateSettingsJdoc,
                                                  ignoredDueToNoChange );
  ANKI_VERIFY( success || ignoredDueToNoChange,
               "AlexaComponent.HandleAnimEvents.SettingFail",
               "Could not set the functionality of the backpack button to alexa" );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SendAuthStateToApp( bool isResponse )
{
  // quick sanity check for feature flag and alexa state
  if( _authState != AlexaAuthState::Uninitialized ) {
    if( !ANKI_VERIFY( _featureFlagEnabled,
                      "AlexaComponent.SendAuthStateToApp.BadFlag",
                      "Feature flag was disabled but the alexa state is '%s'",
                      AlexaAuthStateToString(_authState) ) ) {
      // try disabling alexa
      SignOut();
      SendSignOutDAS( UserIntentSource::Unknown );
      return;
    }
  }
  
  // message the app
  if( _robot.HasGatewayInterface() ) {
    auto* gi = _robot.GetGatewayInterface();
    if( isResponse ) {
      auto* response = new external_interface::AlexaAuthStateResponse;
      const external_interface::AlexaAuthState authState = CladProtoTypeTranslator::ToProtoEnum( _authState );
      response->set_auth_state( authState );
      if( authState == external_interface::ALEXA_AUTH_WAITING_FOR_CODE ) {
        response->set_extra( _authStateExtra );
      } else {
        ANKI_VERIFY( _authStateExtra.empty(),
                     "AlexaComponent.SendAuthStateToApp.NonEmptyExtra",
                     "Unexpected extra data '%s'",
                     _authStateExtra.c_str() );
      }
      gi->Broadcast( ExternalMessageRouter::WrapResponse( response ) );
    } else {
      auto* event = new external_interface::AlexaAuthEvent;
      const external_interface::AlexaAuthState authState = CladProtoTypeTranslator::ToProtoEnum( _authState );
      event->set_auth_state( authState );
      if( authState == external_interface::ALEXA_AUTH_WAITING_FOR_CODE ) {
        event->set_extra( _authStateExtra );
      } else {
        ANKI_VERIFY( _authStateExtra.empty(),
                     "AlexaComponent.SendAuthStateToApp.NonEmptyExtra",
                     "Unexpected extra data '%s'",
                     _authStateExtra.c_str() );
      }
      gi->Broadcast( ExternalMessageRouter::Wrap( event ) );
    }
    
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SetAlexaUXResponses( const std::unordered_map<AlexaUXState,AlexaUXResponse>& responses )
{
  namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
  
  _uxResponseInfo.clear();
  
  // initially reset to nothing
  RobotInterface::SetAlexaUXResponses msg;
  msg.getInAnimTags = {{kInvalidAnimationTag}};
  msg.postAudioEvents = {{AECH::CreatePostAudioEvent( AudioMetaData::GameEvent::GenericEvent::Invalid,
                                                      AudioMetaData::GameObjectType::Behavior,
                                                      0 )}};
  // msg.csvGetInAnimNames is set below
  
  std::vector<std::string> animNames;
  static_assert( AlexaUXStateNumEntries == 5, "New states need changing the size below (decremented by one)" );
  animNames.resize(4); // don't include Idle
  for( const auto& response : responses ) {
    const unsigned int idx = static_cast<unsigned int>(response.first);
    if( !ANKI_VERIFY( (idx < 4), "AlexaComponent.SetAlexaUXResponses.Invalid",
                      "Invalid state %hhu. Allowed values are Listening, Speaking, Thinking, Error", response.first ) )
    {
      continue;
    }
    
    const std::string animName = GetAnimName( response.second.animTrigger );
    const bool hasAnim = !animName.empty();
    const bool hasAudio = (response.second.audioEvent != AudioMetaData::GameEvent::GenericEvent::Invalid);
    
    if( !hasAudio && !hasAnim ) {
      LOG_WARNING("AlexaComponent.SetAlexaUXResponses.Unnecessary",
                  "You should just supply those states you want to have a response");
      continue;
    }
    
    msg.postAudioEvents[idx] = AECH::CreatePostAudioEvent( response.second.audioEvent,
                                                           AudioMetaData::GameObjectType::Behavior,
                                                           0 );
    
    _uxResponseInfo[response.first].hasAnim = hasAnim;
    // always send this tag when there is any response, even when there is no anim, so animProc knows
    // if there is a valid response
    msg.getInAnimTags[idx] = _animTags[idx];
    animNames[idx] = animName;
  }
  msg.csvGetInAnimNames = Util::StringJoin(animNames, ',');

  // TODO (VIC-11517): downgrade. for now this is useful in webots
  PRINT_NAMED_WARNING("AlexaComponent.SetUXResponses", "new response get-ins: %s", msg.csvGetInAnimNames.c_str());

  
  _robot.SendMessage( RobotInterface::EngineToRobot( std::move(msg) ) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::HandleNewUXState( AlexaUXState state )
{
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  PRINT_NAMED_WARNING("AlexaComponent.HandleNewUXState", "State=%s", AlexaUXStateToString(state) );


  if( _uxState != state ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float lastStateTime_s = currTime_s - _lastUxStateTransition_s;

    DASMSG(ux_state_msg, "alexa.ux_state_transition", "The alexa agent changed UX states");
    DASMSG_SET(s1, AlexaUXStateToString(state), "new UX state (AlexaUXState in alexaTypes.clad)");
    DASMSG_SET(s2, AlexaUXStateToString(_uxState), "old UX state (AlexaUXState in alexaTypes.clad)");
    DASMSG_SET(i1, ((int)(lastStateTime_s * 1000.0f)), "Time (in milliseconds) the last ux state was active");
    DASMSG_SEND();

    _lastUxStateTransition_s = currTime_s;
  }
  
  if( (_uxState == AlexaUXState::Idle) && (_uxState != state) ) {
    // alexa exited from idle. the anim process is likely playing some getin right now if we pushed one.
    auto it = _uxResponseInfo.find( state );
    if( it != _uxResponseInfo.end() ) {
      const bool shouldWait = it->second.hasAnim;
      it->second.SetWaiting( shouldWait );
    }
  }
  _uxState = state;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaComponent::IsIdle() const
{
  const bool isIdle = (_uxState == AlexaUXState::Idle);
  return isIdle;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaComponent::IsUXStateGetInPlaying( AlexaUXState state ) const
{
  const auto it = _uxResponseInfo.find( state );
  if( it != _uxResponseInfo.end() ) {
    return (it->second.hasAnim && it->second.waitingForGetInCompletion);
  } else {
    return false;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaComponent::IsAnyUXStateGetInPlaying() const
{
  for( const auto& entry : _uxResponseInfo ) {
    if( entry.second.hasAnim && entry.second.waitingForGetInCompletion ) {
      return true;
    }
  }
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string AlexaComponent::GetAnimName( AnimationTrigger trigger ) const
{
  std::string animName;
  const auto* dataLoader = _robot.GetContext()->GetDataLoader();
  if( dataLoader->HasAnimationForTrigger( trigger ) ) {
    const auto groupName = dataLoader->GetAnimationForTrigger( trigger );
    if( !groupName.empty() ) {
      animName = _robot.GetAnimationComponent().GetAnimationNameFromGroup( groupName );
      if( animName.empty() ) {
        LOG_WARNING( "AlexaComponent.GetAnimName.AnimationNotFound",
                     "No animation returned for group %s",
                     groupName.c_str() );
      }
    } else {
      LOG_WARNING( "AlexaComponent.GetAnimName.GroupNotFound",
                   "Group not found for trigger %s",
                   AnimationTriggerToString( trigger ) );
    }
  }
  return animName;
}
  
void AlexaComponent::AlexaUXResponseInfo::SetWaiting( bool waiting )
{
  if( waiting ) {
    waitingForGetInCompletion = true;
    // if the ux state change happens before anim has received our new anim response, anim and engine
    // will be out of sync. Just in case, make sure this times out in a few seconds.
    timeout_s = kAnimTimeout_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  } else {
    waitingForGetInCompletion = false;
    timeout_s = -1.0f;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SetRequest( Request request )
{
  if( (_request == Request::None) && (request != Request::None) ) {
    _request = request;
    _requestTimeout = kRequestDelayTicks + BaseStationTimer::getInstance()->GetTickCount();

    switch( _request ) {
      case Request::SignInApp:
        SendSignInDAS( UserIntentSource::App );
        break;
      case Request::SignOutApp:
        SendSignOutDAS( UserIntentSource::App );
        break;
      case Request::SignInVC:
        SendSignInDAS( UserIntentSource::Voice );
        break;
      case Request::SignOutVC:
        SendSignOutDAS( UserIntentSource::Voice );
        break;
      case Request::None:
        break;
    }
  } else {
    LOG_WARNING( "AlexaComponent.AddRequest.NotHandled",
                 "Request of type %d dropped because %d is already pending or request is none",
                 (int)request, (int)_request );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SendSignInDAS( UserIntentSource source ) const
{
  DASMSG(alexa_opt_in, "alexa.user_sign_in_attempt", "User attempted to sign in to alexa");
  DASMSG_SET(s1, UserIntentSourceToString( source ), "source of the request (Voice or App)");
  DASMSG_SEND();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SendSignOutDAS( UserIntentSource source ) const
{
  DASMSG(alexa_opt_in, "alexa.user_sign_out_attempt", "User attempted to sign out of alexa");
  DASMSG_SET(s1, UserIntentSourceToString( source ), "source of the request (Voice or App)");
  DASMSG_SEND();
}
  
} // namespace Vector
} // namespace Anki
