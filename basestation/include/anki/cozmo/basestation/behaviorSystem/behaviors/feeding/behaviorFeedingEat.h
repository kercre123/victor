/**
* File: behaviorFeedingEat.h
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to interact with an "energy" filled cube
* and drain the energy out of it
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingEat_H__
#define __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingEat_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/feedingCubeController.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"


#include "anki/common/basestation/objectIDs.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorFeedingEat : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorFeedingEat(Robot& robot, const Json::Value& config);

public:
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeObject& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  virtual void AddListener(IFeedingListener* listener) override{
    _feedingListeners.push_back(listener);
  };
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
private:
  mutable ObjectID _targetID;
  std::vector<IFeedingListener*> _feedingListeners;
  bool _shouldSendUpdateUI;
  
  void TransitionToReactingToFood(Robot& robot);
  void TransitionToDrivingToFood(Robot& robot);
  void TransitionToEating(Robot& robot);
  
  AnimationTrigger UpdateNeedsStateAndCalculateAnimation(Robot& robot);

  void SendDelayedUIUpdateIfAppropriate(Robot& robot);

};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingEat_H__
