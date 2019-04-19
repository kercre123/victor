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

#include <vector>


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


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Structs and Enums ...



  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper Functions ...

  // handle messages coming from the anim process
  void HandleTriggerWordDetectedEvent( const AnkiEvent<RobotInterface::RobotToEngine>& );
  void HandleStreamingStateSyncEvent( const AnkiEvent<RobotInterface::RobotToEngine>& );

  // this tells all of our callbacks that the streaming process has begun and the current state of it
  bool NotifyStreamingEventCallbacks( StreamingEvent, StreamingEventData = {} ) const;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Variables ...

  Robot*              _robot          = nullptr;
  MicStreamState      _streamingState = MicStreamState::Listening;

  std::vector<OnStreamEventCallback>  _streamingEventCallbacks;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MicStreamingComponent::RegisterStreamEventCallback( OnStreamEventCallback callback )
{
  _streamingEventCallbacks.push_back( callback );
}

} // namespace Vector
} // namespace Anki

#endif // __EngineProcess_MicStreamingComponent_H_
