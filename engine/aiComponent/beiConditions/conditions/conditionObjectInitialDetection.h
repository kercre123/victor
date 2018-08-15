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

#include "engine/aiComponent/beiConditions/conditions/conditionObjectKnown.h"

namespace Anki {
namespace Vector {
  
class ConditionObjectInitialDetection : public ConditionObjectKnown
{
public:
  explicit ConditionObjectInitialDetection(const Json::Value& config);
  
  virtual ~ConditionObjectInitialDetection();
  
protected:
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
private:
  
  // If true, only react to the first time ever seeing this type of object
  bool _firstTimeOnly = false;
  
  mutable bool _reacted = false;
};


} // namespace Vector
} // namespace Anki

#endif // __Engine_BeiConditions_ConditionObjectInitialDetection_H__
