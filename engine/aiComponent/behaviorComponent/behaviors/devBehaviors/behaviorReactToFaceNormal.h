/**
 * File: behaviorReactToFaceNormal.h
 *
 * Author: Robert Cosgriff
 * Created: 2018-09-04
 *
 * Description: React depending on the face-normal direction
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToFaceNormal__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToFaceNormal__

#include "coretech/common/engine/robotTimeStamp.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToFaceNormal : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToFaceNormal() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToFaceNormal(const Json::Value& config);

  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  bool WantsToBeActivatedBehavior() const override;
  void OnBehaviorActivated() override;
  void BehaviorUpdate() override;

  void InitBehavior() override;

private:

  // TODO add eye contact "state" or something
  void TransitionToCheckFaceNormalDirectedAtRobot();
  void TransitionToCheckFaceNormalDirectedLeftOrRight();
  void TransitionToCompleted();
  bool CheckIfShouldStop();

  enum class State : uint8_t {

    // TODO add state for eye contact
    NotStarted,
    CheckingFaceNormalDirectedAtRobot,
    CheckingFaceNormalDirectedLeft,
    CheckingFaceNormalDirectedRight,
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
