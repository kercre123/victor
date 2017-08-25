/**
* File: behaviorSearchForFace.h
*
* Author: Kevin M. Karol
* Created: 7/21/17
*
* Description: A behavior which uses an animation in order to search around for a face
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSearchForFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSearchForFace_H__

#include "engine/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorSearchForFace : public IBehavior
{
protected:
  using base = IBehavior;
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorSearchForFace(Robot& robot, const Json::Value& config);
  
public:
  virtual ~BehaviorSearchForFace() override {}
  virtual bool IsRunnableInternal(const Robot& robot) const override;

  virtual bool CarryingObjectHandledInternally() const override{ return false;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  
private:
  enum class State {
    SearchingForFace,
    FoundFace
  };
  
  State    _behaviorState;
  
  void TransitionToSearchingAnimation(Robot& robot);
  void TransitionToFoundFace(Robot& robot);
  
  void SetState_internal(State state, const std::string& stateName);

};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorSearchForFace_H__
