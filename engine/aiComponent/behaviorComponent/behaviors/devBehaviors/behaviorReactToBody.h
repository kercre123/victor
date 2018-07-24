/**
 * File: behaviorReactToBody.h
 *
 * Author: Lorenzo Riano
 * Created: 2018-05-30
 *
 * Description: Reacts when a body is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTurnTowardsPerson__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTurnTowardsPerson__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/salientPointTypes.h"

namespace Anki {
namespace Cozmo {

class ConditionSalientPointDetected;
class BehaviorSearchWithinBoundingBox;

class BehaviorReactToBody : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToBody() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToBody(const Json::Value& config);

  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  bool WantsToBeActivatedBehavior() const override;
  void OnBehaviorActivated() override;
  void BehaviorUpdate() override;

  void InitBehavior() override;

private:

  void TransitionToStart();
  void TransitionToCheckIfGoStraight();
  void TransitionToGoStraight();
  void TransitionToLookForFace();
  void TransitionToFaceFound();
  void TransitionToCompleted();


  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);

    std::string moveOffChargerBehaviorStr;
    bool shouldDriveStraightWhenBodyDetected;
    ICozmoBehaviorPtr moveOffChargerBehavior;
    float drivingForwardDistance;
    std::shared_ptr<BehaviorSearchWithinBoundingBox> searchBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    void Reset();

    Vision::SalientPoint lastPersonDetected;
    bool searchingForFaces;
    TimeStamp_t imageTimestampWhenActivated;
  };


  InstanceConfig _iConfig;
  DynamicVariables _dVars;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTurnTowardsPerson__
