/**
* File: StrategyFacePositionUpdated.h
*
* Author:  Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy which monitors large changes in face position
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyFacePositionUpdated_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyFacePositionUpdated_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategyEventHandler.h"

#include "anki/vision/basestation/faceIdTypes.h"

#include <set>

namespace Anki {
namespace Cozmo {

class StateConceptStrategyMessageHelper;

class StrategyFacePositionUpdated : public IStateConceptStrategy, private IStateConceptStrategyEventHandler
{
public:
  StrategyFacePositionUpdated(BehaviorExternalInterface& behaviorExternalInterface,
                              IExternalInterface& robotExternalInterface,
                              const Json::Value& config);

  virtual ~StrategyFacePositionUpdated();

protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void HandleEvent(const EngineToGameEvent& event,
                           BehaviorExternalInterface& behaviorExternalInterface) override;
  

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

  std::unique_ptr<StateConceptStrategyMessageHelper> _messageHelper;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyFacePositionUpdated_H__
