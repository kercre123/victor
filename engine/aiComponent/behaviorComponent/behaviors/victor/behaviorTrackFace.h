/**
 * File: BehaviorTrackFace.h
 *
 * Author: Brad
 * Created: 2018-05-21
 *
 * Description: Test behavior for tracking faces
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTrackFace__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTrackFace__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorTrackFace : public ICozmoBehavior
{
public: 
  virtual ~BehaviorTrackFace();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorTrackFace(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig;
  std::unique_ptr<InstanceConfig> _iConfig;

  struct DynamicVariables;
  std::unique_ptr<DynamicVariables> _dVars;

  void BeginTracking();

  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTrackFace__
