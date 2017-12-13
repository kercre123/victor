/**
* File: StrategyFrustration.h
*
* Author:  Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy which indicates when Cozmo has become frustrated
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyFrustration_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyFrustration_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

namespace Anki {
namespace Cozmo {

class StrategyFrustration : public IStateConceptStrategy
{
public:
  explicit StrategyFrustration(const Json::Value& config);

protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  void LoadJson(const Json::Value& config);
  
  float _maxConfidentScore = 0.0f;
  float _cooldownTime_s = 0.0f;
  float _lastReactedTime_s = -1.0f;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyFrustration_H__
