/**
 * File: behaviorReactToGazeDirection.h
 *
 * Author: Robert Cosgriff
 * Created: 2018-09-04
 *
 * Description: React depending on the gaze direction of a face
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToGazeDirection__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToGazeDirection__

#include "coretech/common/engine/robotTimeStamp.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Vector {

class BehaviorReactToGazeDirection : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToGazeDirection() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToGazeDirection(const Json::Value& config);

  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  bool WantsToBeActivatedBehavior() const override;
  void OnBehaviorActivated() override;
  void BehaviorUpdate() override;

  void InitBehavior() override;

private:

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);

    bool searchForFaces;
  };

  struct DynamicVariables {
    DynamicVariables();

    SmartFaceID           faceIDToTurnBackTo;
  };

  // TODO add eye contact "state" or something
  void TransitionToCheckFaceDirection();
  void TransitionToCompleted();
  void FoundNewFace(ActionResult result);
  bool CheckIfShouldStop();

  std::unique_ptr<InstanceConfig> _iConfig;
  // TODO it's unclear if this is should should be a pointer
  // and where it should be intialized
  DynamicVariables _dVars;

  Radians ComputeTurnAngleFromGazePose(const Pose3d& gazePose);
  void TransitionToCheckForFace(const Radians& turnAngle);
  void TransitionToLookAtFace(const SmartFaceID& faceToTurnTowards, const Radians& turnAngle);
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToGazeDirection__
