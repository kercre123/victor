/**
 * File: reactionTriggerStrategyFace.h
 *
 * Author: Andrew Stein :: Kevin M. Karol
 * Created: 2016-06-16 :: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to seeing a face
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyFace_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyFace_H__

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyPositionUpdate.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iReactToFaceListener.h"
#include "anki/vision/basestation/faceIdTypes.h"
#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyFacePositionUpdated : public ReactionTriggerStrategyPositionUpdate, IReactToFaceListener {
public:
  ReactionTriggerStrategyFacePositionUpdated(BehaviorExternalInterface& behaviorExternalInterface,
                                             IExternalInterface* robotExternalInterface,
                                             const Json::Value& config);

   // Implement IReactToFaceListener
  virtual void FinishedReactingToFace(BehaviorExternalInterface& behaviorExternalInterface,Vision::FaceID_t faceID) override;
  virtual void ClearDesiredTargets() override;

protected:
  virtual void AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void EnabledStateChanged(BehaviorExternalInterface& behaviorExternalInterface, bool enabled) override
                 {_desiredTargets.clear();}

  virtual void BehaviorThatStrategyWillTriggerInternal(ICozmoBehaviorPtr behavior) override;
  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior) override;
  
private:
  void HandleFaceObserved(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedFace& msg);

  // helper to add a face as something we want to react to (or not, if we are disabled). Returns true if it
  // added anything
  bool AddDesiredFace(BehaviorExternalInterface& behaviorExternalInterface, Vision::FaceID_t faceID);
  
  float _lastReactionTime_s = -1.0f;
  
  // everything we want to react to before we stop (to handle multiple faces in the same frame)
  std::set< Vision::FaceID_t > _desiredTargets;
  
  // face IDs (enrolled only) which we have done an initial reaction to
  std::set< Vision::FaceID_t > _hasReactedToFace;
  
  // true if we saw the face close last time, false otherwise (applies hysteresis internally)
  std::map< Vision::FaceID_t, bool > _faceWasClose;
  

  
};


} // namespace Cozmo
} // namespace Anki


#endif //
