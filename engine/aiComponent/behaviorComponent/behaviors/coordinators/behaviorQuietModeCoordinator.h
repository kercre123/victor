/**
 * File: behaviorQuietModeCoordinator.h
 *
 * Author: ross
 * Created: 2018-05-14
 *
 * Description: Coordinator for quiet modes. No volume, no stimulation.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorQuietModeCoordinator__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorQuietModeCoordinator__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorQuietModeCoordinator : public ICozmoBehavior
{
public: 
  virtual ~BehaviorQuietModeCoordinator() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorQuietModeCoordinator(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void SimmerDownNow();
  void ResumeNormal();
  
  void SetAudioActive( bool active );
  
  struct BehaviorInfo {
    BehaviorID behaviorID;
    ICozmoBehaviorPtr behavior;
    bool allowsAudio;
  };
  
  struct InstanceConfig {
    InstanceConfig();
    
    float activeTime_s;
    std::vector<BehaviorInfo> behaviors;
    ICozmoBehaviorPtr wakeWordBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    
    bool audioActive;
    bool wasFixed;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorQuietModeCoordinator__
