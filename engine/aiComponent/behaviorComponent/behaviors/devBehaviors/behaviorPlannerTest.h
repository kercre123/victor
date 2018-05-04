/**
 * File: behaviorPlannerTest.cpp
 *
 * Author: ross
 * Created: 2018-05-03
 *
 * Description: Upon spotting cube(s), the robot drives to one of them, then to a point far from it, then back again, forever.
 *              Point the cube lights in some direction for the robot to follow that direction. Otherwise,
 *              the planner choses the side of the cube (and the cube, if there are multiple cubes), although
 *              it will stick to that selected pose near the cube for the remainder of the behavior.
 *              To reset, pick up the robot.
 *              The robot will use TTS to convey success/fail for each step
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlannerTest__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlannerTest__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlannerTest : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPlannerTest() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPlannerTest(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  enum class State : uint8_t {
    FirstRun=0,
    WaitingOnCube,
    DrivingToCube, // these last two repeat until the robot is picked up
    DrivingFar
  };
  
  bool FindPoseFromCube();
  void CalcFarAwayPose();
  void GoToCubePose();
  void GoToFarAwayPose();

  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    State state;
    std::vector<Pose3d> cubePoses;
    Pose3d drivePose;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlannerTest__
