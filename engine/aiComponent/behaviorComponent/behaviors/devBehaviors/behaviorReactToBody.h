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

#include "clad/types/salientPointTypes.h"
#include "coretech/common/engine/robotTimeStamp.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/faceSelectionComponent.h"

namespace Anki {
namespace Vector {

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
  void TransitionToMaybeGoStraightAndLookUp();
  void TransitionToLookForFace();
  void TransitionToFaceFound();
  void TransitionToCompleted();

  bool CheckIfShouldStop(); // It won't run if e.g. it's picked up, or there's a cliff, or an obstacle..


  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);

    std::string moveOffChargerBehaviorStr;
    bool shouldDriveStraightWhenBodyDetected;
    ICozmoBehaviorPtr moveOffChargerBehavior;
    float drivingForwardDistance;
    std::shared_ptr<BehaviorSearchWithinBoundingBox> searchBehavior;
    float upperPortionLookUpPercent;
    float trackingTimeout;
    FaceSelectionComponent::FaceSelectionFactorMap faceSelectionCriteria;
  };

  struct DynamicVariables {
    struct Persistent {
      RobotTimeStamp_t lastSeenTimeStamp;
    } persistent;

    DynamicVariables();
    void Reset(bool keepPersistent);

    Vision::SalientPoint lastPersonDetected;
    bool searchingForFaces;
    bool droveOffCharger;
    bool actingOnFaceFound; // currently tracking, could be something else
    RobotTimeStamp_t imageTimestampWhenActivated;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTurnTowardsPerson__
