/**
* File: ConditionFacePositionUpdated.h
*
* Author:  Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy which monitors large changes in face position
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFacePositionUpdated_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFacePositionUpdated_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

#include "coretech/vision/engine/faceIdTypes.h"

#include <set>

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

namespace ExternalInterface {
struct RobotObservedFace;
}

class ConditionFacePositionUpdated : public IBEICondition, private IBEIConditionEventHandler
{
public:
  ConditionFacePositionUpdated(const Json::Value& config);

  virtual ~ConditionFacePositionUpdated();

protected:
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const override {
    requests.insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low });
  }
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
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

  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;

};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFacePositionUpdated_H__
