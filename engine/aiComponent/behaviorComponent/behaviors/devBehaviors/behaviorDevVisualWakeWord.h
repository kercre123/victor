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

// Forward Declarations
class BehaviorPromptUserForVoiceCommand;

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
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  bool WantsToBeActivatedBehavior() const override;
  void OnBehaviorActivated() override;
  void BehaviorUpdate() override;

  void InitBehavior() override;

private:

  enum class EState {
    CheckingForVisualWakeWord,
    DetectedVisualWakeWord,
    Listening,
    Responding
  };

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);

    std::shared_ptr<BehaviorPromptUserForVoiceCommand> yeaOrNayBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();

    EState                state;
    Pose3d                gazeDirectionPose;
    SmartFaceID           faceIDToTurnBackTo;
  };

  void TransitionToCheckForVisualWakeWord();
  void TransitionToListening();
  void TransitionToResponding(const int response);

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevVisualWakeWord____
