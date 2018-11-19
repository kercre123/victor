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
}

  
enum class AlexaAuthState : uint8_t;
enum class AlexaUXState : uint8_t;
enum class AnimationTrigger : int32_t;
  
template<typename T>
class AnkiEvent;

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

private:
  
  void SetAlexaOption( bool optedIn );
  
  void HandleAppEvents( const AnkiEvent<external_interface::GatewayWrapper>& event );
  void HandleAnimEvents( const AnkiEvent<RobotInterface::RobotToEngine>& event );
  
  void HandleNewUXState( AlexaUXState state);
  
  void SendAuthStateToApp( bool isResponse );
  
  // tell anim to cancel any pending auth, but not any completed auth
  void SendCancelPendingAuth() const;
  
  void ToggleButtonWakewordSetting( bool isAlexa ) const;
  
  std::string GetAnimName( AnimationTrigger trigger ) const;
  
  Robot& _robot;
  std::list<Signal::SmartHandle> _signalHandles;
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
  
  AlexaAuthState _authState;
  std::string _authStateExtra;
  
  AlexaUXState _uxState;
  
  struct AlexaUXResponseInfo
  {
    bool waitingForGetInCompletion = false;
    bool hasAnim = false;
    float timeout_s = -1.0f;
  };
  std::unordered_map<AlexaUXState, AlexaUXResponseInfo> _uxResponseInfo;
  std::array<AnimationTag,4> _animTags;
  
  bool _pendingAuthIsFromOptIn = false;
  
  bool _featureFlagEnabled = false;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_AlexaComponent_H__
