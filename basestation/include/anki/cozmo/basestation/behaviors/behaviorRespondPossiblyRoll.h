/**
 * File: behaviorRespondPossiblyRoll.h
 *
 * Author: Kevin M. Karol
 * Created: 01/23/17
 *
 * Description: Behavior that turns towards a block, plays an animation
 * and then rolls it if the block is on its side
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorRespondPossiblyRoll_H__
#define __Cozmo_Basestation_Behaviors_BehaviorRespondPossiblyRoll_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/common/basestation/objectIDs.h"

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BehaviorRespondPossiblyRoll: public IBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRespondPossiblyRoll(Robot& robot, const Json::Value& config);

public:
  virtual ~BehaviorRespondPossiblyRoll();
  virtual bool CarryingObjectHandledInternally() const override { return false;}

  
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  void SetReactionAnimation(AnimationTrigger trigger){ _reactionAnimation = trigger;}

  void SetTarget(const ObjectID& objID){ _targetID = objID; }
  const ObjectID& GetTargetID(){ return _targetID;}
  void ClearTarget(){_targetID.UnSet();}
  
  
  
  // Behavior can be queried to find out whether it responded successfully
  // sets the trigger responded with if the response was successful
  bool WasResponseSuccessful(AnimationTrigger& response,
                             float& completedTimestamp_s,
                             bool& hadToRoll) const;
  
  virtual void   StopInternal(Robot& robot) override;

  
protected:
  virtual Result InitInternal(Robot& robot) override;
  // We shouldn't play the animation a second time if it's interrupted so simply return RESULT_OK
  virtual Result ResumeInternal(Robot& robot) override;
  
private:
  AnimationTrigger _reactionAnimation;
  ObjectID _targetID;
  bool _responseSuccessfull;
  bool _attemptingRoll;
  float _completedTimestamp_s;

  
  void TurnAndReact(Robot& robot);
  void RollIfNecessary(Robot& robot);


};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorRespondPossiblyRoll_H__
