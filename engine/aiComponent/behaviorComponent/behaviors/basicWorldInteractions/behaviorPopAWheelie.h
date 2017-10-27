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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"

#include <string>

namespace Anki {
  namespace Cozmo {
    
    class BlockWorldFilter;
    class ObservableObject;
    class Robot;
    
    class BehaviorPopAWheelie : public ICozmoBehavior
    {
    protected:
      // Enforce creation through BehaviorContainer
      friend class BehaviorContainer;
      BehaviorPopAWheelie(const Json::Value& config);
      
      virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
      virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
      
      virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
      virtual bool CarryingObjectHandledInternally() const override { return false;}
      
      virtual void UpdateTargetBlocksInternal(BehaviorExternalInterface& behaviorExternalInterface) const override { UpdateTargetBlock(behaviorExternalInterface); }
      
      virtual std::set<ObjectInteractionIntention>
            GetBehaviorObjectInteractionIntentions() const override {
              return {ObjectInteractionIntention::PopAWheelieOnObject};
            }

    private:
      
      // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
      // UpdateWhileNotRunning
      mutable ObjectID _targetBlock;
      ObjectID _lastBlockReactedTo;
      
      s32 _numPopAWheelieActionRetries = 0;
      
      bool _hasDisabledcliff = false;
            
      enum class DebugState {
        ReactingToBlock,
        PerformingAction
      };
            
      void TransitionToReactingToBlock(BehaviorExternalInterface& behaviorExternalInterface);
      void TransitionToPerformingAction(BehaviorExternalInterface& behaviorExternalInterface);
      void TransitionToPerformingAction(BehaviorExternalInterface& behaviorExternalInterface, bool isRetry);
      
      void SetupRetryAction(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotCompletedAction& msg);
      
      void ResetBehavior(BehaviorExternalInterface& behaviorExternalInterface);
      
      virtual void UpdateTargetBlock(BehaviorExternalInterface& behaviorExternalInterface) const;
    };
    
  }
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPopAWheelie_H__
