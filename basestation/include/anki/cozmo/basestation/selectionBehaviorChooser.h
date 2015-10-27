/**
 * File: selectionBehaviorChooser.h
 *
 * Author: Lee Crippen
 * Created: 10/15/15
 *
 * Description: Class for allowing specific behaviors to be selected.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_SelectionBehaviorChooser_H__
#define __Cozmo_Basestation_SelectionBehaviorChooser_H__

#include "anki/cozmo/basestation/behaviorChooser.h"
#include "clad/types/behaviorType.h"
#include "json/json.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <cstdint>

namespace Anki {
namespace Cozmo {
  
// forward declarations
class IBehavior;
class BehaviorNone;
class Robot;
namespace ExternalInterface {
  class MessageGameToEngine;
}
template<typename T>class AnkiEvent;
  
class SelectionBehaviorChooser : public SimpleBehaviorChooser
{
public:
  SelectionBehaviorChooser(Robot& robot, const Json::Value& config);
  
  virtual IBehavior* ChooseNextBehavior(const MoodManager& moodManager, double currentTime_sec) const override;
  
protected:
  Robot& _robot;
  Json::Value _config;
  std::vector<Signal::SmartHandle> _eventHandlers;
  IBehavior* _selectedBehavior = nullptr;
  BehaviorNone* _behaviorNone = nullptr;
  
  void HandleExecuteBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  IBehavior* AddNewBehavior(BehaviorType newType);
  
}; // class SelectionBehaviorChooser
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_SelectionBehaviorChooser_H__