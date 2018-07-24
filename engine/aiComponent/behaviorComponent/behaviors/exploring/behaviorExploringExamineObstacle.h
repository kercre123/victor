/**
 * File: BehaviorExploringExamineObstacle.h
 *
 * Author: ross
 * Created: 2018-03-28
 *
 * Description: Behavior to examine extents of obstacle detected by the prox sensor
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploringExamineObstacle__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploringExamineObstacle__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class IActionRunner;

class BehaviorExploringExamineObstacle : public ICozmoBehavior
{
public: 
  virtual ~BehaviorExploringExamineObstacle();
  
  // if set true, this behavior may want to activate when there are obstacles on its side, in which
  // case it will turn towards them
  void SetCanSeeAndTurnToSideObstacles( bool canSee ) { _dVars.persistent.canSeeSideObstacle = canSee; }

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorExploringExamineObstacle(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void TransitionToNextAction();
  
  // true if a poly of the width of the robot to a point dist_mm away is free of obstacles
  bool RobotPathFreeOfObstacle( float dist_mm, bool useRobotWidth ) const;
  
  // true if a prox obstacle is within dist_mm away
  bool RobotSeesObstacleInFront( float dist_mm, bool forActivation ) const;
  
  // true if a new prox obstacle recently popped into view within dist_mm away within angle halfAngle_rad
  // (useful for drive-by's of recently-discovered obstacles). Returns one position
  bool RobotSeesNewObstacleInCone( float dist_mm, float halfAngle_rad, Point2f& position ) const;
  
  void DevTakePhoto() const;
  
  enum class State : uint8_t {
    Initial=0,
    DriveToObstacle,
    FirstTurn,
    ReturnToCenter,
    SecondTurn,
    ReturnToCenterEnd,
    ReferenceHuman,
    Bumping,
  };

  struct InstanceConfig {
    InstanceConfig();
    ICozmoBehaviorPtr bumpBehavior;
    ICozmoBehaviorPtr referenceHumanBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    
    State state;
    
    bool firstTurnDirectionIsLeft;
    float initialPoseAngle_rad;
    float totalObjectAngle_rad; // sum of abs of left and right turns
    std::weak_ptr<IActionRunner> scanCenterAction;
    bool playingScanSound;
    
    struct Persistent {
      bool canSeeSideObstacle;
      bool seesFrontObstacle;
      bool seesSideObstacle;
      Point2f sideObstaclePosition;
    };
    Persistent persistent;
    
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorExploringExamineObstacle__
