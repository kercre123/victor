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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/factory/factoryTestLogger.h"


namespace Anki {
namespace Cozmo {
  
  class BehaviorFactoryCentroidExtractor : public IBehavior
  {
  protected:
    
    friend class BehaviorFactory;
    BehaviorFactoryCentroidExtractor(Robot& robot, const Json::Value& config);
    
  public:
    
    virtual ~BehaviorFactoryCentroidExtractor() { }
    
    virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
    virtual bool CarryingObjectHandledInternally() const override {return true;}
    
  private:
    
    virtual Result InitInternal(Robot& robot) override;
    virtual Status UpdateInternal(Robot& robot) override;
    virtual void StopInternal(Robot& robot) override;
    
    void TransitionToMovingHead(Robot& robot);
    
    virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
    
    bool _waitingForDots = false;
    bool _headCalibrated = false;
    bool _liftCalibrated = false;
    
    FactoryTestLogger _factoryTestLogger;
  };
}
}



#endif // __Cozmo_Basestation_Behaviors_BehaviorFactoryCentroidExtractor_H__
