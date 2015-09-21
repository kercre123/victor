/**
 * File: demoBehaviorChooser.h
 *
 * Author: Lee Crippen
 * Created: 09/04/15
 *
 * Description: Class for handling picking of behaviors in a somewhat scripted demo.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_DemoBehaviorChooser_H__
#define __Cozmo_Basestation_DemoBehaviorChooser_H__

#include "anki/cozmo/basestation/behaviorChooser.h"
#include "json/json.h"
#include "clad/types/demoBehaviorState.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <cstdint>

namespace Anki {
namespace Cozmo {
  
// forward declarations
class IBehavior;
class BehaviorLookAround;
class BehaviorInteractWithFaces;
class BehaviorOCD;
class BehaviorFidget;
class Robot;
namespace ExternalInterface {
  class MessageGameToEngine;
}
template<typename T>class AnkiEvent;
  
class DemoBehaviorChooser : public ReactionaryBehaviorChooser
{
public:
  DemoBehaviorChooser(Robot& robot, const Json::Value& config);
  
  virtual IBehavior* ChooseNextBehavior(double currentTime_sec) const override;
  virtual Result Update(double currentTime_sec) override;
  virtual Result AddBehavior(IBehavior* newBehavior) override;
  
protected:
  Robot& _robot;
  std::vector<Signal::SmartHandle> _eventHandlers;
  
  DemoBehaviorState _demoState = DemoBehaviorState::Default;
  DemoBehaviorState _requestedState = _demoState;
  
  // Note these are for easy access - the inherited _behaviorList owns the memory
  BehaviorLookAround* _behaviorLookAround = nullptr;
  BehaviorInteractWithFaces* _behaviorInteractWithFaces = nullptr;
  BehaviorOCD* _behaviorOCD = nullptr;
  BehaviorFidget* _behaviorFidget = nullptr;
  
  void SetupBehaviors(Robot& robot, const Json::Value& config);
  void HandleSetDemoState(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
private:
  using super = ReactionaryBehaviorChooser;
  
}; // class DemoBehaviorChooser
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_DemoBehaviorChooser_H__