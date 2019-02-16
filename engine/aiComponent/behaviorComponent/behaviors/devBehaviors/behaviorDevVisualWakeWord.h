/**
 * File: behaviorDevVisualWakeWord.h
 *
 * Author: Robert Cosgriff
 * Created: 2/15/2019
 *
 * Description: Prototype behavior that responds to positive and negative
 *              voice intents, when the user's gaze is directed at the
 *              robot.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/
#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevVisualWakeWord__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevVisualWakeWord__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Vector {

class BehaviorDevVisualWakeWord : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDevVisualWakeWord() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDevVisualWakeWord(const Json::Value& config);

  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  bool WantsToBeActivatedBehavior() const override;
  void OnBehaviorActivated() override;
  void BehaviorUpdate() override;

  void InitBehavior() override;

private:

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
  };

  struct DynamicVariables {
    DynamicVariables();

    Pose3d                gazeDirectionPose;
    SmartFaceID           faceIDToTurnBackTo;
  };

  void TransitionToCheckForVisualWakeWord();

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevVisualWakeWord____
