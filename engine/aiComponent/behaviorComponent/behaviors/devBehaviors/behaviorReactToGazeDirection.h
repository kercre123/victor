/**
 * File: behaviorReactToGazeDirection.h
 *
 * Author: Robert Cosgriff
 * Created: 11/29/2018
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

    Pose3d                gazeDirectionPose;
    SmartFaceID           faceIDToTurnBackTo;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToCheckGazeDirection();
  void TransitionToCheckForFace(const Radians& turnAngle);
  void TransitionToLookAtFace(const SmartFaceID& faceToTurnTowards, const Radians& turnAngle);
  void TransitionToCheckForPointOnSurface(const Pose3d& gazePose);

  Radians ComputeTurnAngleFromGazePose(const Pose3d& gazePose);
  void FoundNewFace(ActionResult result);

  void SendDASEventForPoseToFollow(const Pose3d& gazePose) const;

  u32 GetMaxTimeSinceTrackedFaceUpdated_ms() const;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToGazeDirection__
