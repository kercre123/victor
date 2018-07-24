/**
 * File: BehaviorTakeAPhotoCoordinator.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-05
 *
 * Description: Behavior which handles the behavior flow when the user wants to take a photo
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTakeAPhotoCoordinator__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTakeAPhotoCoordinator__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorTakeAPhotoCoordinator : public ICozmoBehavior
{
public: 
  virtual ~BehaviorTakeAPhotoCoordinator();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorTakeAPhotoCoordinator(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;


private:

  struct InstanceConfig {
    InstanceConfig();
    ICozmoBehaviorPtr frameFacesBehavior;
    ICozmoBehaviorPtr storageIsFullBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToStorageIsFull();
  void TransitionToFrameFaces();
  void TransitionToFocusingAnimation();
  void TransitionToTakePhoto();
  void CaptureCurrentImage();
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTakeAPhotoCoordinator__
