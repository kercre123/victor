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

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "proto/external_interface/shared.pb.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
  
namespace {
  const UserIntentTag kSignInIntent = USER_INTENT(amazon_signin);
  const UserIntentTag kSignOutIntent = USER_INTENT(amazon_signout);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaComponent::AlexaComponent(Robot& robot)
  : IDependencyManagedComponent<AIComponentID>(this, AIComponentID::AlexaComponent)
  , _robot(robot)
  , _authState( AlexaAuthState::Uninitialized )
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
    SetAlexaOption( true );
  };
  _consoleFuncs.emplace_front( "ForceAlexaOptIn", std::move(forceOptIn), "Alexa", "" );
  auto forceOptOut = [this](ConsoleFunctionContextRef context) {
    SetAlexaOption( false );
  };
  _consoleFuncs.emplace_front( "ForceAlexaOptOut", std::move(forceOptOut), "Alexa", "" );
  
  
  
  // check Alexa feature flag
  const auto* ctx = robot->GetContext();
  if( ctx != nullptr ) {
    const auto* featureGate = ctx->GetFeatureGate();
    _featureFlagEnabled = featureGate->IsFeatureEnabled( FeatureType::Alexa );
  }
  if( !_featureFlagEnabled ) {
    // just in case the feature flag changed, but animProcess still has a file indicating that we used to be
    // authenticated, force logout of alexa
    SetAlexaOption( false );
  }
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
  auto& BC = dependentComps.GetComponent<BehaviorComponent>();
  auto& uic = BC.GetComponent<UserIntentComponent>();
  
  if( uic.IsUserIntentPending(kSignInIntent) ) {
    if( _featureFlagEnabled ) {
      SetAlexaOption( true );
    }
    uic.DropUserIntent( kSignInIntent );
  } else if( uic.IsUserIntentPending(kSignOutIntent) ) {
    if( _featureFlagEnabled ) {
      SetAlexaOption( false );
    }
    uic.DropUserIntent( kSignOutIntent );
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SetAlexaOption( bool optedIn ) const
{
  // send anim message to opt IN/OUT
  _robot.SendMessage( RobotInterface::EngineToRobot( RobotInterface::SetAlexaUsage(optedIn) ) );
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
        // tell anim process the new auth state
        const bool optIn = msg.alexa_opt_in_request().opt_in();
        SetAlexaOption( optIn );
      }
    }
      break;
    case external_interface::GatewayWrapperTag::kAppDisconnected:
    {
      // tell anim process to cancel any pending auth, but remain authenticated if already authenticated
      SendCancelPendingAuth();
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
    if( (newState != _authState) || (newExtra != _authStateExtra) ) {
      _authState = msg.Get_alexaAuthChanged().state;
      _authStateExtra = msg.Get_alexaAuthChanged().extra;
      const bool isResponse = false;
      SendAuthStateToApp( isResponse );
    }
  } else if( msg.GetTag() == RobotInterface::RobotToEngineTag::alexaUXChanged ) {
    const auto& state = msg.Get_alexaUXChanged().state;
    // TODO: cache so behaviors can access it. for now, print so it can be seen in webots
    PRINT_NAMED_WARNING("AlexaComponent.HandleAnimEvents.UXState", "State=%s", AlexaUXStateToString(state) );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaComponent::SendAuthStateToApp( bool isResponse ) const
{
  // quick sanity check for feature flag and alexa state
  if( _authState != AlexaAuthState::Uninitialized ) {
    if( !ANKI_VERIFY( _featureFlagEnabled,
                      "AlexaComponent.SendAuthStateToApp.BadFlag",
                      "Feature flag was disabled but the alexa state is '%s'",
                      AlexaAuthStateToString(_authState) ) ) {
      // try disabling alexa
      SetAlexaOption( false );
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
void AlexaComponent::SendCancelPendingAuth() const
{
  // send anim message to cancel pending authorization
  _robot.SendMessage( RobotInterface::EngineToRobot( RobotInterface::CancelPendingAlexaAuth() ) );
}

  
} // namespace Vector
} // namespace Anki
