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

#ifndef __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IBEICondition_H__
#define __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IBEICondition_H__

#include "engine/aiComponent/beiConditions/iBEICondition_fwd.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include "clad/types/behaviorComponent/beiConditionTypes.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class BehaviorExternalInterface;
class IExternalInterface;
class Robot;
  
class IBEICondition{
public:
  static Json::Value GenerateBaseConditionConfig(BEIConditionType type);  
  static BEIConditionType ExtractConditionType(const Json::Value& config);

  explicit IBEICondition(const Json::Value& config);
  virtual ~IBEICondition() {};

  // Called once after construction, and before any other functions are called
  void Init(BehaviorExternalInterface& behaviorExternalInterface);

  // reset this strategy. For many strategies this call won't do anything, but if the strategy is internally
  // tracking something (e.g. time, number of events), this call will reset the state. This must be called at
  // least once before AreConditionsMet is called
  void Reset(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool AreConditionsMet(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  BEIConditionType GetConditionType(){return _conditionType;}
  
protected:

  // ResetInternal is called whenever Reset is called, which depends on how the strategy is being used
  virtual void ResetInternal(BehaviorExternalInterface& behaviorExternalInterface) {}
  
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) {}
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const = 0;

private:
  BEIConditionType _conditionType;
  bool _hasEverBeenReset = false;
  bool _isInitialized = false;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IBEICondition_H__
