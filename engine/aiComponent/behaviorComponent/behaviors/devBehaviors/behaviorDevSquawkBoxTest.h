/**
 * File: BehaviorDevSquawkBoxTest.h
 *
 * Author: Jordan Rivas
 * Created: 2018-09-28
 *
 * Description: Behavior to test scenarios while the robot is in the squawk box.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevSquawkBoxTest__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevSquawkBoxTest__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include <list>


namespace Anki {
namespace Util {
  class IConsoleFunction;
}
  
namespace Vector {

class IAction;
class Robot;

class BehaviorDevSquawkBoxTest : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDevSquawkBoxTest();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDevSquawkBoxTest(const Json::Value& config);  

  virtual void InitBehavior() override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  virtual void HandleWhileActivated(const RobotToEngineEvent& event) override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void BehaviorUpdate() override;

private:
  
  void SetupConsoleFuncs();
  
  // Update Motor State
  void MoveLift();
  void MoveHead();
  void MoveTreads();
  
  
  struct InstanceConfig {
    InstanceConfig();
    // NOTE: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();
    // State
    bool _isLiftMoving        = false;
    bool _isHeadMoving        = false;
    bool _isMovingTreads      = false;
    float _currentTreadSpeed  = 0.0f;
    // Action Tags
    uint32_t _liftActionTag   = 0;
    uint32_t _headActionTag   = 0;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  Robot* _robot = nullptr;
  
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevSquawkBoxTest__
