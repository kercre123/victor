/**
 * File: ConditionObjectInitialDetection.h
 *
 * Author: Matt Michini
 * Created: 1/10/2018
 *
 * Description: Strategy for responding to an object being seen
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_BeiConditions_ConditionObjectInitialDetection_H__
#define __Engine_BeiConditions_ConditionObjectInitialDetection_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

#include "clad/types/objectTypes.h"

namespace Anki {
namespace Cozmo {

class BEIConditionMessageHelper;
  
class ConditionObjectInitialDetection : public IBEICondition, private IBEIConditionEventHandler
{
public:
  explicit ConditionObjectInitialDetection(const Json::Value& config);
  
  virtual ~ConditionObjectInitialDetection();
  
protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override {
    requiredVisionModes.insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low });
  }
private:
  
  // If true, only react to the first time ever seeing this type of object
  bool _firstTimeOnly = false;
  
  // The object type we care about (TODO: make this a vector of multiple object types)
  ObjectType _targetType = ObjectType::InvalidObject;
  
  // Last time we observed the target object type
  size_t _tickOfLastObservation = 0;
  
  mutable bool _reacted = false;
  
  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Engine_BeiConditions_ConditionObjectInitialDetection_H__
