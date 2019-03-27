/***********************************************************************************************************************
 *
 *  MicRecordingStateController .cpp
 *  Victor / Anim
 *
 *  Created by Jarrod Hatfield on 3/22/2019
 *  Copyright Anki, Inc. 2019
 *
 **********************************************************************************************************************/

// #include "cozmoAnim/micData/micRecordingStateController.h"
#include "micRecordingStateController.h"
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
MicRecordingStateController::MicRecordingStateController( MicDataSystem* micSystem ) :
  _micDataSystem( micSystem )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicRecordingStateController::~MicRecordingStateController()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicRecordingStateController::Initialize( const Anim::AnimContext* animContext )
{
  _animContext = animContext;

  // we need this to do our stuff ...
  DEV_ASSERT( nullptr != _animContext, "MicRecordingStateController.Initialize" );
  DEV_ASSERT( nullptr != _animContext->GetBackpackLightComponent(), "MicRecordingStateController.Initialize" );
  DEV_ASSERT( nullptr != _animContext->GetShowAudioStreamStateManager(), "MicRecordingStateController.Initialize" );

  // these should all exist by now ...
  DEV_ASSERT( nullptr != _micDataSystem, "MicRecordingStateController.Initialize" );
  DEV_ASSERT( nullptr != _micDataSystem->GetMicDataProcessor(), "MicRecordingStateController.Initialize" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicRecordingStateController::CanBeginStreamingJob() const
{
  const ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();
  DEV_ASSERT( nullptr != streamAnimManager, "MicRecordingStateController.BeginStreamingJob" );

  return ( HasStreamingBegun() || streamAnimManager->HasValidTriggerResponse() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicRecordingStateController::BeginStreamingJob( CloudMic::StreamType streamType, bool shouldPlayTransitionAnim, OnTransitionComplete callback )
{
  bool streamingBegun = false;

  // don't allow a new stream if we're already streaming
  if ( CanBeginStreamingJob() )
  {
    // we're going to attempt the stream (doesn't mean it will succeed)
    // something can go wrong in ShowAudioStreamStateManager and we can still fail (we'll get a callback with success/fail)
    streamingBegun = true;

    ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();
    DEV_ASSERT( nullptr != streamAnimManager, "MicRecordingStateController.BeginStreamingJob" );

    // we consider ourselves transitioning regardless of if the anim has played or not
    // this is because we want to wait until we actually get the callback to stream before setting our stream state
    // in case something goes wrong (callback returns sucess == false)
    OnTransitionBegin();

    // wrap the streaming callback in our own so we know when streaming is about to begin
    const bool shouldStream = streamAnimManager->ShouldStreamAfterTriggerWordResponse();
    auto streamingCallback = [=]( bool success )
    {
      if ( shouldStream )
      {
        if ( success )
        {
          // the transition was successful and now we should officially start the stream.
          MicDataProcessor* micProcessor = _micDataSystem->GetMicDataProcessor();
          DEV_ASSERT( nullptr != micProcessor, "MicRecordingStateController.BeginStreamingJob" );

          const RobotTimeStamp_t timestamp = micProcessor->CreateStreamJob( streamType, kTriggerLessOverlapSize_ms );
          LOG_INFO( "MicRecordingStateController.CreateStreamJob", "Timestamp %d", (TimeStamp_t)timestamp );

          OnStreamingBegin();
        }
        else
        {
          // something went wrong ...
          SetWillStream( false );
        }
      }

      // if we're not going to progress into streaming, then we're done with this stream job
      if ( !success || !shouldStream )
      {
        OnStreamingEnd();
      }

      callback( success );
    };

    if ( shouldPlayTransitionAnim )
    {
      streamAnimManager->SetPendingTriggerResponseWithGetIn( streamingCallback );
    }
    else
    {
      streamAnimManager->SetPendingTriggerResponseWithoutGetIn( streamingCallback );
    }
  }
  else
  {
    callback( false );
    LOG_WARNING( "MicRecordingStateController.BeginStreamingJob",
                 "Trying to start a new streaming job while one is already active ... ignoring this request" );
  }

  return streamingBegun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicRecordingStateController::EndStreamingJob()
{
  OnStreamingEnd();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicRecordingStateController::OnTransitionBegin()
{
  _state = MicState::TransitionToStreaming;

  const ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();
  DEV_ASSERT( nullptr != streamAnimManager, "MicRecordingStateController.OnTransitionBegin" );

  // todo: jmeh
  // why do we even play the transition if we're not going to stream afterwards?
  const bool shouldStream = streamAnimManager->ShouldStreamAfterTriggerWordResponse();
  SetWillStream( shouldStream );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicRecordingStateController::OnStreamingBegin()
{
  _state = MicState::Streaming;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicRecordingStateController::OnStreamingEnd()
{
  _state = MicState::Listening;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicRecordingStateController::SetWillStream( bool willStream ) const
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
void MicRecordingStateController::SetMicsMuted( bool isMuted )
{
  Anim::BackpackLightComponent* bpLights = _animContext->GetBackpackLightComponent();
  DEV_ASSERT( nullptr != bpLights, "MicRecordingStateController.SetMicsMuted" );

  if ( _isMuted != isMuted )
  {
    _isMuted = isMuted;

    // toggle backpack lights
    bpLights->SetMicMute( _isMuted );
  }
}

} // namespace MicData
} // namespace Vector
} // namespace Anki
