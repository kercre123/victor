/**
 * File: BehaviorLearnBoundary.h
 *
 * Author: Matt Michini
 * Created: 2019-01-11
 *
 * Description: Use the cube to learn boundaries of the "play space"
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLearnBoundary__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLearnBoundary__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/math/point_fwd.h"

#include <vector>

namespace Anki {
class Pose3d;
namespace Vector {

enum class CubeAnimationTrigger;
class ObservableObject;
  
class BehaviorLearnBoundary : public ICozmoBehavior
{
public: 
  virtual ~BehaviorLearnBoundary();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorLearnBoundary(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void InitBehavior() override;
  virtual void BehaviorUpdate() override {}
 
private:

  struct InstanceConfig {
    InstanceConfig() {}
    ICozmoBehaviorPtr findCubeBehavior;
  };

  struct DynamicVariables {
    DynamicVariables() {}
    ObjectID cubeID;
    AxisName firstSeenUpAxis = AxisName::X_POS;
    std::vector<Pose3d> cubePoses;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  // Also updates _dVars.cubeID
  ObservableObject* GetLocatedCube();
  
  void PlayCubeLightAnim(const CubeAnimationTrigger& cubeAnimTrigger);
  
  // Returns true if the cube has been flipped to an orientation different from the starting orientation (e.g., someone
  // flipped it over)
  bool HasCubeBeenFlipped();
  
  // Find the cube (if we don't already know where it is)
  void TransitionToFindCube();
  
  // Face the cube and visually verify it
  void TransitionToFaceAndVerifyCube();
  
  // Ensure that we are an appropriate distance away from the cube
  void TransitionToAdjustPositionWrtCube();
  
  // Record the cube's pose and play the "got it" animation
  void TransitionToRecordCubePose();
  
  // Waiting for the cube to be moved to a new location we can see it again
  void TransitionToWaitForCubeToMove();
  
  // If we have recorded enough poses to create a boundary, then do so. If not, just cancel
  void TransitionToCreateBoundary();
  
  // Now that we have created a boundary, indicate that we know where it is by surveying it
  void TransitionToSurveyBoundary();
  
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLearnBoundary__
