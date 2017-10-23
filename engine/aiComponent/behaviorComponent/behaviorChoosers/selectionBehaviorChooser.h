/**
 * File: SelectionBehaviorChooser.h
 *
 * Author: Lee Crippen
 * Created: 10/15/15
 *
 * Description: Class for allowing specific behaviors to be selected.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_SelectionBehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_SelectionBehaviorChooser_H__

#include "engine/aiComponent/behaviorComponent/behaviorChoosers/iBehaviorChooser.h"
#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "json/json.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <cstdint>

namespace Anki {
namespace Cozmo {
  
// forward declarations
class ICozmoBehavior;
class Robot;
namespace ExternalInterface {
  class MessageGameToEngine;
}
template<typename T>class AnkiEvent;
  
class SelectionBehaviorChooser : public IBehaviorChooser
{
public:
  SelectionBehaviorChooser(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  
  virtual ICozmoBehaviorPtr GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior) override;
  
  // events to notify the chooser when it becomes (in)active
  virtual void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  
protected:
  BehaviorExternalInterface& _behaviorExternalInterface;
  ICozmoBehaviorPtr _selectedBehavior = nullptr;
  ICozmoBehaviorPtr _behaviorWait;
  
  void HandleExecuteBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
    
  // requests enabling processes required by behaviors when they are the selected one
  void SetProcessEnabled(const ICozmoBehaviorPtr behavior, bool newValue);
  
  virtual void UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;

  
private:
  // Number of times to run the selected behavior
  // -1 for infinite
  int _numRuns = -1;
  
  // Whether or not the selected behavior is running
  bool _selectedBehaviorIsRunning = false;
  
}; // class SelectionBehaviorChooser
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_SelectionBehaviorChooser_H__
