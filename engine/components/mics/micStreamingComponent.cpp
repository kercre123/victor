/***********************************************************************************************************************
 *
 *  MicStreamingComponent .cpp
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 3/22/2019
 *  Copyright Anki, Inc. 2019
 *
 **********************************************************************************************************************/

// #include "engine/components/mics/micStreamingComponent.h"
#include "micStreamingComponent.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/types/animationTrigger.h"
#include "clad/audio/audioEventTypes.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"
#include "util/global/globalDefinitions.h"

// todo: jmeh
// + change all required INFO's back to DEBUG's
// + how to handle multiple behaviors wanting to respond to trigger word ... this is going to depend on how
//   we end up using the system; if there's only a single behavior then we don't care
//   - keep old "pending trigger word" model
//   - have a "TriggerWordHandled" event
//   - stack (earliest wins)
//   - priorities
//   - only ONE to rule them all
// + other non-wakeword streaming needs to use this as well
//   - BehaviorPromptUserForVoiceCommand
//   - BehaviorKnowledgeGraphQuestion
// + does wakewordless streaming even have a concept of a get-in animation?
//   - I suppose in practice this would never happen, but what if a trigger behavior "disabled" recognizer response?
//   - looks like BehaviorPromptUserForVoiceCommand does for some behaviors (when playListeningGetIn is true)
// + need to be able to unregister for callbacks (use handles like I did for the sound reaction stuff)
// + note: EmergencyModeTriggerWord did not push a trigger word response; probably doesn't want to respond?
// + kill UserIntentComponent::_disableTriggerWordNames and all things trigger word
// + move the hard coded recognizer response strings in behavior to configs


#define LOG_CHANNEL "Microphones"

namespace {
  const std::string   kConfigFilename                   = "config/micData/micStreamingConfig.json";

