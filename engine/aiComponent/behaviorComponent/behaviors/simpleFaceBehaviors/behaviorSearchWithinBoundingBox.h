/**
 * File: BehaviorSearchWithinBoundingBox.h
 *
 * Author: Lorenzo Riano
 * Created: 2018-07-06
 *
 * Description: Behavior to move the head/body whitin a confined space. The limits of the space are
 *              defined by a bounding box in normalized image coordinates.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSearchWithinBoundingBox__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSearchWithinBoundingBox__
#pragma once

#include "coretech/common/engine/math/point.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorSearchWithinBoundingBox : public ICozmoBehavior
{
public: 
  ~BehaviorSearchWithinBoundingBox() override = default;

  // Sets the bounding box for the search. All values are between 0 and 1
  void SetNewBoundingBox(float minX, float maxX, float minY, float maxY);
  void InitBehavior() override;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorSearchWithinBoundingBox(const Json::Value& config);

  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  bool WantsToBeActivatedBehavior() const override;
  void OnBehaviorActivated() override;

private:

  void TransitionToGenerateNewPose();
  void TransitionToMoveToLookAt();
  void TransitionToMoveToOriginalPose();
  void TransitionToWait();

  struct InstanceConfig {
    int8_t howManySearches;
    float waitBeforeTurning;

    // margins for random search, in [0..1]
    float minX = 0;
    float maxX = 1;
    float minY = 0;
    float maxY = 1;
  };

  struct DynamicVariables {
    DynamicVariables();

    // where to look at next
    float nextX;
    float nextY;

    int8_t numberOfRemainingSearches = 0; // how many pan/tilt motions to perform

    Radians initialTheta; // original robot orientation before last move
    float initialTilt; // original head tilt before last move
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSearchWithinBoundingBox__
