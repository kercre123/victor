//
//  audioCallback.h
//  audioengine
//
//  Created by Jordan Rivas on 11/14/15.
//
//

#ifndef __AnkiAudio_AudioCallback_H__
#define __AnkiAudio_AudioCallback_H__


#include "audioEngine/audioExport.h"
#include "audioEngine/audioTypes.h"

namespace Anki {
namespace AudioEngine {
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Audio Engine Global Callbacks
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

using AudioEngineCallbackId = uint32_t;
enum { kInvalidAudioEngineCallbackId = 0 };

enum AUDIOENGINE_EXPORT AudioEngineCallbackFlag
{
  NoAudioEngineCallbacks = 0,
  BegingFrameRender   = 1 << 0,   // Start of frame rendering, after having processed game messages
  EndFrameRender      = 1 << 1,   // End of frame rendering
  EndAudioProcessing  = 1 << 2    // End of audio processing
};

inline AudioEngineCallbackFlag operator| ( AudioEngineCallbackFlag a, AudioEngineCallbackFlag b )
{ return static_cast<AudioEngineCallbackFlag>( static_cast<int>(a) | static_cast<int>(b) ); }

using AudioEngineCallbackFunc = std::function<void ( AudioEngineCallbackId callbackId,
                                                     AudioEngineCallbackFlag callbackType )>;
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Audio Post Event Callbacks
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum AUDIOENGINE_EXPORT AudioCallbackFlag
{
  NoCallbacks   = 0,
  Duration      = 1 << 0,
  Marker        = 1 << 1,
  Complete      = 1 << 2,
  AllCallbacks  = 0xff
};

enum class AUDIOENGINE_EXPORT AudioCallbackType : uint8_t
{
  Invalid = 0,
  Duration,
  Marker,
  Complete,
  Error
};
  
enum class AUDIOENGINE_EXPORT AudioCallbackErrorType : uint8_t
{
  Invalid = 0,
  EventFailed,
  Starvation
};

struct AUDIOENGINE_EXPORT AudioCallbackInfo
{
  AudioGameObject   gameObjectId = kInvalidAudioGameObject;
  AudioCallbackType callbackType = AudioCallbackType::Invalid;
  
  AudioCallbackInfo( AudioGameObject gameObjectId, AudioCallbackType callbackType ) :
   gameObjectId( gameObjectId ),
   callbackType( callbackType ) {};

  virtual ~AudioCallbackInfo() = default;
  
  virtual const std::string GetDescription() const
  {
    const std::string description = "AudioCallback: GameObjectId: " + std::to_string(gameObjectId);
    return description;
  }
};

struct AUDIOENGINE_EXPORT AudioDurationCallbackInfo : public AudioCallbackInfo
{
  AudioReal32 duration;           // Duration of the sound in milliseconds
  AudioReal32 estimatedDuration;  // Estimated duration of the sound in milliseconds depending on the source settings
  // such as pitch.
  AudioUInt32 audioNodeId;        // Audio Node Id of playing item
  bool        isStreaming;        // True if source is streaming.
  
  AudioDurationCallbackInfo( AudioGameObject gameObjectId,
                             AudioReal32 duration,
                             AudioReal32 estimatedDuration,
                             AudioUInt32 audioNodeId,
                             bool isStreaming ) :
   AudioCallbackInfo ( gameObjectId, AudioCallbackType::Duration ),
   duration( duration ),
   estimatedDuration( estimatedDuration ),
   audioNodeId( audioNodeId ),
   isStreaming( isStreaming ) {}
  
  virtual const std::string GetDescription() const override
  {
    const std::string description = "AudioDurationCallback: GameObjectId: " + std::to_string(gameObjectId) +
    "  Duration: " + std::to_string(duration) + "ms  Estimated Duration: " + std::to_string(estimatedDuration) +
    " ms  AudioNodeId: " + std::to_string(audioNodeId) + ( isStreaming ? "  Is Streaming" : "  Not Streaming");
    return description;
  }
};

struct AUDIOENGINE_EXPORT AudioMarkerCallbackInfo : public AudioCallbackInfo
{
  AudioUInt32 identifier; // Cue point identifier
  AudioUInt32 position;   // Position (sample frame)
  const char* labelStr;   // Label of the marker
  
  AudioMarkerCallbackInfo( AudioGameObject gameObjectId,
                           AudioUInt32 identifier,
                           AudioUInt32 position,
                           const char* labelStr ) :
   AudioCallbackInfo ( gameObjectId, AudioCallbackType::Marker ),
   identifier( identifier ),
   position( position ),
   labelStr( labelStr ) {}
  
