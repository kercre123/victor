/***********************************************************************************************************************
 *
 *  MicStreamingController .cpp
 *  Victor / Anim
 *
 *  Created by Jarrod Hatfield on 3/22/2019
 *  Copyright Anki, Inc. 2019
 *
 **********************************************************************************************************************/

// #include "cozmoAnim/micData/micStreamingController.h"
#include "micStreamingController.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/backpackLights/animBackpackLightComponent.h"
#include "cozmoAnim/micData/micDataProcessor.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/showAudioStreamStateManager.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/logging/logging.h"
#include "util/global/globalDefinitions.h"


// todo: jmeh
// + move ShowAudioStreamStateManager logic into here?


#define LOG_CHANNEL "Microphones"

namespace Anki {
namespace Vector {
namespace MicData {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicStreamingController::MicStreamingController( MicDataSystem* micSystem ) :
  _micDataSystem( micSystem )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicStreamingController::~MicStreamingController()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::Initialize( const Anim::AnimContext* animContext )
{
  _animContext = animContext;

  // we need this to do our stuff ...
  DEV_ASSERT( nullptr != _animContext, "MicStreamingController.Initialize" );
  DEV_ASSERT( nullptr != _animContext->GetBackpackLightComponent(), "MicStreamingController.Initialize" );
  DEV_ASSERT( nullptr != _animContext->GetShowAudioStreamStateManager(), "MicStreamingController.Initialize" );

  // these should all exist by now ...
  DEV_ASSERT( nullptr != _micDataSystem, "MicStreamingController.Initialize" );
  DEV_ASSERT( nullptr != _micDataSystem->GetMicDataProcessor(), "MicStreamingController.Initialize" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::Update()
{
  if ( IsActivelyStreaming() )
  {
    if ( CanStreamToCloud() )
    {
      static constexpr size_t kMaxRecordNumChunks = ( kStreamingTimeout_ms / kTimePerChunk_ms ) + 1;
      const size_t chunksStreamed = _micDataSystem->SendLatestMicStreamingJobDataToCloud();

      // check to see if we've reached the max duration of recording
      if ( chunksStreamed >= kMaxRecordNumChunks )
      {
        LOG_INFO( "MicStreamingController.Update", "Mic streaming job timed out after %zu ms",
                  ( chunksStreamed * kTimePerChunk_ms ) );
        StopCurrentMicStreaming( StreamingResult::Timeout );
      }
    }
    else
    {
      const ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();
      DEV_ASSERT( nullptr != streamAnimManager, "MicStreamingController.CanStartNewMicStream" );

      // if we're actively streaming, but can't stream to the cloud, it means we're faking a stream for whatever reason
      // let's wait for our min time before closing our fake stream
      const BaseStationTime_t currentTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

      const uint32_t minStreamingDuration_ms = streamAnimManager->GetMinStreamingDuration();
      const BaseStationTime_t minStreamDuration_ns = ( minStreamingDuration_ms * 1000 * 1000 );
      const BaseStationTime_t minStreamEnd_ns = ( _streamingData.streamBeginTime + minStreamDuration_ns );
      if ( currentTime_ns >= minStreamEnd_ns )
      {
        LOG_INFO( "MicStreamingController.Update", "Simulated streaming job timed out after %zu ms",
                  minStreamingDuration_ms );
        StopCurrentMicStreaming( StreamingResult::Timeout );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::CanStartNewMicStream() const
{
  const ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();
  DEV_ASSERT( nullptr != streamAnimManager, "MicStreamingController.CanStartNewMicStream" );

  return ( !HasStreamingBegun() && streamAnimManager->HasValidTriggerResponse() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::StartNewMicStream( StreamingArguments args )
{
  LOG_DEBUG( "MicStreamingController.StartNewMicStream", "Received request to start a new mic stream" );

  StreamData streamData =
  {
    .args = args
  };

  // don't start a new stream if we're not supposed to
  if ( CanStartNewMicStream() )
  {
    StartStreamInternal( streamData );
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::StartCloudTestStream()
{
  StreamingArguments args =
  {
    .streamType                 = CloudMic::StreamType::Normal,
    .shouldPlayTransitionAnim   = false,
    .shouldRecordTriggerWord    = false
  };

  StreamData streamData =
  {
    .args = args
  };

  StartStreamInternal( streamData );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::StartStreamInternal( StreamData streamData )
{
  _streamingData = streamData;

  // we consider ourselves transitioning regardless of if there is a get-in animation or not
  // this is because we want to wait until we actually get the callback to stream before setting our stream state
  // in case something goes wrong (callback returns sucess == false)
  TransitionToState( MicState::TransitionToStreaming );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::StopCurrentMicStreaming( StreamingResult reason )
{
  if ( HasStreamingBegun() )
  {
    _streamingData.result = reason;
    TransitionToState( MicState::Listening );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::TransitionToState( MicState nextState )
{
  if ( nextState != _state )
  {
    LOG_INFO( "MicStreamingController.TransitionToState",
              "Transition from MicState::%s to MicState::%s",
              GetStateString( _state ), GetStateString( nextState ) );

    const MicState previousState = _state;
    _state = nextState;

    switch ( _state )
    {
      case MicState::TransitionToStreaming:
      {
        DEV_ASSERT_MSG( MicState::Listening == previousState, "MicStreamingController.TransitionToState",
                        "Can only enter Transition state from Listening state (currently in %s state)",
                        GetStateString( previousState ) );

        OnStreamingTransitionBegin();
        break;
      }

      case MicState::Streaming:
      {
        DEV_ASSERT_MSG( MicState::TransitionToStreaming == previousState, "MicStreamingController.TransitionToState",
                       "Can only enter Streaming state from Transition state (currently in %s state)",
                       GetStateString( previousState ) );

        OnStreamingBegin();
        break;
      }

      case MicState::Listening:
      {
        if ( MicState::Streaming == previousState )
        {
          OnStreamingEnd();
        }
      }
    }
  }
  else
  {
    LOG_ERROR( "MicStreamingController.TransitionToState",
               "Attempting to transition into a state we are already in (%s)",
               GetStateString( _state ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::OnStreamingTransitionBegin()
{
  // todo: jmeh
  // + why do we even play the transition if we're not going to stream afterwards?

  // we're going to attempt the stream (doesn't mean it will succeed)
  // something can go wrong in ShowAudioStreamStateManager and we can still fail (we'll get a callback with success/fail)

  ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();
  DEV_ASSERT( nullptr != streamAnimManager, "MicStreamingController.OnStreamingTransitionBegin" );

  // wrap the streaming callback in our own so we know when streaming is about to begin
  const bool shouldStream = streamAnimManager->ShouldStreamAfterTriggerWordResponse();
  NotifyTriggerWordDetectedCallbacks( shouldStream );

  // this callback is fired as soon as the "earcon" is finished
  // this is so that we do not include the earcon audio in the stream
  auto streamingCallback = [=]( bool success )
  {
    if ( shouldStream )
    {
      if ( success )
      {
        // all is good, let's kick off the streaming job
        TransitionToState( MicState::Streaming );
      }
      else
      {
        // something went wrong, notify our callbacks that we're bailing
        // note: this is legacy and I don't like it
        NotifyTriggerWordDetectedCallbacks( false );
      }
    }

    // if we're not going to progress into streaming, then we're done with this stream job
    // no need to stop any streaming at this point since it hasn't begun
    if ( !success || !shouldStream )
    {
      // note: again, why the hell are we starting a stream when shouldStream is false?!?
      const StreamingResult result = ( success ? StreamingResult::Success : StreamingResult::Error );
      StopCurrentMicStreaming( result );
    }
  };

  LOG_INFO( "MicStreamingController.OnStreamingTransitionBegin", "Starting ww transition to streaming (%s stream)",
            shouldStream ? "will" : "will not" );

  // start our transition, if we have one, else we jump right into streaming
  // note: streaming is kicked off from the callback
  if ( _streamingData.args.shouldPlayTransitionAnim )
  {
    streamAnimManager->SetPendingTriggerResponseWithGetIn( streamingCallback );
  }
  else
  {
    streamAnimManager->SetPendingTriggerResponseWithoutGetIn( streamingCallback );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::OnStreamingBegin()
{
  // track the begin time for both regular and simluated stream
  _streamingData.streamBeginTime = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

  if ( !IsStreamSimulated() )
  {
    // if we were told to record the trigger word, do so now
    // note: it can reach "back in time" to save out the trigger word recording
    if ( _streamingData.args.shouldRecordTriggerWord )
    {
      _micDataSystem->StartTriggerWordDetectedJob();
    }

    // Now let's kick off our streaming job
    _streamingData.streamJobCreated = _micDataSystem->StartMicStreamingJob( _streamingData.args.streamType );
  }
  else
  {
    LOG_DEBUG( "MicStreamingController.OnStreamingBegin", "Kicking off new simulated stream" );
  }

  NotifyStreamingStateChangedCallbacks( true );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::OnStreamingEnd()
{
  // only need to kill the streaming job if we've created one
  if ( _streamingData.streamJobCreated )
  {
    // if we timed out, we need to notify the cloud that we're no longer streaming
    const bool shouldNotifyCloud = ( StreamingResult::Success != _streamingData.result );

    _micDataSystem->StopMicStreamingJob( shouldNotifyCloud );
    _streamingData.streamJobCreated = false;
  }
  else
  {
    LOG_DEBUG( "MicStreamingController.OnStreamingEnd", "Stopping a simulated stream" );
  }

  NotifyStreamingStateChangedCallbacks( false );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::CanStreamToCloud() const
{
  return ( !IsStreamSimulated() && _streamingData.streamJobCreated );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::NotifyTriggerWordDetectedCallbacks( bool willStream ) const
{
  for ( auto func : _triggerWordDetectedCallbacks )
  {
    if ( func != nullptr )
    {
      func( willStream );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::NotifyStreamingStateChangedCallbacks( bool streamStarted ) const
{
  for ( auto func : _streamingStateChangedCallbacks )
  {
    if ( func != nullptr )
    {
      func( streamStarted );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* MicStreamingController::GetStateString( MicState state ) const
{
  switch ( state )
  {
    case MicState::Listening:
      return "Listening";
    case MicState::TransitionToStreaming:
      return "TransitionToStreaming";
    case MicState::Streaming:
      return "Streaming";
  }
}

} // namespace MicData
} // namespace Vector
} // namespace Anki
