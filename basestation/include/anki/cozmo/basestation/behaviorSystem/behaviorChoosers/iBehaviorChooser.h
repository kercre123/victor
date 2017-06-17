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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior_fwd.h"
#include "anki/common/types.h"
#include "util/helpers/noncopyable.h"
#include <vector>


namespace Json {
  class Value;
}

namespace Anki {
namespace Cozmo {
  
//Forward declarations
class Robot;

// Interface for the container and logic associated with holding and choosing behaviors
class IBehaviorChooser : private Util::noncopyable
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // constructor/destructor
  IBehaviorChooser(Robot& robot, const Json::Value& config){};
  virtual ~IBehaviorChooser() {}
  
  // events to notify the chooser when it becomes (in)active
  virtual void OnSelected() {};
  virtual void OnDeselected() {};

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Logic
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehaviorPtr ChooseNextBehavior(Robot& robot, const IBehaviorPtr currentRunningBehavior) = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Used to access objectTapInteraction behaviors
  std::vector<IBehaviorPtr> GetObjectTapBehaviors(){ return {};}
  
}; // class IBehaviorChooser
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorChooser_H__
