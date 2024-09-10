//
//  audioCallback.cpp
//  audioengine
//
//  Created by Jordan Rivas on 11/14/15.
//
//

#include "audioEngine/audioCallback.h"

namespace Anki {
namespace AudioEngine {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioCallbackContext::HandleCallback( const AudioCallbackInfo& callbackInfo )
{
  if ( nullptr != _eventCallbackFunc ) {

    // Check if Event callback want's callback type
    bool sendEvent = false;
    switch ( callbackInfo.callbackType ) {

      case AudioCallbackType::Duration:
        sendEvent = ((AudioCallbackFlag::Duration & _callbackFlags) == AudioCallbackFlag::Duration);
        break;

      case AudioCallbackType::Marker:
        sendEvent = ((AudioCallbackFlag::Marker & _callbackFlags) == AudioCallbackFlag::Marker);
        break;

      case AudioCallbackType::Complete:
        sendEvent = ((AudioCallbackFlag::Complete & _callbackFlags) == AudioCallbackFlag::Complete);
        break;

      case AudioCallbackType::Error:
        // Always send error events
        sendEvent = true;
        break;

        // TODO: Handle other callback types
      default:
        break;
    }

    if ( sendEvent ) {
      _eventCallbackFunc( this, callbackInfo );
    }
  }

  // FIXME: Waiting to hear back from WWise if Callback error will also call completed event
  if ( (AudioCallbackType::Complete == callbackInfo.callbackType ||
        AudioCallbackType::Error == callbackInfo.callbackType )
       && nullptr != _destroyCallbackFunc ) {
    _destroyCallbackFunc( this );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioCallbackContext::ClearCallbacks()
{
  _eventCallbackFunc    = nullptr;
  _destroyCallbackFunc  = nullptr;
}

}
}
