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
    
    friend class BehaviorContainer;
    BehaviorFactoryCentroidExtractor(const Json::Value& config);
    
  public:
    
    virtual ~BehaviorFactoryCentroidExtractor() { }
    
    virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
    virtual bool CarryingObjectHandledInternally() const override {return true;}
    
  private:
    
    virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
    virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
    virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
    
    void TransitionToMovingHead(Robot& robot);
    
    virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
    
    bool _waitingForDots = false;
    bool _headCalibrated = false;
    bool _liftCalibrated = false;
    
    FactoryTestLogger _factoryTestLogger;
  };
}
}



#endif // __Cozmo_Basestation_Behaviors_BehaviorFactoryCentroidExtractor_H__
