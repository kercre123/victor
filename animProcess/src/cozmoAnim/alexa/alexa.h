/**
 * File: alexa.h
 *
 * Author: ross
 * Created: Oct 16 2018
 *
 * Description: Wrapper for component that integrates the Alexa Voice Service (AVS) SDK. Alexa is an opt-in
 *              feature, so this class handles communication with the engine to opt in and out, and is
 *              otherwise primarily a pimpl wrapper.
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#ifndef ANIMPROCESS_COZMO_ALEXA_H
#define ANIMPROCESS_COZMO_ALEXA_H

#include <memory>
#include <string>

namespace Anki {
namespace Vector {
  
class AlexaImpl;
class AnimContext;
enum class AlexaAuthState : uint8_t;

class Alexa
{
public:
  
  Alexa();
  ~Alexa();
  
  // movable (and implicitly noncopyable)
  Alexa(Alexa&& other) noexcept;
  Alexa& operator=(Alexa&& other) noexcept;
  
  void Init(const AnimContext* context);
  
  void Update();
  
  // handles message from engine to opt in or out
  void SetAlexaUsage(bool optedIn);
  // handles message from engine to cancel any pending authorization
  void CancelPendingAlexaAuth();
  
  void OnEngineLoaded();

protected:
  // explicitly declare noncopyable (Util::noncopyable doesn't play well with movable)
  Alexa(const Alexa& other) = delete;
  Alexa& operator=(const Alexa& other) = delete;
  
private:
  
  // decides whether to create/destroy the impl
  void SetAlexaActive(bool active);
  
  // called when SDK auth state changes
  void OnAlexaAuthChanged( AlexaAuthState state, const std::string& url, const std::string& code );
  
  void CreateImpl();
  void DeleteImpl();
  bool HasImpl() const { return _impl != nullptr; }
  
  // sets this class's _authState and messages engine if it changes
  void SetAuthState( AlexaAuthState state, const std::string& extra="" );
  // actually messages engine
  void SendAuthState();
  
  // helpers for the file that indicates whether the last robot run ended during an authenticated session
  const std::string& GetOptInFilePath() const;
  void TouchOptInFile() const;
  bool DidAuthenticatePreviously() const;
  void DeleteOptInFile() const;
  
  
  std::unique_ptr<AlexaImpl> _impl;
  
  const AnimContext* _context = nullptr;
  
  AlexaAuthState _authState;
  std::string _authExtra;
  bool _engineLoaded = false;
  bool _pendingEngineMsgs = false;
  
  // whether a message was received from engine saying to opt in
  bool _userOptedIn = false;
  // if during an authentication the state was ever WaitingForCode, this is the most recent code
  std::string _previousCode;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
