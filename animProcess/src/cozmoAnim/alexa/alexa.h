/**
 * File: alexa.h
 *
 * Author: ross
 * Created: Oct 16 2018
 *
 * Description: Wrapper for component that integrates the Alexa Voice Service (AVS) SDK. Alexa is an opt-in
 *              feature, so this class handles communication with the engine to opt in and out, and is
 *              otherwise a pimpl-style wrapper, although this class does a fair amount since the impl can be deleted
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#ifndef ANIMPROCESS_COZMO_ALEXA_H
#define ANIMPROCESS_COZMO_ALEXA_H
#pragma once

#include "audioEngine/audioTypes.h"
#include "audioUtil/audioDataTypes.h"
#include <memory>
#include <string>
#include <mutex>
#include <future>
#include <list>

namespace Anki {
namespace AudioEngine {
class AudioCallbackContext;
}
namespace Util {
  class IConsoleFunction;
  class Locale;
}
namespace Vector {
  
class AlexaImpl;
namespace Anim {
  class AnimContext;
}
enum class AlexaAuthState : uint8_t;
enum class AlexaNetworkErrorType : uint8_t;
enum class AlexaSimpleState : uint8_t;
enum class AlexaUXState : uint8_t;
enum class ScreenName : uint8_t;

class Alexa
{
public:
  
  Alexa();
  ~Alexa();
  
  void Init(const Anim::AnimContext* context);
  
  void Update();
  
  // handles message from engine to opt in or out
  void SetAlexaUsage(bool optedIn);

  // cancels any pending authorization started by the user (not an auto-auth during reboot). Reason is
  // included in a DAS message
  void CancelPendingAlexaAuth(const std::string& reason);
  
  void OnEngineLoaded();
  
  // Adds samples to the mic stream buffer. Should be ok to call on another thread
  void AddMicrophoneSamples( const AudioUtil::AudioSample* const samples, size_t nSamples ) const;
  
  bool StopAlertIfActive();
  
  void NotifyOfTapToTalk( bool fromMute );
  
  void NotifyOfWakeWord( uint64_t fromSampleIndex, uint64_t toSampleIndex );
  
  void UpdateLocale( const Util::Locale& locale );
  
  // Get the number of audio samples that have been added to Alexa "Microphone" component
  uint64_t GetMicrophoneSampleIndex() const;

  // Whether there is a session that is active or in the process of initializing.
  // Assumes that the existence of the impl is still tied to opt-in state (which may change)
  bool IsOptedIn() const { return HasImpl(); }
  
  void SetFrozenOnCharger(bool frozenOnCharger) { _frozenOnCharger = frozenOnCharger; }
  void SetOnCharger(bool onCharger) { _onCharger = onCharger; }

protected:
  // explicitly declare noncopyable (Util::noncopyable doesn't play well with movable)
  Alexa(const Alexa& other) = delete;
  Alexa& operator=(const Alexa& other) = delete;
  
private:
  

  // decides whether to create/destroy the impl. If !active, then deleteUserData decides whether user data will be cleared
  void SetAlexaActive( bool active, bool deleteUserData = false );
  
  // called when SDK auth state changes
  void OnAlexaAuthChanged( AlexaAuthState state, const std::string& url, const std::string& code, bool errFlag );
  
  // called when SDK dialog or media player state changes. Note this will never send Error. The state should
  // be switch to Error only when OnAlexaNetworkError is called
  void OnAlexaUXStateChanged( AlexaUXState newState );
  
  // called when there is a user-facing error. This will set the ux state to Error for some period of time
  void OnAlexaNetworkError( AlexaNetworkErrorType errorType );
  
  // called when SDK requests that we logout
  void OnLogout();
  
  // called when SDK changes whether an indicator for notifications should be shown
  void OnNotificationsChanged( bool hasNotification ) const;
  
  void CreateImpl();
  void DeleteImpl();
  bool HasImpl() const { return _impl != nullptr; }
  bool HasInitializedImpl() const; // done loading sdk but maybe not done connecting yet
  
  // sets this class's _authState and messages engine if it changes
  void SetAuthState( AlexaAuthState state, const std::string& url="", const std::string& code="" );
  // sets this class's _uxState and messages engine if it changes
  void SetUXState( AlexaUXState newState );
  
  // Helper to tell MicDataSystem whether the wake word should be active
  void SetSimpleState( AlexaSimpleState state ) const;
  
  void PlayErrorAudio( AlexaNetworkErrorType errorType );
  bool IsErrorPlaying() const { return (_timeToEndError_s >= 0.0f); }
  
  // messages engine
  void SendAuthState();
  void SendUXState();

  void SendStatesToWebViz() const;
  
  // Sets the face info screen to screenName, possibly with a url (intentional copy) and code
  void SetAlexaFace( ScreenName screenName, std::string url="", const std::string& code="" ) const;
  
  // helpers for the file that indicates whether the last robot run ended during an authenticated session
  const std::string& GetPersistentFolder() const;
  void TouchOptInFiles() const;
  bool DidAuthenticateEver() const;
  bool DidAuthenticateLastBoot() const;
  void DeleteOptInFile() const;
  void DeleteUserFiles() const;
  
  // Play Audio Event Helper
  // Use new create AudioCallbackContext instance, hand off ownership when passing in to method
  void PlayAudioEvent( AudioEngine::AudioEventId eventId, AudioEngine::AudioCallbackContext* callback = nullptr ) const;
  
  std::unique_ptr<AlexaImpl> _impl;
  AlexaImpl* _implToBeDeleted = nullptr;
  std::future<void> _implDtorResult;
  
  const Anim::AnimContext* _context = nullptr;
  
  AlexaAuthState _authState;
  std::string _authExtra;
  bool _engineLoaded = false;
  bool _pendingAuthMsgs = false;
  bool _pendingUXMsgs = false;
  
  AlexaUXState _uxState;
  AlexaUXState _pendingUXState; // during AlexaUXState::Error, this is pending to be re-assigned to _uxState
  
  // If non-negative, turn on the wakeword at this time, even if not connected, so we can play error states
  float _timeEnableWakeWord_s = -1.0f;
  // If non-negative, this is the time that the AlexaUXState::Error ends, restoring _pendingUXState
  float _timeToEndError_s = -1.0f;
  
  enum class NotifyType : uint8_t {
    None = 0,
    Voice,
    Button,
    ButtonFromMute,
  };
  
  NotifyType _notifyType = NotifyType::None;
  
  std::unique_ptr<Util::Locale> _locale;
  bool _pendingLocale = false;

  // whether a message was received from engine saying to opt in. this gets reset after auth completes
  bool _authStartedByUser = false;
  // if during an authentication the state was ever WaitingForCode, this is the most recent code
  std::string _previousCode;
  
  // if the user was ever authenticated, even in a previous boot
  bool _authenticatedEver = false;
  
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
  
  // guards access to _impl when releasing it on main thread, and when used in NotifyOfWakeWord
  // and AddMicrophoneSamples, which can be called off thread
  mutable std::mutex _implMutex;
  
  bool _frozenOnCharger = false;
  bool _onCharger = false;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
