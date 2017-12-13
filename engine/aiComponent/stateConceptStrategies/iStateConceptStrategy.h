/**
* File: iStateConceptStrategy.h
*
* Author: Kevin M. Karol
* Created: 7/3/17
*
* Description: Interface for generic strategies which can be used across 
* all parts of the behavior system to determine when a 
* behavior/reaction/activity wants to run
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IStateConceptStrategy_H__
#define __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IStateConceptStrategy_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy_fwd.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include "clad/types/behaviorComponent/strategyTypes.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class BehaviorExternalInterface;
class IExternalInterface;
class Robot;
  
class IStateConceptStrategy{
public:
  static Json::Value GenerateBaseStrategyConfig(StateConceptStrategyType type);  
  static StateConceptStrategyType ExtractStrategyType(const Json::Value& config);

  explicit IStateConceptStrategy(const Json::Value& config);
  virtual ~IStateConceptStrategy() {};

  // Called once after construction, and before any other functions are called
  void Init(BehaviorExternalInterface& behaviorExternalInterface);

  // reset this strategy. For many strategies this call won't do anything, but if the strategy is internally
  // tracking something (e.g. time, number of events), this call will reset the state. This must be called at
  // least once before AreStateConditionsMet is called
  void Reset(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool AreStateConditionsMet(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  StateConceptStrategyType GetStrategyType(){return _strategyType;}
  
protected:

  // ResetInternal is called whenever Reset is called, which depends on how the strategy is being used
  virtual void ResetInternal(BehaviorExternalInterface& behaviorExternalInterface) {}
  
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) {}
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const = 0;

private:
  StateConceptStrategyType _strategyType;
  bool _hasEverBeenReset = false;
  bool _isInitialized = false;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IStateConceptStrategy_H__
