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

#include "util/logging/logging.h"


// todo: jmeh
// + handle the case when ShowAudioStreamStateManager::ShouldStreamAfterTriggerWordResponse() is false
// + what about ShouldSimulateStreaming()?
// + migrate _streamUpdatedCallbacks here as well

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
bool MicStreamingController::CanBeginStreamingJob() const
{
  const ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();
  DEV_ASSERT( nullptr != streamAnimManager, "MicStreamingController.BeginStreamingJob" );

  return ( !HasStreamingBegun() && streamAnimManager->HasValidTriggerResponse() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::BeginStreamingJob( StreamingArguments args )
{
  bool streamingBegun = false;

  // don't allow a new stream if we're already streaming
  if ( CanBeginStreamingJob() )
  {
    _streamingArgs = args;

    // we consider ourselves transitioning regardless of if there is a get-in animation or not
    // this is because we want to wait until we actually get the callback to stream before setting our stream state
    // in case something goes wrong (callback returns sucess == false)
    TransitionToState( MicState::TransitionToStreaming );

    streamingBegun = true;
  }

  return streamingBegun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::EndStreamingJob()
{
  TransitionToState( MicState::Listening );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::TransitionToState( MicState nextState )
{
  if ( nextState != _state )
  {
    LOG_INFO( "MicStreamingController.TransitionToState",
              "Transition from MicState::%s to MicState::%s",
              GetStateString( _state ), GetStateString( nextState ) );

    _state = nextState;
    switch ( _state )
    {
      case MicState::TransitionToStreaming:
      {
        OnStreamingTransitionBegin();
        break;
      }

      case MicState::Streaming:
      {
        OnStreamingBegin();
        break;
      }

      case MicState::Listening:
      {
        OnStreamingEnd();
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
        TransitionToState( MicState::Streaming );
      }
      else
      {
        // something went wrong ...
        NotifyTriggerWordDetectedCallbacks( false );
      }
    }

    // if we're not going to progress into streaming, then we're done with this stream job
    if ( !success || !shouldStream )
    {
      TransitionToState( MicState::Listening );
    }
  };

  LOG_INFO( "MicStreamingController.OnStreamingTransitionBegin", "Starting ww transition to streaming (%s stream)",
            shouldStream ? "will" : "will not" );

  // start our transition, if we have one, else we jump right into streaming
  // note: streaming is kicked off from the callback
  if ( _streamingArgs.shouldPlayTransitionAnim )
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
  MicDataProcessor* micProcessor = _micDataSystem->GetMicDataProcessor();
  DEV_ASSERT( nullptr != micProcessor, "MicStreamingController.OnStreamingBegin" );

  // if we were told to record the trigger word, do so now
  // note: it can reach "back in time" to save out the trigger word recording
  if ( _streamingArgs.shouldRecordTriggerWord )
  {
    micProcessor->CreateTriggerWordDetectedJob();
  }

  // Now let's kick off our streaming job
  const RobotTimeStamp_t timestamp = micProcessor->CreateStreamingJob( _streamingArgs.streamType, kTriggerLessOverlapSize_ms );
  LOG_INFO( "MicStreamingController.OnStreamingBegin", "Streaming job created at time %d", (TimeStamp_t)timestamp );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::OnStreamingEnd()
{

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
void MicStreamingController::SetMicsMuted( bool isMuted )
{
  Anim::BackpackLightComponent* bpLights = _animContext->GetBackpackLightComponent();
  DEV_ASSERT( nullptr != bpLights, "MicStreamingController.SetMicsMuted" );

  if ( _isMuted != isMuted )
  {
    _isMuted = isMuted;

    // toggle backpack lights
    bpLights->SetMicMute( _isMuted );
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
