/*
 * File: audioCallback.h
 *
 * Author: Jordan Rivas
 * Created: 9/15/17
 *
 * Description: This class simplifies audio event callback functionality by providing one object to contain the various
 *              types of callback messages the audio engine provides. When the caller receives an event callback they
 *              can determine the type and respond appropriately.
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#ifndef __AnkiAudio_AudioCallback_H__
#define __AnkiAudio_AudioCallback_H__

#include "audioEngine/audioExport.h"
#include "clad/audio/audioCallbackMessage.h"
#include <assert.h>
#include <string>


namespace Anki {
namespace AudioEngine {
namespace Multiplexer {

class AUDIOENGINE_EXPORT AudioCallback
{
  
public:
  
  using CallbackIdType = uint16_t;
  
  // Callback specific Constructors
  // Does not accept an invalid callback pointer
  AudioCallback( const AudioCallbackDuration* callback )
  : _callbackId( callback->callbackId )
  , _type( CallbackType::Duration )
  , _callbackDuration( callback )
  { }
  
  AudioCallback( const AudioCallbackMarker* callback )
  : _callbackId( callback->callbackId )
  , _type( CallbackType::Marker )
  , _callbackMarker( callback )
  { }
  
  AudioCallback(const AudioCallbackComplete* callback )
  : _callbackId( callback->callbackId )
  , _type( CallbackType::Complete )
  , _callbackComplete( callback )
  { }
  
  AudioCallback( const AudioCallbackError* callback )
  : _callbackId( callback->callbackId )
  , _type( CallbackType::Error )
  , _callbackError( callback )
  { }
  
  
  CallbackIdType GetId() const { return _callbackId; }
  CallbackType GetType() const { return _type; }
  
  const AudioCallbackDuration&  GetCallbackDuration() const { assert(CallbackType::Duration==_type); return *_callbackDuration; }
  const AudioCallbackMarker&    GetCallbackMarker()   const { assert(CallbackType::Marker==_type); return *_callbackMarker; }
  const AudioCallbackComplete&  GetCallbackComplete() const { assert(CallbackType::Duration==_type); return *_callbackComplete; }
  const AudioCallbackError&     GetCallbackError()    const { assert(CallbackType::Error==_type); return *_callbackError; }
  
  const std::string GetDescription() const
  {
    switch ( _type ) {
      case CallbackType::Duration:
        return ("Callback Id " +  std::to_string( _callbackDuration->callbackId ) +
                " Callback Duration " + std::to_string(_callbackDuration->duration));
        break;
      case CallbackType::Marker:
        return ("Callback Id " +  std::to_string( _callbackMarker->callbackId ) +
                " Callback Marker " + _callbackMarker->labelTitle);
        break;
      case CallbackType::Complete:
        return ("Callback Id " +  std::to_string( _callbackComplete->callbackId ) +
                " Complete EventId " + std::to_string((uint32_t)_callbackComplete->eventType));
        break;
      case CallbackType::Error:
        return ("Callback Id " +  std::to_string( _callbackError->callbackId ) +
                " Callback Error " + std::to_string((uint8_t) _callbackError->callbackError));
        break;
      case CallbackType::Invalid:
        return "Callback Invalid";
        break;
    }
    return "";
  }
  
  
private:
  
  CallbackIdType  _callbackId = 0;
  CallbackType    _type = CallbackType::Invalid;
  union {
    const AudioCallbackDuration*  _callbackDuration;
    const AudioCallbackMarker*    _callbackMarker;
    const AudioCallbackComplete*  _callbackComplete;
    const AudioCallbackError*     _callbackError;
  };
};

} // Multiplexer
} // AudioEngine
} // Anki


#endif /* __AnkiAudio_AudioCallback_H__ */
