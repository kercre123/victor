/**
 * File: BehaviorFindCubeAndThen.h
 *
 * Author: Sam Russell
 * Created: 2018-08-20
 *
 * Description: Run BehaviorFindCube. Upon locating a cube, go to the pre-dock pose if found/known, connect to the cube,
 *              (visiblly if this behavior was user driven) then delegate to the followUp behavior, if specified.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCubeAndThen__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCubeAndThen__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Vector {

// FWD Declarations
class BehaviorFindCube;

class BehaviorFindCubeAndThen : public ICozmoBehavior
{

public: 
  virtual ~BehaviorFindCubeAndThen();
  virtual bool WantsToBeActivatedBehavior() const override;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorFindCubeAndThen(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  enum class FindCubeState{
    FindCube,
    DriveToPredockPose,
    AttemptConnection,
    FollowUpBehavior,
    GetOutFailure
  };

  struct InstanceConfig {
    InstanceConfig();
    std::shared_ptr<BehaviorFindCube> findCubeBehavior;
    ICozmoBehaviorPtr                 connectToCubeBehavior;
    std::string                       followUpBehaviorID;
    ICozmoBehaviorPtr                 followUpBehavior;
    bool                              skipConnectToCubeBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    FindCubeState state;
    ObjectID      cubeID;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToFindCube();
  void TransitionToDriveToPredockPose();
  void TransitionToAttemptConnection();
  void TransitionToFollowUpBehavior();
  void TransitionToGetOutFailure();

  // Returns a pointer to the object belonging with ID == _dVars.cubeID, nullptr if none is found
  ObservableObject* GetTargetCube();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCubeAndThen__
