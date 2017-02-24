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
  
class BehaviorRespondPossiblyRoll;
  
struct RespondPossiblyRollMetadata{
public:
  RespondPossiblyRollMetadata(){};
  RespondPossiblyRollMetadata(const ObjectID& objID,
                              s32 uprightAnimIndex,
                              s32  onSideAnimIndex)
  : _objID(objID)
  , _uprightAnimIndex(uprightAnimIndex)
  , _playedUpright(false)
  , _onSideAnimIndex(onSideAnimIndex)
  , _playedOnSide(false)
  , _reachedPreDockRoll(false)
  {
  }
  
  const ObjectID& GetObjectID() const { return _objID;}
  s32 GetUprightAnimIndex()     const { return _uprightAnimIndex;}
  bool GetPlayedUprightAnim()   const { return _playedUpright;}
  s32 GetOnSideAnimIndex()      const { return _onSideAnimIndex;}
  bool GetPlayedOnSideAnim()    const { return _playedOnSide;}
  bool GetReachedPreDocRoll()   const { return _reachedPreDockRoll;}
  
protected:
  friend class BehaviorRespondPossiblyRoll;
  void SetPlayedUprightAnim() { _playedUpright = true;}
  void SetPlayedOnSideAnim() { _playedOnSide = true;}
  void SetReachedPreDockRoll() { _reachedPreDockRoll = true;}

private:
  ObjectID _objID;
  s32 _uprightAnimIndex = 0;
  bool _playedUpright = false;
  s32 _onSideAnimIndex = 0;
  bool _playedOnSide = false;
  bool _reachedPreDockRoll = false;
};
  

class BehaviorRespondPossiblyRoll: public IBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRespondPossiblyRoll(Robot& robot, const Json::Value& config);

public:
  virtual ~BehaviorRespondPossiblyRoll();
  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
  virtual bool IsRunnableInternal(const BehaviorPreReqRespondPossiblyRoll& preReqData) const override;
  
  // Behavior can be queried to find out where it is in its process
  const RespondPossiblyRollMetadata& GetResponseMetadata() const { return _metadata;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  // Override b/c default resume internal uses invalid pre-req data
  virtual Result ResumeInternal(Robot& robot) override;

  
private:
  mutable RespondPossiblyRollMetadata _metadata;
  
  std::map<ObjectID, UpAxis> _upAxisChangedIDs;
  std::vector<Signal::SmartHandle> _eventHalders;
  u32 _lastActionTag = ActionConstants::INVALID_TAG;
  
  void DetermineNextResponse(Robot& robot);
  void TurnAndRespondPositively(Robot& robot);
  void TurnAndRespondNegatively(Robot& robot);
  void DelegateToRollHelper(Robot& robot);
  void RollBlock(Robot& robot);

};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorRespondPossiblyRoll_H__
