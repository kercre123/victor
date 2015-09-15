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
#include "util/signals/simpleSignal_fwd.h"


namespace Anki {
namespace Cozmo {
  
// forward declarations
class IBehavior;
class BehaviorLookAround;
class BehaviorInteractWithFaces;
class BehaviorOCD;
class BehaviorFidget;
class Robot;
  
class DemoBehaviorChooser : public ReactionaryBehaviorChooser
{
public:
  DemoBehaviorChooser(Robot& robot, const Json::Value& config);
  
  virtual IBehavior* ChooseNextBehavior(double currentTime_sec) const override;
  virtual Result Update(double currentTime_sec) override;
  virtual Result AddBehavior(IBehavior* newBehavior) override;
  
protected:
  enum class DemoState {
    Faces,
    Blocks,
    Rest
  };
  
  // Amount of time in seconds to stay in the Blocks DemoState while waiting for some blocks that need fixing
  constexpr static f32 kBlocksBoredomTime = 60;
  
  Robot& _robot;
  
  DemoState _demoState = DemoState::Faces;
  
  // Note these are for easy access - the inherited _behaviorList owns the memory
  BehaviorLookAround* _behaviorLookAround = nullptr;
  BehaviorInteractWithFaces* _behaviorInteractWithFaces = nullptr;
  BehaviorOCD* _behaviorOCD = nullptr;
  BehaviorFidget* _behaviorFidget = nullptr;
  
  void SetupBehaviors(Robot& robot, const Json::Value& config);
  
private:
  using super = ReactionaryBehaviorChooser;
  
}; // class DemoBehaviorChooser
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_DemoBehaviorChooser_H__