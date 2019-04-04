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
// #include "util/global/globalDefinitions.h"
#include "clad/cloud/mic.h"


namespace Anki {
namespace Vector {

  namespace Anim {
    class AnimContext;
  }

namespace MicData {

  class MicDataSystem;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MicStreamingController
{
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Structs and Enums ...

  enum class MicState : uint8_t
  {
    Listening,
    TransitionToStreaming,
    Streaming,
  };


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  MicStreamingController( MicDataSystem* micSystem );
  ~MicStreamingController();

  MicStreamingController( const MicStreamingController& )             = delete;
  MicStreamingController& operator=( const MicStreamingController& )  = delete;

  void Initialize( const Anim::AnimContext* animContext );


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public API ...

  // Callback parameter is whether or not we will be streaming after the trigger word is detected
  using OnTriggerWordDetectedCallback = std::function<void(bool)>;
  void AddTriggerWordDetectedCallback( OnTriggerWordDetectedCallback callback )
  {
    _triggerWordDetectedCallbacks.push_back(callback);
  }

  // are we allowed to start a new stream at this time
  // limit to one stream at a time
  // must have appropriate "permission" from the engine
  bool CanBeginStreamingJob() const;

  // this kicks off the start of a new streaming job
  // returns true if the process was started successfully, false otherwise (eg. we're already streaming)
  struct StreamingArguments
  {
    CloudMic::StreamType      streamType; // what type of stream is this
    bool                      shouldPlayTransitionAnim; // attempt to play the get-in animation or not
    bool                      shouldRecordTriggerWord;
  };
  bool BeginStreamingJob( StreamingArguments args );

  // this ends the current streaming job and cleans up any state needed so that we are ready to begin a new stream
  void EndStreamingJob();

  // returns true if we kicked off the process of starting a new streaming job by calling BeginStreamingJob(...)
  // this includes actively streaming, as well as the transition animations into streaming
  bool HasStreamingBegun() const { return ( MicState::Listening != _state ); }
  // returns true if we are actively in the midst of streaming mic data to the cloud
  // does not include the transition animations into streaming
  bool IsActivelyStreaming() const { return ( MicState::Streaming == _state ); }

  bool IsMicsMuted() const { return _isMuted; }
  void SetMicsMuted( bool isMuted );


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper Functions ...

  // starts the transition from the current state to the specified state
  // use this function to move throughout the states
  void TransitionToState( MicState nextState );

  // called whenever each "event" is hit, generally through state transition
  void OnStreamingTransitionBegin();
  void OnStreamingBegin();
  void OnStreamingEnd();

  // this tells all of our 'trigger word detected' callbacks that we've detected the trigger word and we're about
  // to start a stream as soon as the earcon finishes
  void NotifyTriggerWordDetectedCallbacks( bool willStream ) const;

  // debug state strings
  const char* GetStateString( MicState state ) const;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Variables ...

  MicDataSystem*            _micDataSystem  = nullptr;
  const Anim::AnimContext*  _animContext    = nullptr;

  MicState                  _state          = MicState::Listening;
  bool                      _isMuted        = false;

  StreamingArguments        _streamingArgs;

  std::vector<OnTriggerWordDetectedCallback> _triggerWordDetectedCallbacks;
};

} // namespace MicData
} // namespace Vector
} // namespace Anki

#endif // __AnimProcess_MicStreamingController_H_
