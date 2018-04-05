/**
* File: ConditionPetInitialDetection.h
*
* Author: Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy for responding to a pet being detected
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPetInitialDetection_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPetInitialDetection_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "coretech/vision/engine/faceIdTypes.h"

#include <set>

namespace Anki {
namespace Cozmo {

class ConditionPetInitialDetection : public IBEICondition
{
public:
  explicit ConditionPetInitialDetection(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override {
    requiredVisionModes.insert({ VisionMode::DetectingPets, EVisionUpdateFrequency::Low });
  }
private:  
  // Everything we have already reacted to
  mutable std::set<Vision::FaceID_t> _reactedTo;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPetInitialDetection_H__
