/**
 * File: behaviorPopAWheelie.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-06-15
 *
 * Description: A behavior which allows cozmo to use a block to pop himself onto his back
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPopAWheelie_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPopAWheelie_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"

#include <string>

namespace Anki {
  namespace Cozmo {
    
    class BlockWorldFilter;
    class ObservableObject;
    class Robot;
    
    class BehaviorPopAWheelie : public IBehavior
    {
    protected:
      // Enforce creation through BehaviorFactory
      friend class BehaviorFactory;
      BehaviorPopAWheelie(Robot& robot, const Json::Value& config);
      
      virtual Result InitInternal(Robot& robot) override;
      virtual void   StopInternal(Robot& robot) override;
      virtual void   StopInternalFromDoubleTap(Robot& robot) override;
      
      virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
      virtual bool CarryingObjectHandledInternally() const override { return false;}
      
      virtual void UpdateTargetBlocksInternal(const Robot& robot) const override { UpdateTargetBlock(robot); }
      
      virtual std::set<ObjectInteractionIntention>
            GetBehaviorObjectInteractionIntentions() const override {
              return {ObjectInteractionIntention::PopAWheelieOnObject};
            }

    private:
      
      // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
      // UpdateWhileNotRunning
      mutable ObjectID _targetBlock;
      
      s32 _numPopAWheelieActionRetries = 0;
      
      bool _hasDisabledcliff = false;
      
      const Robot& _robot;
      
      enum class DebugState {
        ReactingToBlock,
        PerformingAction
      };
            
      void TransitionToReactingToBlock(Robot& robot);
      void TransitionToPerformingAction(Robot& robot);
      void TransitionToPerformingAction(Robot& robot, bool isRetry);
      
      void SetupRetryAction(Robot& robot, const ExternalInterface::RobotCompletedAction& msg);
      
      void ResetBehavior(Robot& robot);
      
      virtual void UpdateTargetBlock(const Robot& robot) const;
    };
    
  }
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPopAWheelie_H__
