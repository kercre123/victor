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
#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToFaceNormal__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToFaceNormal__

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

  // TODO add eye contact "state" or something
  void TransitionToCheckFaceDirection();
  void TransitionToCompleted();
  void FoundNewFace(ActionResult result);
  bool CheckIfShouldStop();

  SmartFaceID _faceIDToTurnBackTo;

  enum class State : uint8_t {

    // TODO add state for eye contact
    NotStarted,
    FaceNormalDirectedAtRobot,
    FaceNormalDirectedAtSurfaceLeft,
    FaceNormalDirectedAtSurfaceRight,
    
  };

  struct InstanceConfig
  {
    f32                   coolDown_sec;
  };
  struct DynamicVariables
  {
    RobotTimeStamp_t      lastReactionTime_ms;
  };

  std::unique_ptr<InstanceConfig>   _iConfig;
  std::unique_ptr<DynamicVariables> _dVars;

  // TODO shoudl this live some place else?
  State state = State::NotStarted;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToFaceNormal__