  const char*         kKeyResponsesField                = "recognizer_responses";
  const char*         kKeyResponseAnimLoop              = "anim_trigger_loop";
  const char*         kKeyResponseAnimGetOut            = "anim_trigger_get_out";
  const char*         kKeyResponseEarconSuccess         = "earcon_success";
  const char*         kKeyResponseEarconFail            = "earcon_fail";
}

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicStreamingComponent::MicStreamingComponent()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicStreamingComponent::~MicStreamingComponent()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::Initialize( Robot* robot )
{
  _robot = robot;
  DEV_ASSERT( ( nullptr != _robot ), "MicStreamingComponent.Initialize" );

  // register for message callbacks
  {
    using namespace RobotInterface;
    MessageHandler* messageHandler = _robot->GetRobotMessageHandler();

    AddSignalHandle( messageHandler->Subscribe( RobotToEngineTag::triggerWordDetected,
                     std::bind( &MicStreamingComponent::HandleTriggerWordDetectedEvent, this, std::placeholders::_1 ) ) );

    AddSignalHandle( messageHandler->Subscribe( RobotToEngineTag::micStateSync,
                     std::bind( &MicStreamingComponent::HandleStreamingStateSyncEvent, this, std::placeholders::_1 ) ) );
  }

  // load all of our pre-set streaming response configuarations
  {
    using namespace Util;
    Json::Value streamingConfig;

    const Data::DataPlatform* platform = _robot->GetContextDataPlatform();
    if ( platform->readAsJson( Data::Scope::Resources, kConfigFilename, streamingConfig ) )
    {
      LoadStreamingReactions( streamingConfig );
    }
    else
    {
      LOG_ERROR( "MicStreamingComponent.Initialize",
                 "Error loading mic streaming response config file (%s), check the formatting (extra comma, etc)",
                 kConfigFilename.c_str() );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::LoadStreamingReactions( const Json::Value& config )
{
  const Json::Value& allResponsesConfig = config[kKeyResponsesField];
  for( Json::Value::const_iterator it = allResponsesConfig.begin() ; it != allResponsesConfig.end() ; it++ )
  {
    const std::string id = it.key().asString();
    RecognizerBehaviorResponse streamingResponse = LoadRecognizerBehaviorResponse( *it, id );

    _streamingResponseMap.emplace( id, streamingResponse );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RecognizerBehaviorResponse MicStreamingComponent::LoadRecognizerBehaviorResponse( const Json::Value& config, const std::string& id )
{
  RecognizerBehaviorResponse streamingResponse;

  // load our anim responses
  {
    // load the AnimationTrigger that's read in from the config
    auto loadAnimationTrigger = [&config, &id]( const char* key, AnimationTrigger& trigger )
    {
      ASSERT_NAMED_EVENT( config.isMember( key ), "MicStreamingComponent.LoadRecognizerBehaviorResponse",
                          "Response %s missing required field %s", id.c_str(), key );

      // attempt to grab the AnimationTrigger directly from the string
      const std::string animString = JsonTools::ParseString( config, key, "MicStreamingComponent.LoadRecognizerBehaviorResponse" );
      if ( !AnimationTriggerFromString( animString, trigger ) )
      {
        trigger = AnimationTrigger::InvalidAnimTrigger;
        LOG_ERROR( "MicStreamingComponent.LoadRecognizerBehaviorResponse",
                   "Value supplied in field %s for response id %s, is not a valid AnimationTrigger",
                   key, id.c_str() );
      }
    };

    loadAnimationTrigger( kKeyResponseAnimLoop, streamingResponse.animLooping );
    loadAnimationTrigger( kKeyResponseAnimGetOut, streamingResponse.animGetOut );
  }

  // load in our earcon responses
  {
    using namespace AudioMetaData::GameEvent;

    // load the GenericEvent that's read in from the config
    auto loadEarcon = [&config, &id]( const char* key, GenericEvent& audioEvent )
    {
      ASSERT_NAMED_EVENT( config.isMember( key ), "MicStreamingComponent.LoadRecognizerBehaviorResponse",
                          "Response %s missing required field %s", id.c_str(), key );

      // attempt to grab the GenericEvent directly from the string
      const std::string earconString = JsonTools::ParseString( config, key, "MicStreamingComponent.LoadRecognizerBehaviorResponse" );
      if ( !GenericEventFromString( earconString, audioEvent ) )
      {
        audioEvent = GenericEvent::Invalid;
        LOG_ERROR( "MicStreamingComponent.LoadRecognizerBehaviorResponse",
                   "Value supplied in field %s for response id %s, is not a valid audio event",
                   key, id.c_str() );
      }
    };

    loadEarcon( kKeyResponseEarconSuccess, streamingResponse.earConSuccess );
    loadEarcon( kKeyResponseEarconFail, streamingResponse.earConFail );
  }

  LOG_DEBUG( "MicStreamingComponent.LoadRecognizerBehaviorResponse",
             "Loaded RecognizerBehaviorResponse %s, anims [%s | %s] earcons [%s | %s]",
             id.c_str(),
             AnimationTriggerToString( streamingResponse.animLooping ),
             AnimationTriggerToString( streamingResponse.animGetOut ),
             GenericEventToString( streamingResponse.earConSuccess ),
             GenericEventToString( streamingResponse.earConFail ) );

  return streamingResponse;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::Update()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::HandleTriggerWordDetectedEvent( const AnkiEvent<RobotInterface::RobotToEngine>& event )
{
  const RobotInterface::TriggerWordDetected triggerEvent = event.GetData().Get_triggerWordDetected();

  // we only need a subset of the trigger word data for our purposes
  StreamingEventData data =
  {
    .triggerWordData =
    {
      .isStreamSimulated  = !triggerEvent.willOpenStream,
      .direction          = triggerEvent.direction
    }
  };

  LOG_INFO( "MicStreamingComponent.RobotToEngine", "Received Trigger Word Detected Message: %s",
            !data.triggerWordData.isStreamSimulated ? "Will open stream" : "Will NOT open stream" );

  // Let's make sure we always have SOMEBODY listening for the trigger word detected message
  // this is a bug if we don't respond to an upcoming stream
  ANKI_VERIFY( NotifyStreamingEventCallbacks( StreamingEvent::TriggerWordDetected, data ),
               "MicStreamingComponent.HandleTriggerWordDetectedEvent",
               "Trigger Word Detected but not handled by any callback listeners (behaviors)" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::HandleStreamingStateSyncEvent( const AnkiEvent<RobotInterface::RobotToEngine>& event )
{
  const RobotInterface::MicStreamingStateSync& streamingStateSync = event.GetData().Get_micStateSync();
  LOG_INFO( "MicStreamingComponent.RobotToEngine", "Received Streaming State Sync Message: New State (%s)",
            MicStreamStateToString( streamingStateSync.state ) );

  // let our listeners know about our state change ...
  const MicStreamState previousState = _streamingState;
  if ( previousState != streamingStateSync.state )
  {
    // all of the state events share the same event data
    StreamingEventData data =
    {
      .streamStateData =
      {
        .isSimulated = streamingStateSync.isSimulated
      }
    };

    switch ( streamingStateSync.state )
    {
      case MicStreamState::TransitionIn:
      {
        NotifyStreamingEventCallbacks( StreamingEvent::StreamProcessBegin, data );
        break;
      }

      case MicStreamState::Streaming:
      {
        // make sure we always post our state progression sequentially, so in the case where we go straight into
        // streaming without transition, let's still let the listeners know that the process has begun
        if ( MicStreamState::Listening == previousState )
        {
          NotifyStreamingEventCallbacks( StreamingEvent::StreamProcessBegin, data );
        }

        NotifyStreamingEventCallbacks( StreamingEvent::StreamOpen, data );
        break;
      }

      case MicStreamState::TransitionOut:
      {
        NotifyStreamingEventCallbacks( StreamingEvent::StreamClosed, data );
        break;
      }

      case MicStreamState::Listening:
      {
        if ( MicStreamState::Streaming == previousState )
        {
          NotifyStreamingEventCallbacks( StreamingEvent::StreamClosed, data );
        }

        NotifyStreamingEventCallbacks( StreamingEvent::StreamProcessEnd, data );
        break;
      }
    }

    // updating this after the callbacks so that listeners can reference the current state
    _streamingState = streamingStateSync.state;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingComponent::NotifyStreamingEventCallbacks( StreamingEvent event, StreamingEventData data ) const
{
  bool wasEventHandled = false;

  for ( auto func : _streamingEventCallbacks )
  {
    if ( func != nullptr )
    {
      func( event, data );
    }
  }

  return wasEventHandled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::StopCurrentStream( MicStreamResult reason ) const
{
  // send the stop request to the anim process and it will let us know when the stream has stopped
  RobotInterface::StopCurrentMicStream stopStreamingMessage( reason );
  _robot->SendMessage( RobotInterface::EngineToRobot( std::move(stopStreamingMessage) ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::PushRecognizerBehaviorResponse( Util::StringID sourceId, std::string responseId )
{
  LOG_INFO( "MicStreamingComponent.PushRecognizerBehaviorResponse", "Behavior ID %s is pushing recognizer response (%s)",
             sourceId.ToString().c_str(), responseId.c_str() );

  // ensure the response actually exists before we actually update our active list
  if ( IsValidRecognizerResponse( responseId ) )
  {
    RecognizerResponsePair newResponse( sourceId, std::move( responseId ) );

    // remove any older version of our response and move it to the front of the list
    _activeRecognizerResponses.remove( newResponse );
    _activeRecognizerResponses.push_front( std::move( newResponse ) );

    OnActiveRecognizerResponseChanged();
  }
  else
  {
    LOG_ERROR( "MicStreamingComponent.PushRecognizerBehaviorResponse",
               "Behavior ID %s is attempted to push an invalid recognizer response (%s)",
               sourceId.ToString().c_str(), responseId.c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::PopRecognizerBehaviorResponse( Util::StringID sourceId )
{
  LOG_INFO( "MicStreamingComponent.PopRecognizerBehaviorResponse", "Behavior ID %s is popping it's recognizer response",
             sourceId.ToString().c_str() );

  _activeRecognizerResponses.remove_if( [sourceId]( const RecognizerResponsePair& response ) { return response.first == sourceId; } );
  OnActiveRecognizerResponseChanged();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::OnActiveRecognizerResponseChanged()
{
  auto sendRecognizerResponseMessage = [this]( const std::string& responseId )
  {
    LOG_INFO( "MicStreamingComponent.OnActiveRecognizerResponseChanged",
               "Setting recognizer response to (%s)", responseId.c_str() );

    // send the stop request to the anim process and it will let us know when the stream has stopped
    RobotInterface::SetRecognizerResponse responseMessage( responseId );
    _robot->SendMessage( RobotInterface::EngineToRobot( std::move( responseMessage ) ) );
  };

  // the front of our list is considered the active response (it's the most recently pushed response)
  // make sure we have a valid response at the front of the list
  if ( HasRecognizerResponse() )
  {
    const std::string& responseId = _activeRecognizerResponses.front().second;

    // this should never happen since we screen for a valid response in PushRecognizerBehaviorResponse(...)
    ASSERT_NAMED_EVENT( IsValidRecognizerResponse( responseId ), "MicStreamingComponent.OnActiveRecognizerResponseChanged",
                        "Invalid recognizer response [%s|%s] found in the active response list!",
                        _activeRecognizerResponses.front().first.ToString().c_str(),
                        responseId.c_str() );

    sendRecognizerResponseMessage( responseId );
  }
  else
  {
    // we don't have an active response, which means we want to disable it on the anim as well
    // empty string will disable
    sendRecognizerResponseMessage( "" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RecognizerBehaviorResponse MicStreamingComponent::GetRecognizerBehaviorResponse() const
{
  using namespace AudioMetaData::GameEvent;

  RecognizerBehaviorResponse response =
  {
    .animLooping    = AnimationTrigger::InvalidAnimTrigger,
    .animGetOut     = AnimationTrigger::InvalidAnimTrigger,
    .earConSuccess  = GenericEvent::Invalid,
    .earConFail     = GenericEvent::Invalid,
  };

  // the response at the front of the list is the one that is currently active
  // use the front of the list if it's not empty, else return the "disabled" response we created above
  if ( HasRecognizerResponse() )
  {
    const std::string& responseId = _activeRecognizerResponses.front().second;

    // this should never happen since we screen for a valid response in PushRecognizerBehaviorResponse(...)
    ASSERT_NAMED_EVENT( IsValidRecognizerResponse( responseId ), "MicStreamingComponent.GetRecognizerResponse",
                        "Invalid recognizer response [%s|%s] found in the active response list!",
                        _activeRecognizerResponses.front().first.ToString().c_str(),
                        responseId.c_str() );

    response = _streamingResponseMap.find( responseId )->second;
  }
  else
  {
    // we're going to error here because we want users to check HasRecognizerResponse() prior to calling this
    LOG_ERROR( "MicStreamingComponent.GetRecognizerResponse",
               "Requesting the active recognizer response, but list is empty, call HasRecognizerResponse() before requesting" );
  }

  return response;
}

} // namespace Vector
} // namespace Anki
