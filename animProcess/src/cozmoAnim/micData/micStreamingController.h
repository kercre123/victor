/***********************************************************************************************************************
 *
 *  MicStreamingController .h
 *  Victor / Anim
 *
 *  Created by Jarrod Hatfield on 3/22/2019
 *  Copyright Anki, Inc. 2019
 *
 *  Description
 *  + Tracks the state of our mic recording jobs and cloud streaming
 *  + Handles the toggling of mic related backpack lights so that they're tied directly to the mic state
 *  + This was previously tracked through various member variables but became very hard to follow and update
 *
 **********************************************************************************************************************/

#ifndef __AnimProcess_MicStreamingController_H_
#define __AnimProcess_MicStreamingController_H_

#include "micDataTypes.h"
#include "coretech/common/shared/types.h"
#include "clad/audio/audioMessage.h"
#include "clad/cloud/mic.h"
#include "clad/types/micStreamingTypes.h"

#include <unordered_map>


namespace Anki {

  namespace AudioUtil {
    struct SpeechRecognizerCallbackInfo;
  }

  namespace AudioMetaData {
    namespace GameEvent {
      enum class GenericEvent : uint32_t;
    }
  }

namespace Vector {

  namespace Anim {
    class AnimationStreamer;
    class AnimContext;
  }

namespace MicData {

  class MicDataSystem;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MicStreamingController
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  MicStreamingController( MicDataSystem* micSystem );
  ~MicStreamingController();

  MicStreamingController( const MicStreamingController& )             = delete;
  MicStreamingController& operator=( const MicStreamingController& )  = delete;

  void Initialize( const Anim::AnimContext* animContext );
  void SetAnimationStreamer( Anim::AnimationStreamer* streamer )
  {
    _animationStreamer = streamer;
  }

  void Update();


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public API ...

  // Callback parameter is whether or not we will be streaming after the trigger word is detected
  using OnTriggerWordDetectedCallback = std::function<void(bool)>;
  void AddTriggerWordDetectedCallback( OnTriggerWordDetectedCallback callback )
  {
    _triggerWordDetectedCallbacks.push_back(callback);
  }

  // Callback parameter is true if streaming has begun, false if has stopped
  using OnStreamingStateChanged = std::function<void(bool)>;
  void AddStreamingStateChangedCallback( OnStreamingStateChanged callback )
  {
    _streamingStateChangedCallbacks.push_back(callback);
  }

  bool CanStartWakeWordStream() const;

  // this kicks off a new streaming job specific to receiving a wakeword event
  // call this when detecting the wakeword or when wanting to simulate hearing it (eg. button press)
  enum class WakeWordSource
  {
    Voice,
    Button,
    ButtonFromMute,
  };
  bool StartWakeWordStream( WakeWordSource, const AudioUtil::SpeechRecognizerCallbackInfo* = nullptr );


  // are we allowed to start a new stream at this time
  // limit to one stream at a time
  // Note: if this returns true, then a call to StartOpenMicStream(...) is guaranteed to be successful
  bool CanStartOpenMicStream() const;

  // this kicks off the start of a new streaming job
  // CloudMic::StreamType - what type of stream is this
  // bool - should we attempt to play the get-in animation or not
  // returns true if the process was started successfully, false otherwise (eg. we're already streaming)
  bool StartOpenMicStream( CloudMic::StreamType streamType, bool shouldPlayTransitionAnim );


  // kick off a new streaming job, bypassing any safety checks, and setting default parameters
  // this is meant to be kicked off from the cloud, but could be used whenever we want to force a stream for testing
  // * behavior is undefined if there is already a current stream open ... should be fine though *
  void StartCloudTestStream();

  // this ends the current streaming job and cleans up any state needed so that we are ready to begin a new stream
  void StopCurrentMicStreaming( MicStreamResult reason );

  // returns true if we kicked off the process of starting a new streaming job by calling StartOpenMicStream(...)
  // this includes actively streaming, as well as the transition animations in/our streaming
  bool HasStreamingBegun() const { return ( MicStreamState::Listening != _state ); }


  // custom recognizer responses control how we react to hearing the trigger word
  // we play the get-in animation (if set) from here so that we respond as quickly as possible, vs. playing an
  // animation from the engine which incurs latency from all of the messaging required
  // responseId comes from pre-set responses defined in micStreamingConfig.json
  void SetRecognizerResponse( const std::string& responseId );


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Structs and Enums ...

  struct StreamingArguments
  {
    CloudMic::StreamType    streamType                = CloudMic::StreamType::Normal;
    bool                    shouldPlayTransitionAnim  = false;
    bool                    shouldRecordTriggerWord   = false;
    bool                    isSimulated               = false;
  };

  struct StreamData
  {
    StreamingArguments      args;

    BaseStationTime_t       streamBeginTime           = 0;
    BaseStationTime_t       streamEndTime             = 0;
    bool                    streamJobCreated          = false;
    MicStreamResult         result                    = MicStreamResult::Success;
  };

