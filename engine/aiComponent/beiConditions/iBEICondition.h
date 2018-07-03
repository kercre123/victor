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

#include "engine/components/visionScheduleMediator/iVisionModeSubscriber.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator_fwd.h"

#include "clad/types/behaviorComponent/beiConditionTypes.h"
#include "json/json-forwards.h"

#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class BehaviorExternalInterface;
class IExternalInterface;
class Robot;
class BEIConditionDebugFactors;
  
class IBEICondition : public IVisionModeSubscriber
{
public:
  static const char* const kConditionTypeKey;

  static Json::Value GenerateBaseConditionConfig(BEIConditionType type);  
  static BEIConditionType ExtractConditionType(const Json::Value& config);

  explicit IBEICondition(const Json::Value& config);
  virtual ~IBEICondition();

  // Called once after construction, and before any other functions are called
  void Init(BehaviorExternalInterface& behaviorExternalInterface);

  // Signal to the strategy that it is entering(or exiting) a period of time during which it may be evaluated.
  // Must be called on a strategy before the strategy is evaluated, and should be called with setActive == false
  // before any period of time during which the strategy will CERTAINLY NOT be evaluated. Manages VisionMode 
  // subscription via the VisionScheduleMediator, and calls SetActiveInternal on implementers to allow for 
  // specialized active functionality.
  void SetActive(BehaviorExternalInterface& behaviorExternalInterface, bool setActive);

  bool AreConditionsMet(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  BEIConditionType GetConditionType() const {return _conditionType;}
  
  void SetDebugLabel(const std::string& label) { _debugLabel = label; }
  const std::string& GetDebugLabel() const { return _debugLabel; }
  
  // If a BEICondition has VisionMode Requirements, override this function to specify them. Modes set here
  // will be automatically managed by the SetActive infrastructure.
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const {};
  
  void SetOwnerDebugLabel(const std::string& ownerLabel) { _ownerLabel = ownerLabel; }
  const std::string& GetOwnerDebugLabel() const { return _ownerLabel; }
  
  const BEIConditionDebugFactors& GetDebugFactors() const;
  BEIConditionDebugFactors& GetDebugFactors();
  
  void BuildDebugFactors( BEIConditionDebugFactors& factors ) const;
  
protected:

  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) {}
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const = 0;

  // Derived classes which have functionality that should only be carried out during an active part of their 
  // lifecycle should override this function.
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool isActive) {}
  
  // Report here whatever factors influence the AreConditionsMetInternal by calling AddFactor and
  // AddChild on factors. You don't need to add a factor for the value of AreConditionsMetInternal
  // since that is already handled for you.
  // Important: Always call AddFactor on variables in the same order! Otherwise the factors will
  // be marked as if they have changed.
  virtual void BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const {}
  
private:
  
  // called when whenever AreConditionsMet is evaluated
  void SendConditionsToWebViz( BehaviorExternalInterface& bei ) const;
  
  // called when this condition becomes inactive
  void SendInactiveToWebViz( BehaviorExternalInterface& bei ) const;
  
  // call only once
  std::string MakeUniqueDebugLabel() const;
  
  BEIConditionType _conditionType;
  bool _isActive = false;
  bool _isInitialized = false;
  
  std::string _debugLabel;
  std::string _ownerLabel;
  
  mutable bool _lastMetValue;
  mutable std::unique_ptr<BEIConditionDebugFactors> _debugFactors;
  static bool _checkDebugFactors;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IBEICondition_H__
