/***********************************************************************************************************************
 *
 *  MicStreamingComponent .h
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 4/08/2019
 *  Copyright Anki, Inc. 2019
 *
 *  Description
 *  + Tracks the state of streaming that is controlled from the anim process/thread
 *  + Access for engine side responses to trigger word
 *  + API for kicking off non-automated mic streaming from the engine side
 *
 **********************************************************************************************************************/

#ifndef __EngineProcess_MicStreamingComponent_H_
#define __EngineProcess_MicStreamingComponent_H_

#include "engine/events/ankiEvent.h"

// #include "engine/components/mics/micStreamingComponentTypes.h"
#include "micStreamingComponentTypes.h"

#include "coretech/common/shared/types.h"
#include "clad/types/micStreamingTypes.h"
#include "clad/cloud/mic.h"
#include "util/signals/signalHolder.h"
#include "util/stringTable/stringID.h"

#include <unordered_map>
#include <vector>
#include <list>
#include <utility>


namespace Anki {
namespace Vector {

  class Robot;

  namespace RobotInterface
  {
    class RobotToEngine;
  }


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MicStreamingComponent : private Util::SignalHolder
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  MicStreamingComponent();
  ~MicStreamingComponent();

  MicStreamingComponent( const MicStreamingComponent& )             = delete;
  MicStreamingComponent& operator=( const MicStreamingComponent& )  = delete;

  void Initialize( Robot* robot );

  void Update();


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public API ...

  inline void RegisterStreamEventCallback( OnStreamEventCallback callback );

  bool HasStreamingProcessBegun() const { return ( MicStreamState::Listening != _streamingState ); }
  bool IsActivelyStreaming() const      { return ( MicStreamState::Streaming == _streamingState ); }

  void StopCurrentStream( MicStreamResult reason ) const;


  // Trigger Recognizer Responses
  void PushRecognizerBehaviorResponse( Util::StringID sourceId, std::string responseId );
  void PopRecognizerBehaviorResponse( Util::StringID sourceId );

  bool HasRecognizerResponse() const { return !_activeRecognizerResponses.empty(); }
  // returns the active trigger word response if one exists, else returns an invalid one
  // call HasRecognizerResponse() prior to this to ensure that one exists
  RecognizerBehaviorResponse GetRecognizerBehaviorResponse() const;


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Structs and Enums ...

  using RecognizerResponsePair = std::pair<Util::StringID, std::string>;
  inline friend bool operator==( const RecognizerResponsePair& lhs, const RecognizerResponsePair& rhs );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper Functions ...

  void LoadStreamingReactions( const Json::Value& config );
  RecognizerBehaviorResponse LoadRecognizerBehaviorResponse( const Json::Value& config, const std::string& id );

  // handle messages coming from the anim process
  void HandleTriggerWordDetectedEvent( const AnkiEvent<RobotInterface::RobotToEngine>& );
  void HandleStreamingStateSyncEvent( const AnkiEvent<RobotInterface::RobotToEngine>& );

  // this tells all of our callbacks that the streaming process has begun and the current state of it
  bool NotifyStreamingEventCallbacks( StreamingEvent, StreamingEventData = {} ) const;

  inline bool IsValidRecognizerResponse( const std::string& responseId ) const;
  void OnActiveRecognizerResponseChanged();

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Variables ...

  Robot*              _robot          = nullptr;
  MicStreamState      _streamingState = MicStreamState::Listening;

  std::vector<OnStreamEventCallback>  _streamingEventCallbacks;

  // list of all of the responses that behaviors have requested
  // priority goes in order of most recently pushed
  std::list<RecognizerResponsePair> _activeRecognizerResponses;

  // mapping of id's to reponse struct
  std::unordered_map<std::string, RecognizerBehaviorResponse> _streamingResponseMap;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::RegisterStreamEventCallback( OnStreamEventCallback callback )
{
  _streamingEventCallbacks.push_back( callback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MicStreamingComponent::IsValidRecognizerResponse( const std::string& responseId ) const
{
  return ( _streamingResponseMap.find( responseId ) != _streamingResponseMap.end() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool operator==( const MicStreamingComponent::RecognizerResponsePair& lhs, const MicStreamingComponent::RecognizerResponsePair& rhs )
{
  return ( lhs.first == rhs.first );
}

} // namespace Vector
} // namespace Anki

#endif // __EngineProcess_MicStreamingComponent_H_
