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
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
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
      
      virtual bool IsRunnableInternal(const Robot& robot) const override;
      virtual bool CarryingObjectHandledInternally() const override { return false;}

    private:
      
      // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
      // UpdateWhileNotRunning
      mutable ObjectID _targetBlock;
      
      s32 _numPopAWheelieActionRetries = 0;
      
      std::unique_ptr<BlockWorldFilter>  _blockworldFilter;
      
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
      
      // This should return true if the block is valid for this action, false otherwise. Checks that the block is
      // a light cube with known position, not moving, resting flat, and not being carried
      virtual bool FilterBlocks(const ObservableObject* obj) const;
      
      virtual void UpdateTargetBlock(const Robot& robot) const;
    };
    
  }
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPopAWheelie_H__
