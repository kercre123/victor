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
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/showAudioStreamStateManager.h"

#include "audioUtil/speechRecognizer.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"
#include "util/global/globalDefinitions.h"


// todo: jmeh
// + DESTROY ShowAudioStreamStateManager and implement it here (but better)
// + make trigger word detection API private and set a callback
// + change all required INFO's back to DEBUG's


#define LOG_CHANNEL "Microphones"

namespace {
  const std::string   kConfigFilename                   = "config/micData/micStreamingConfig.json";
  const char*         kKeyResponsesField                = "recognizer_responses";
  const char*         kKeyResponseType                  = "response_type";
  const char*         kKeyResponsesAnimation            = "response_animation";
}

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

  {
    using namespace Util;

    Json::Value streamingConfig;

    const Data::DataPlatform* platform = animContext->GetDataPlatform();
    if ( platform->readAsJson( Data::Scope::Resources, kConfigFilename, streamingConfig ) )
    {
      LoadStreamingReactions( streamingConfig );
    }
    else
    {
      LOG_ERROR( "MicStreamingController.Initialize",
                 "Error loading mic streaming response config file (%s), check the formatting (extra comma, etc)",
                 kConfigFilename.c_str() );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::LoadStreamingReactions( const Json::Value& config )
{
  const Json::Value& allResponsesConfig = config[kKeyResponsesField];
  for( Json::Value::const_iterator it = allResponsesConfig.begin() ; it != allResponsesConfig.end() ; it++ )
  {
    const std::string id = it.key().asString();
    RecognizerResponse recognizerResponse = LoadRecognizerResponse( *it, id );

    _recognizerResponseMap.emplace( id, recognizerResponse );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicStreamingController::RecognizerResponse MicStreamingController::LoadRecognizerResponse( const Json::Value& config, const std::string& id )
{
  RecognizerResponse response;

  ASSERT_NAMED_EVENT( config.isMember( kKeyResponseType ), "MicStreamingController.LoadRecognizerResponse",
                      "Response %s missing required field %s", id.c_str(), kKeyResponseType );
  const std::string typeString = JsonTools::ParseString( config, kKeyResponseType, "MicStreamingController.LoadRecognizerResponse" );
  ANKI_VERIFY( RecognizerResponseTypeFromString( typeString, response.type ),
               "MicStreamingController.LoadRecognizerResponse",
               "Response %s supplied an invalid value for field %s",
               id.c_str(), kKeyResponseType );

  // don't need a response animation if we're disabled
  if ( RecognizerResponseType::ResponseDisabled != response.type )
  {
    JsonTools::GetValueOptional( config, kKeyResponsesAnimation, response.animation );
  }

  LOG_DEBUG( "MicStreamingController.LoadRecognizerResponse",
             "Loaded RecognizerResponse %s, with type [%s] and animation [%s]",
             id.c_str(),
             RecognizerResponseTypeToString( response.type ),
             response.animation.c_str() );

  return response;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::Update()
{
  // we're either actively streaming or simulating a stream
  if ( IsInStreamingState() )
  {
    // keep streaming to the cloud as long as we can
    if ( CanStreamToCloud() )
    {
      static constexpr size_t kMaxRecordNumChunks = ( kStreamingTimeout_ms / kTimePerChunk_ms ) + 1;
      const size_t chunksStreamed = _micDataSystem->SendLatestMicStreamingJobDataToCloud();

      // check to see if we've reached the max duration of recording
      if ( chunksStreamed >= kMaxRecordNumChunks )
      {
        LOG_INFO( "MicStreamingController.Update", "Mic streaming job finished streaming after %fs",
                  ( ( chunksStreamed * kTimePerChunk_ms ) / 1000.0 ) );
        TransitionToState( MicStreamState::TransitionOut );
      }
    }
    else
    {
      const ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();

      // if we're actively streaming, but can't stream to the cloud, it means we're faking a stream for whatever reason
      // let's wait for our min time before closing our fake stream
      const BaseStationTime_t currentTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

      const uint32_t minStreamingDuration_ms = streamAnimManager->GetMinStreamingDuration();
      const BaseStationTime_t minStreamDuration_ns = ( minStreamingDuration_ms * 1000 * 1000 );
      const BaseStationTime_t minStreamEnd_ns = ( _streamingData.streamBeginTime + minStreamDuration_ns );
      if ( currentTime_ns >= minStreamEnd_ns )
      {
        LOG_INFO( "MicStreamingController.Update", "Simulated streaming job timed out after %fs",
                  ( minStreamingDuration_ms / 1000.0 ) );

        // note: treating this as a success since we're simluating a stream and can only "time out" in this case
        //       so therefore, a time out is our success
        StopCurrentMicStreaming( MicStreamResult::Success );
      }
    }
  }
  else if ( IsWaitingForCloudResponse() )
  {
    // we're not streaming, so we're in the TransitionOut state which means we're giving the cloud some extra
    // time to see if it can figure out what was streamed
    const BaseStationTime_t currentTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

    const BaseStationTime_t extendedThinkTime_ns = ( kStreamingTimeoutCloudExt_ms * 1000 * 1000 );
    const BaseStationTime_t endTime_ns = ( _streamingData.streamEndTime + extendedThinkTime_ns );
    if ( currentTime_ns >= endTime_ns )
    {
      LOG_INFO( "MicStreamingController.Update", "Mic stream timed out with no response from cloud after %fs",
                ( kStreamingTimeout_ms / 1000.0 ) + ( kStreamingTimeoutCloudExt_ms / 1000.0 ) );
      StopCurrentMicStreaming( MicStreamResult::Timeout );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::CanStartOpenMicStream() const
{
  const ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();

  // DO NOT STREAM IF ...
  // + we're already streaming
  // + we don't have a "stream open" response (this is because everything is done from the earcon callback)
  return ( !HasStreamingBegun() && streamAnimManager->HasValidTriggerResponse() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::CanStartWakeWordStream() const
{
  const ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();

  // todo: jmeh - merge the concept of HasValidTriggerResponse() && ShouldStreamAfterTriggerWordResponse()
  //              within ShowAudioStreamStateManager

  // DO NOT STREAM IF ...
  // + we're already streaming
  // + we've been explicitely told not to stream after hearing the trigger word
  return ( !HasStreamingBegun() && streamAnimManager->ShouldStreamAfterTriggerWordResponse() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::StartWakeWordStream( WakeWordSource source, const AudioUtil::SpeechRecognizerCallbackInfo* info )
{
  LOG_DEBUG( "MicStreamingController.StartWakeWordStream",
             "Received request to start a wakeword stream from source %s",
             WakeWordSourceToString( source ) );

  if ( CanStartWakeWordStream() )
  {
    const bool muteButton = ( source == WakeWordSource::ButtonFromMute );
    const bool buttonPress = ( source == WakeWordSource::Button ) || muteButton;

    // get our direction information
    DirectionIndex wakeWordDirection = kDirectionUnknown;
    if ( WakeWordSource::Voice == source )
    {
      MicDirectionData wakeWordData;
      _micDataSystem->GetMicDataProcessor()->GetLatestMicDirectionData( wakeWordData, wakeWordDirection );
    }

    const uint32_t triggerScore = ( ( nullptr != info ) ? static_cast<uint32_t>(info->score) : 0 );

    // Set up a message to send out about the triggerword
    // Fire off the trigger word detected message BEFORE we open the stream
    RobotInterface::TriggerWordDetected twDetectedMessage;
    {
      twDetectedMessage.direction       = wakeWordDirection;
      twDetectedMessage.isButtonPress   = buttonPress;
      twDetectedMessage.fromMute        = muteButton;
      twDetectedMessage.triggerScore    = triggerScore;
      twDetectedMessage.willOpenStream  = true;
    }
    AnimProcessMessages::SendAnimToEngine( twDetectedMessage );

    LOG_INFO( "MicDataProcessor.TriggerWordDetectCallback", "Direction index %d", wakeWordDirection );


    // don't play the get-in if this trigger word started from mute, because the mute animation should be playing
    StreamingArguments args =
    {
      .streamType                 = CloudMic::StreamType::Normal,
      .shouldPlayTransitionAnim   = !muteButton,
      .shouldRecordTriggerWord    = true,
      .isSimulated                = ShouldSimulateWakeWordStreaming()
    };
    StartStreamInternal( args );

    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::StartOpenMicStream( CloudMic::StreamType streamType, bool shouldPlayTransitionAnim )
{
  LOG_DEBUG( "MicStreamingController.StartOpenMicStream", "Received request to start a new mic stream" );

  // don't start a new stream if we're not supposed to
  if ( CanStartOpenMicStream() )
  {
    StreamingArguments args =
    {
      .streamType                 = streamType,
      .shouldPlayTransitionAnim   = shouldPlayTransitionAnim,
      .shouldRecordTriggerWord    = false,
      .isSimulated                = false
    };

    StartStreamInternal( args );

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
    .shouldRecordTriggerWord    = false,
    .isSimulated                = false
  };

  StartStreamInternal( args );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::StartStreamInternal( StreamingArguments args )
{
  _streamingData.args = args;

  // we consider ourselves transitioning regardless of if there is a get-in animation or not
  // this is because we want to wait until we actually get the callback to stream before setting our stream state
  // in case something goes wrong (callback returns sucess == false)
  TransitionToState( MicStreamState::TransitionIn );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::StopCurrentMicStreaming( MicStreamResult reason )
{
  if ( HasStreamingBegun() )
  {
    _streamingData.result = reason;
    TransitionToState( MicStreamState::Listening );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::TransitionToState( MicStreamState nextState )
{
  if ( nextState != _state )
  {
    LOG_INFO( "MicStreamingController.TransitionToState",
              "Transition from MicStreamState::%s to MicStreamState::%s",
              EnumToString( _state ), EnumToString( nextState ) );

    const MicStreamState previousState = _state;
    _state = nextState;

    // any time we leave the Streaming state, we need to close down the stream
    if ( MicStreamState::Streaming == previousState )
    {
      OnStreamingEnd();
    }

    switch ( _state )
    {
      case MicStreamState::TransitionIn:
      {
        DEV_ASSERT_MSG( MicStreamState::Listening == previousState, "MicStreamingController.TransitionToState",
                        "Can only enter TransitionIn state from Listening state (currently in %s state)",
                        EnumToString( previousState ) );

        OnStreamingTransitionBegin();
        break;
      }

      case MicStreamState::Streaming:
      {
        DEV_ASSERT_MSG( MicStreamState::TransitionIn == previousState, "MicStreamingController.TransitionToState",
                       "Can only enter Streaming state from TransitionIn state (currently in %s state)",
                       EnumToString( previousState ) );

        OnStreamingBegin();
        break;
      }

      case MicStreamState::TransitionOut:
      {
        DEV_ASSERT_MSG( MicStreamState::Streaming == previousState, "MicStreamingController.TransitionToState",
                        "Can only enter TransitionOut state from Streaming state (currently in %s state)",
                        EnumToString( previousState ) );

        break;
      }

      case MicStreamState::Listening:
      {
        break;
      }
    }

    // send our stream state sync message to the engine
    RobotInterface::MicStreamingStateSync event =
    {
      .state        = _state,
      .isSimulated  = _streamingData.args.isSimulated,
    };

    AnimProcessMessages::SendAnimToEngine( event );
  }
  else
  {
    LOG_ERROR( "MicStreamingController.TransitionToState",
               "Attempting to transition into a state we are already in (%s)",
               EnumToString( _state ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::OnStreamingTransitionBegin()
{
  // we're going to attempt the stream (doesn't mean it will succeed)
  // something can go wrong in ShowAudioStreamStateManager and we can still fail (we'll get a callback with success/fail)

  ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();

  // wrap the streaming callback in our own so we know when streaming is about to begin
  // this callback is fired as soon as the "earcon" is finished
  // this is so that we do not include the earcon audio in the stream
  auto streamingCallback = [=]( bool success )
  {
    if ( success )
    {
      // all is good, let's kick off the streaming job
      TransitionToState( MicStreamState::Streaming );
    }
    else
    {
      // something went wrong, notify our callbacks that we're bailing
      // note: this is legacy and I don't like it
      NotifyTriggerWordDetectedCallbacks( false );

      // if we're not going to progress into streaming, then we're done with this stream job
      StopCurrentMicStreaming( MicStreamResult::Error );
    }
  };

  LOG_INFO( "MicStreamingController.OnStreamingTransitionBegin", "Starting ww transition to streaming (%s simulated)",
            _streamingData.args.isSimulated ? "is" : "not" );

  // tell everybody we're about to start streaming
  NotifyTriggerWordDetectedCallbacks( true );

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
    MicDataSystem::CloudStreamResponseCallback callback = std::bind( &MicStreamingController::OnCloudStreamResponseCallback,
                                                                     this,
                                                                     std::placeholders::_1 );

    _streamingData.streamJobCreated = _micDataSystem->StartMicStreamingJob( _streamingData.args.streamType, callback );
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
  // track the end time for both regular and simluated stream
  _streamingData.streamEndTime = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

  // only need to kill the streaming job if we've created one
  if ( _streamingData.streamJobCreated )
  {
    _micDataSystem->StopMicStreamingJob();
    _streamingData.streamJobCreated = false;
  }
  else
  {
    LOG_DEBUG( "MicStreamingController.OnStreamingEnd", "Stopping a simulated stream" );
  }

  NotifyStreamingStateChangedCallbacks( false );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::OnCloudStreamResponseCallback( MicStreamResult reason )
{
  if ( IsInStreamingState() )
  {
    // if our stream finished properly, transition out of our streaming state
    // this allows some extra time for the intent to make it's way to the behavior before we kill the stream
    if ( MicStreamResult::Success == reason )
    {
      TransitionToState( MicStreamState::TransitionOut );
    }
    else
    {
      // there was some sort of error, so kill the stream
      StopCurrentMicStreaming( reason );
    }
  }
  else
  {
    LOG_WARNING( "MicStreamingController.OnCloudStreamResponseCallback",
                 "Received StreamComplete callback but not in the Streaming state" );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::ShouldSimulateWakeWordStreaming() const
{
  ShowAudioStreamStateManager* streamAnimManager = _animContext->GetShowAudioStreamStateManager();

  const bool simulationRequested = streamAnimManager->ShouldSimulateStreamAfterTriggerWord();
  const bool isLowBattery = _micDataSystem->_batteryLow; // todo: jmeh - register for our own "battery callbacks"

  return ( isLowBattery || simulationRequested );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::CanStreamToCloud() const
{
  return ( !IsStreamSimulated() && _streamingData.streamJobCreated );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingController::SetRecognizerResponse( const std::string& responseId )
{
  if ( !responseId.empty() )
  {
    const auto it = _recognizerResponseMap.find( responseId );
    if ( it != _recognizerResponseMap.end() )
    {
      _recognizerResponse = it->second;
      LOG_INFO( "MicStreamingController.SetRecognizerResponse", "Updated recognizer response to (%s)",
                 responseId.c_str() );
    }
    else
    {
      _recognizerResponse = RecognizerResponse();
      LOG_ERROR( "MicStreamingController.SetRecognizerResponse",
                 "Attempted to set an invalid recognizer response (%s), disabling instead", responseId.c_str() );
    }
  }
  else
  {
    // default response is disabled
    _recognizerResponse = RecognizerResponse();
    LOG_INFO( "MicStreamingController.SetRecognizerResponse",
               "Disabling recognizer response since empty id was given" );
  }
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

} // namespace MicData
} // namespace Vector
} // namespace Anki
