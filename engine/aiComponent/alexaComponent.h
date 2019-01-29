/**
* File: alexaComponent.h
*
* Author: ross
* Created: Oct 16 2018
*
* Description: Component that manages Alexa interactions among anim, engine, and app
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_AlexaComponent_H__
#define __Engine_AiComponent_AlexaComponent_H__
#pragma once

#include "anki/cozmo/shared/animationTag.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/aiComponent/alexaComponentTypes.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <array>

namespace Anki {

namespace Util {
  class IConsoleFunction;
}
namespace AudioMetaData {
  namespace GameEvent {
    enum class GenericEvent : uint32_t;
  }
}
  
namespace Vector {

namespace external_interface {
  class GatewayWrapper;
}
namespace RobotInterface {
  class RobotToEngine;
  enum class CancelAlexaFromEngineReason : uint8_t;
}

  
enum class AlexaAuthState : uint8_t;
enum class AlexaUXState : uint8_t;
enum class AnimationTrigger : int32_t;
  
template<typename T>
class AnkiEvent;
  
class UserIntentComponent;

class AlexaComponent : public IDependencyManagedComponent<AIComponentID>,
                       private Util::noncopyable
{
public:
  AlexaComponent(Robot& robot);
  ~AlexaComponent() = default;
  
  virtual void InitDependent(Robot* robot, const AICompMap& dependentComps) override;
  
  virtual void AdditionalUpdateAccessibleComponents(AICompIDSet& components) const override;
  virtual void UpdateDependent(const AICompMap& dependentComps) override;


  // Set and reset get in anims and audio events for transitions from Idle to: Listening, Thinking, Speaking.
  // If the a given state is not provided as a key, it will have no response (anim nor audio). You can also
  // play audio without an anim by passing InvalidAnimTrigger.
  // Note this doesn't match the stack-style responses used in UserIntentComponent, because we only expect one
  // alexa behavior for now, and hence one response
  void SetAlexaUXResponses( const AlexaResponseMap& responses );
  void ResetAlexaUXResponses() { SetAlexaUXResponses({}); }
  
  AlexaUXState GetUXState() const { return _uxState; }
  bool IsIdle() const;
  bool IsUXStateGetInPlaying( AlexaUXState state ) const;
  bool IsAnyUXStateGetInPlaying() const;
  
  // check if sign in/out is pending. If it is pending, AlexaComponent will SignIn/SignOut for you
  // after some number of ticks. To avoid this, Claim it, but then you must call SignIn/SignOut. Note
  // that only one request is allowed until SignIn() or SignOut() is called (or a pending request times out).
  bool IsSignInPending() const;
  bool IsSignOutPending() const;
  void ClaimRequest();
  void SignIn();
  void SignOut();

private:
  
  void SetAlexaOption( bool optedIn );
  
  void HandleAppEvents( const AnkiEvent<external_interface::GatewayWrapper>& event );
  void HandleAnimEvents( const AnkiEvent<RobotInterface::RobotToEngine>& event );
  
  void HandleNewUXState( AlexaUXState state);
  
  void SendAuthStateToApp( bool isResponse );
  
  void ToggleButtonWakewordSetting( bool isAlexa ) const;
  
  std::string GetAnimName( AnimationTrigger trigger ) const;

  void SendSignInDAS( UserIntentSource source ) const;
  void SendSignOutDAS( UserIntentSource source ) const;
  
  enum class Request : uint8_t {
    None=0,
    SignInApp, // app or console var
    SignOutApp, // app or console var
    SignInVC,
    SignOutVC,
  };
  
  void SetRequest( Request request );
  
  Robot& _robot;
  UserIntentComponent* _uic = nullptr;
  std::list<Signal::SmartHandle> _signalHandles;
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
  
  AlexaAuthState _authState;
  std::string _authStateExtra;
  
  AlexaUXState _uxState;
  float _lastUxStateTransition_s = 0.0f;
  
  struct AlexaUXResponseInfo
  {
    void SetWaiting( bool waiting ); // sets waitingForGetInCompletion and timeout_s, or clears both
    
    bool hasAnim = false;
    
    bool waitingForGetInCompletion = false;
    float timeout_s = -1.0f;
  };
  std::unordered_map<AlexaUXState, AlexaUXResponseInfo> _uxResponseInfo;
  std::array<AnimationTag,4> _animTags;
  
  bool _pendingAuthIsFromOptIn = false;
  
  bool _featureFlagEnabled = false;
  
  Request _request = Request::None;
  size_t _requestTimeout = 0;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_AlexaComponent_H__
