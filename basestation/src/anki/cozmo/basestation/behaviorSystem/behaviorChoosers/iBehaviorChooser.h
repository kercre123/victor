/**
 * File: iBehaviorChooser.h
 *
 * Author: Lee
 * Created: 08/20/15, raul 05/03/16
 *
 * Description: Class for handling picking of behaviors.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorChooser_H__

#include "anki/types.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupFlags.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/helpers/noncopyable.h"
#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"
#include <map>
#include <string>
#include <set>
#include <vector>
#include <functional>


namespace Json {
  class Value;
}

namespace Anki {
namespace Cozmo {
  
//Forward declarations
class IBehavior;
class Robot;

// Interface for the container and logic associated with holding and choosing behaviors
class IBehaviorChooser : private Util::noncopyable
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // constructor/destructor
  IBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~IBehaviorChooser() {}
  
  // events to notify the chooser when it becomes (in)active
  virtual void OnSelected() {};
  virtual void OnDeselected() {};

  // read which groups/behaviors are enabled/disabled from json configuration
  virtual void ReadEnabledBehaviorsConfiguration(const Json::Value& inJson) = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Logic
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) = 0;

  // update internal state of the chooser
  virtual Result Update() { return Result::RESULT_OK; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // name (for debug/identification)
  virtual const char* GetName() const = 0;
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:
  virtual std::vector<std::string> GetEnabledBehaviorList() = 0;
  Util::RandomGenerator& GetRNG() const;
  Robot& _robot;

private:
  std::vector<Signal::SmartHandle> _signalHandles;

  

  
}; // class IBehaviorChooser
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorChooser_H__
