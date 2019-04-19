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
#include "coretech/common/engine/utils/timer.h"
#include "util/logging/logging.h"
#include "util/global/globalDefinitions.h"

// todo: jmeh
// + how to handle multiple behaviors wanting to respond to trigger word ... this is going to depend on how
//   we end up using the system; if there's only a single behavior then we don't care
//   - keep old "pending trigger word" model
//   - have a "TriggerWordHandled" event
//   - stack (earliest wins)
//   - priorities
// + other non-wakeword streaming needs to use this as well
//   - BehaviorPromptUserForVoiceCommand
//   - BehaviorKnowledgeGraphQuestion

#define LOG_CHANNEL "Microphones"

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

} // namespace Vector
} // namespace Anki