  enum class RecognizerResponseType
  {
    ResponseEnabled,
    ResponseDisabled,
    ResponseSimulated,
  };

  inline const char* RecognizerResponseTypeToString( RecognizerResponseType type ) const;
  inline bool RecognizerResponseTypeFromString( const std::string& typeString, RecognizerResponseType& outType ) const;

  struct RecognizerResponse
  {
    RecognizerResponseType  type = RecognizerResponseType::ResponseDisabled;
    std::string             animation;
  };


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper Functions ...

  void LoadSettings( const Json::Value& config );
  void LoadStreamingReactions( const Json::Value& config );
  RecognizerResponse LoadRecognizerResponse( const Json::Value& config, const std::string& id );

  // kicks off the streaming process
  // this assumes all checks and requirements have already been taken care of (eg. !HasStreamingBegun())
  void StartStreamInternal( StreamingArguments args );

  // starts the transition from the current state to the specified state
  // use this function to move throughout the states
  void TransitionToState( MicStreamState nextState );

  // called whenever each "event" is hit, generally through state transition
  void OnStreamingTransitionBegin();
  void OnStreamingBegin();
  void OnStreamingEnd();

  // this tells all of our 'trigger word detected' callbacks that we've detected the trigger word and we're about
  // to start a stream as soon as the earcon finishes
  void NotifyTriggerWordDetectedCallbacks( bool willStream ) const;
  // this tells all of our callbacks that streaming has either started or stopped
  void NotifyStreamingStateChangedCallbacks( bool streamStarted ) const;

  // simulated streaming is when we make everything look like we're streaming normally, but we're not actually
  // sending any data to the cloud
  bool ShouldSimulateWakeWordStreaming() const;
  bool IsStreamSimulated() const { return _streamingData.args.isSimulated; }
  bool CanStreamToCloud() const;

  // returns true if we are in the streaming state (ignores transitional states)
  // note: this also includes simulated streaming state
  bool IsInStreamingState() const { return ( MicStreamState::Streaming == _state ); }
  // returns true if we're in the post-stream state, waiting for the engine to recieve it's data
  bool IsWaitingForCloudResponse() const { return ( MicStreamState::TransitionOut == _state ); }

  // this is called when the cloud let's us know that it's no longer streaming
  void OnCloudStreamResponseCallback( MicStreamResult reason );

  bool IsRecognizerResponseEnabled() const;

  // debug state strings
  inline const char* WakeWordSourceToString( WakeWordSource source ) const;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Variables ...

  MicDataSystem*            _micDataSystem      = nullptr;
  const Anim::AnimContext*  _animContext        = nullptr;
  Anim::AnimationStreamer*  _animationStreamer  = nullptr;

  MicStreamState            _state              = MicStreamState::Listening;
  StreamData                _streamingData;

  AudioMetaData::GameEvent::GenericEvent _streamingEarcon;

  // our trigger word comes in from a callbcak on a different thread, so we need to lock it down
  // when handling things
  mutable std::recursive_mutex  _recognizerResponseCallback;

  std::vector<OnTriggerWordDetectedCallback>  _triggerWordDetectedCallbacks;
  std::vector<OnStreamingStateChanged>        _streamingStateChangedCallbacks;

  RecognizerResponse                          _recognizerResponse;
  std::unordered_map<std::string, RecognizerResponse> _recognizerResponseMap;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* MicStreamingController::WakeWordSourceToString( WakeWordSource source ) const
{
  switch ( source )
  {
    case WakeWordSource::Voice:
      return "Voice";
    case WakeWordSource::Button:
      return "Button";
    case WakeWordSource::ButtonFromMute:
      return "ButtonFromMute";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* MicStreamingController::RecognizerResponseTypeToString( RecognizerResponseType type ) const
{
  switch ( type )
  {
    case RecognizerResponseType::ResponseEnabled:
      return "ResponseEnabled";
    case RecognizerResponseType::ResponseDisabled:
      return "ResponseDisabled";
    case RecognizerResponseType::ResponseSimulated:
      return "ResponseSimulated";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingController::RecognizerResponseTypeFromString( const std::string& typeString, RecognizerResponseType& outType ) const
{
  static const std::unordered_map<std::string, RecognizerResponseType> stringToEnumMap =
  {
    { "ResponseEnabled",    RecognizerResponseType::ResponseEnabled },
    { "ResponseDisabled",   RecognizerResponseType::ResponseDisabled },
    { "ResponseSimulated",  RecognizerResponseType::ResponseSimulated },
  };

  const auto it = stringToEnumMap.find( typeString );
  if ( it != stringToEnumMap.end() )
  {
    outType = it->second;
    return true;
  }

  return false;
}

} // namespace MicData
} // namespace Vector
} // namespace Anki

#endif // __AnimProcess_MicStreamingController_H_
