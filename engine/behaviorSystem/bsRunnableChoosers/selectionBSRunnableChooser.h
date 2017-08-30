/**
 * File: SelectionBSRunnableChooser.h
 *
 * Author: Lee Crippen
 * Created: 10/15/15
 *
 * Description: Class for allowing specific behaviors to be selected.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_SelectionBSRunnableChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_SelectionBSRunnableChooser_H__

#include "engine/behaviorSystem/bsRunnableChoosers/iBSRunnableChooser.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "json/json.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <cstdint>

namespace Anki {
namespace Cozmo {
  
// forward declarations
class IBehavior;
class Robot;
namespace ExternalInterface {
  class MessageGameToEngine;
}
template<typename T>class AnkiEvent;
  
class SelectionBSRunnableChooser : public IBSRunnableChooser
{
public:
  SelectionBSRunnableChooser(Robot& robot, const Json::Value& config);
  
  virtual IBehaviorPtr GetDesiredActiveBehavior(Robot& robot, const IBehaviorPtr currentRunningBehavior) override;
  
  // events to notify the chooser when it becomes (in)active
  virtual void OnSelected() override;
  virtual void OnDeselected() override;
  
protected:
  Robot& _robot;
  std::vector<Signal::SmartHandle> _eventHandlers;
  IBehaviorPtr _selectedBehavior = nullptr;
  IBehaviorPtr _behaviorWait;
  
  void HandleExecuteBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
    
  // requests enabling processes required by behaviors when they are the selected one
  void SetProcessEnabled(const IBehaviorPtr behavior, bool newValue);
  
private:
  // Number of times to run the selected behavior
  // -1 for infinite
  int _numRuns = -1;
  
  // Whether or not the selected behavior is running
  bool _selectedBehaviorIsRunning = false;
  
}; // class SelectionBSRunnableChooser
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_SelectionBSRunnableChooser_H__
