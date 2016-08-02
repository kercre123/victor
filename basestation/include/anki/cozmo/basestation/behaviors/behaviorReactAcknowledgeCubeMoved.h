/**
 * File: behaviorReactAcknowledgeCubeMoved.h
 *
 * Author: Kevin M. Karol
 * Created: 7/26/16
 *
 * Description: Behavior to acknowledge when a localized cube has been moved
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class ReactionObjectData{
public:
  ReactionObjectData(int objectID, double nextUniqueTime, bool observedSinceLastResponse);
  int _objectID;
  double _nextUniqueTime;
  bool _observedSinceLastResponse;
};

class BehaviorReactAcknowledgeCubeMoved : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactAcknowledgeCubeMoved(Robot& robot, const Json::Value& config);

  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;

public:
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override { return true;}
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  
protected:
  
  virtual Result InitInternalReactionary(Robot& robot) override;
  
  
private:
  enum class State {
    PlayingSenseReaction,
    TurningToLastLocationOfBlock,
    ReactingToBlockAbsence
  };
    
  State _state;
  std::vector<ReactionObjectData> _reactionObjects;
  uint32_t _activeObjectID;
  
  void TransitionToPlayingSenseReaction(Robot& robot);
  void TransitionToTurningToLastLocationOfBlock(Robot& robot);
  void TransitionToReactingToBlockAbsence(Robot& robot);
  
  void SetState_internal(State state, const std::string& stateName);
  
}; // class BehaviorReactAcknowledgeCubeMoved

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__