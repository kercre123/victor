/**
 * File: behaviorChooser.h
 *
 * Author: Lee
 * Created: 08/20/15
 *
 * Description: Class for handling picking of behaviors.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorChooser_H__

#include "anki/cozmo/shared/cozmoTypes.h"
#include "util/helpers/noncopyable.h"
#include <string>
#include <vector>


namespace Anki {
namespace Cozmo {
  
//Forward declarations
class IBehavior;

class IBehaviorChooser : private Util::noncopyable
{
public:
  virtual Result AddBehavior(IBehavior *newBehavior) = 0;
  virtual IBehavior* ChooseNextBehavior(float currentTime_sec) const = 0;
  virtual IBehavior* GetBehaviorByName(const std::string& name) const = 0;
  
  virtual ~IBehaviorChooser() { }
}; // class IBehaviorChooser
  
  
// A simple implementation for choosing behaviors based on priority only
// Behaviors are checked for runnability in the order they were added
class SimpleBehaviorChooser : public IBehaviorChooser
{
public:
  // For IBehaviorChooser
  virtual Result AddBehavior(IBehavior *newBehavior) override;
  virtual IBehavior* ChooseNextBehavior(float currentTime_sec) const override;
  virtual IBehavior* GetBehaviorByName(const std::string& name) const override;
  
  // We need to clean up the behaviors we've been given to hold onto
  virtual ~SimpleBehaviorChooser();
  
protected:
  std::vector<IBehavior*> _behaviorList;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorChooser_H__