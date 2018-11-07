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

#include "engine/aiComponent/aiComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>

namespace Anki {

namespace Util {
  class IConsoleFunction;
}
  
namespace Vector {

namespace external_interface {
  class GatewayWrapper;
}
namespace RobotInterface {
  class RobotToEngine;
}

  
enum class AlexaAuthState : uint8_t;

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

private:
  
  void SetAlexaOption( bool optedIn ) const;
  
  void HandleAppEvents( const AnkiEvent<external_interface::GatewayWrapper>& event );
  void HandleAnimEvents( const AnkiEvent<RobotInterface::RobotToEngine>& event );
  
  void SendAuthStateToApp( bool isResponse ) const;
  
  // tell anim to cancel any pending auth, but not any completed auth
  void SendCancelPendingAuth() const;
  
  Robot& _robot;
  std::list<Signal::SmartHandle> _signalHandles;
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
  
  AlexaAuthState _authState;
  std::string _authStateExtra;
  
  bool _featureFlagEnabled = false;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_AlexaComponent_H__
