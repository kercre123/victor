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

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/simpleBehaviorChooser.h"
#include "clad/types/behaviorTypes.h"
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
  
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  virtual const char* GetName() const override { return "Selection"; }
  
  // events to notify the chooser when it becomes (in)active
  virtual void OnSelected() override;
  virtual void OnDeselected() override;
  
protected:
  Robot& _robot;
  std::vector<Signal::SmartHandle> _eventHandlers;
  IBehavior* _selectedBehavior = nullptr;
  IBehavior* _behaviorNone = nullptr;
  
  void HandleExecuteBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  IBehavior* AddNewBehavior(BehaviorClass newType);
  
  // requests enabling processes required by behaviors when they are the selected one
  void SetProcessEnabled(const IBehavior* behavior, bool newValue);
  
}; // class SelectionBehaviorChooser
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_SelectionBehaviorChooser_H__
