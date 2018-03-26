/**
 * File: behaviorFactoryCentroidExtractor.h
 *
 * Author: Al Chaussee
 * Date:   06/20/2016
 *
 * Description: Logs the four centroid positions and calculated camera pose
 *              using the pattern on the standalone factory test fixture
 *
 *              Failures are indicated by backpack lights
 *                Red: Failure to find centroids
 *                Magenta: Failure to move head to zero degrees
 *                Orange: Failure to compute camera pose
 *                Blue: One or more of the camPose thresholds was exceeded
 *                Green: Passed
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFactoryCentroidExtractor_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFactoryCentroidExtractor_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/visionComponent.h"
#include "engine/factory/factoryTestLogger.h"


namespace Anki {
namespace Cozmo {
  
class BehaviorFactoryCentroidExtractor : public ICozmoBehavior
{
protected:
  
  friend class BehaviorFactory;
  BehaviorFactoryCentroidExtractor(const Json::Value& config);
  
public:
  
  virtual ~BehaviorFactoryCentroidExtractor() { }
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

private:
  struct InstanceConfig {
    InstanceConfig();
    FactoryTestLogger factoryTestLogger;

  };

  struct DynamicVariables {
    DynamicVariables();
    bool waitingForDots;
    bool headCalibrated;
    bool liftCalibrated;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual void OnBehaviorDeactivated() override;
  
  void TransitionToMovingHead(Robot& robot);
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  
};

} // namespace Cozmo
} // namespace Anki



#endif // __Cozmo_Basestation_Behaviors_BehaviorFactoryCentroidExtractor_H__