  virtual const std::string GetDescription() const override
  {
    const std::string description = "AudioMarkerCallback: GameObjectId: " + std::to_string(gameObjectId) +
    "  Identifier: " + std::to_string(identifier) + "  Position: " + std::to_string(position) +
    "(sample)  Label: " + std::string(labelStr);
    return description;
  }
};

struct AUDIOENGINE_EXPORT AudioCompletionCallbackInfo : public AudioCallbackInfo
{
  AudioPlayingId playId = kInvalidAudioPlayingId; // Play Id of Event
  AudioEventId  eventId = kInvalidAudioEventId;   // Performed event Id
  
  AudioCompletionCallbackInfo( AudioGameObject gameObjectId,
                               AudioPlayingId playId,
                               AudioEventId eventId ) :
   AudioCallbackInfo( gameObjectId, AudioCallbackType::Complete ),
   playId( playId ),
   eventId( eventId ) {}
  
  virtual const std::string GetDescription() const override
  {
    const std::string description = "AudioCompletionCallback: GameObjectId: " + std::to_string(gameObjectId) +
    "  PlayId: " + std::to_string(playId) + "  EventId: " + std::to_string(eventId);
    return description;
  }
};

struct AUDIOENGINE_EXPORT AudioErrorCallbackInfo : public AudioCallbackInfo
{
  AudioPlayingId        playId = kInvalidAudioPlayingId; // Play Id of Event
  AudioEventId         eventId = kInvalidAudioEventId;   // Performed event Id
  AudioCallbackErrorType error = AudioCallbackErrorType::Invalid;
  
  AudioErrorCallbackInfo( AudioGameObject gameObjectId,
                          AudioPlayingId playId,
                          AudioEventId eventId,
                          AudioCallbackErrorType error ) :
  AudioCallbackInfo( gameObjectId, AudioCallbackType::Error ),
  playId( playId ),
  eventId( eventId ),
  error( error ) {}
  
  virtual const std::string GetDescription() const override
  {
    const std::string description = "AudioErrorCallbackInfo: GameObjectId: " + std::to_string(gameObjectId) +
    "  EventId: " + std::to_string(eventId) + "  ErrorVal: " +
    std::to_string((uint8_t)error);
    return description;
  }
};

class AUDIOENGINE_EXPORT AudioCallbackContext
{
public:

  // Set callback flags for Post Event Caller
  void SetCallbackFlags( AudioCallbackFlag callbackFlags ) { _callbackFlags = callbackFlags; }
  AudioCallbackFlag GetCallbackFlags() const { return _callbackFlags; }
  
  // Event callback function for Post Event Caller
  using AudioEventCallbackFunc = std::function<void ( const AudioCallbackContext* thisContext,
                                                      const AudioCallbackInfo& callbackInfo )>;
  void SetEventCallbackFunc( AudioEventCallbackFunc&& callbackFunc ) { _eventCallbackFunc = std::move( callbackFunc ); }
  
  // This should be set by the Context owner
  void SetPlayId( AudioPlayingId playId ) { _playId = playId; }
  AudioPlayingId GetPlayId() const { return _playId; }
  
  // Destory callback function for Context owner
  using AudioDestoryCallbackFunc = std::function<void ( const AudioCallbackContext* thisContext )>;
  void SetDestroyCallbackFunc( AudioDestoryCallbackFunc&& callbackFunc) { _destroyCallbackFunc = std::move( callbackFunc ); }
  
  // Called by AudioEngineController to trigger the callback
  void HandleCallback( const AudioCallbackInfo& callbackInfo );
  
  // Clear all callback funcs
  void ClearCallbacks();

  // Sets whether this callback will execute on the thread it's invoked from (async=true)
  // or stored in a queue for processing in the engine update (async=false)
  void SetExecuteAsync( const bool async ) { _executeAsync = async; }
  bool GetExecuteAsync() const { return _executeAsync; }
  
private:
  
  AudioCallbackFlag         _callbackFlags        = AudioCallbackFlag::NoCallbacks;
  AudioPlayingId            _playId               = kInvalidAudioPlayingId;
  AudioEventCallbackFunc    _eventCallbackFunc    = nullptr;
  AudioDestoryCallbackFunc  _destroyCallbackFunc  = nullptr;
  bool                      _executeAsync         = true;
  
};

}
}

#endif /* __AnkiAudio_AudioCallback_H__ */
