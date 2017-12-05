/**
* File: StrategyPetInitialDetection.h
*
* Author: Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy for responding to a pet being detected
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyPetInitialDetection_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyPetInitialDetection_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

#include "anki/vision/basestation/faceIdTypes.h"

namespace Anki {
namespace Cozmo {

class StrategyPetInitialDetection : public IStateConceptStrategy
{
public:
  StrategyPetInitialDetection(BehaviorExternalInterface& behaviorExternalInterface,
                      IExternalInterface* robotExternalInterface,
                      const Json::Value& config);

protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:  
  // Everything we have already reacted to
  mutable std::set<Vision::FaceID_t> _reactedTo;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyPetInitialDetection_H__
