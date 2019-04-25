/***********************************************************************************************************************
 *
 *  MicStreamingComponentTypes .h
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 4/08/2019
 *  Copyright Anki, Inc. 2019
 *
 *  Description
 *  + Types used when interacting with the MicStreamingComponent
 *  + This streaming system is exclusively used by the behavior system and acts as the engine/behavior side API
 *    for mic streaming
 *
 **********************************************************************************************************************/

#ifndef __EngineProcess_MicStreamingComponentTypes_H_
#define __EngineProcess_MicStreamingComponentTypes_H_

#include "engine/components/mics/micDirectionTypes.h"
#include "clad/types/micStreamingTypes.h"


namespace Anki {

  namespace AudioMetaData {
  namespace GameEvent {
    enum class GenericEvent : uint32_t;
  }
  }

namespace Vector {

  enum class AnimationTrigger : int32_t;

enum class StreamingEvent : uint8_t
{
  TriggerWordDetected,    // we've detected a trigger word and have begun prepping the stream
  StreamProcessBegin,     // we are prepping to stream mic data (independent of TriggerWordDetected)
  StreamOpen,             // we're actively streaming mic data (may not happen in all cases)
  StreamClosed,           // an open stream was closed
  StreamProcessEnd,       // we've finished the streaming process (may or may not have actively streamed)
};

inline const char* StreamingEventToString( StreamingEvent event );

struct StreamEventTriggerWord
{
  MicDirectionIndex   direction;          // what direction did we hear the trigger word
  bool                isStreamSimulated;  // are we going to open up a cloud stream after the wakeword transition?
};

struct StreamEventStreamState
{
  bool                isSimulated;  // are we actually streaming to the cloud, or faking it
};

union StreamingEventData
{
  StreamEventTriggerWord  triggerWordData;  // only valid with TriggerWordDetected event
  StreamEventStreamState  streamStateData;   // valid with StreamProcessBegin, StreamOpen, StreamClosed, StreamProcessEnd
};

// returns true if the event was "handled", false otherwise
using OnStreamEventCallback = std::function<bool(StreamingEvent, StreamingEventData)>;



inline const char* StreamingEventToString( StreamingEvent event )
{
  switch ( event )
  {
    case StreamingEvent::TriggerWordDetected:   return "TriggerWordDetected";
    case StreamingEvent::StreamProcessBegin:    return "StreamProcessBegin";
    case StreamingEvent::StreamOpen:            return "StreamOpen";
    case StreamingEvent::StreamClosed:          return "StreamClosed";
    case StreamingEvent::StreamProcessEnd:      return "StreamProcessEnd";
  }
}

struct RecognizerBehaviorResponse
{
  AnimationTrigger animLooping;
  AnimationTrigger animGetOut;

  AudioMetaData::GameEvent::GenericEvent earConSuccess;
  AudioMetaData::GameEvent::GenericEvent earConFail;
};

} // namespace Vector
} // namespace Anki

#endif // __EngineProcess_MicStreamingComponentTypes_H_
