/**
 * File: conditionRobotInHabitat
 *
 * Author: Arjun Menon
 * Created: 07-30-18
 *
 * Description: 
 * condition which returns true if the robot is in the habitat, false otherwise
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Engine_BehaviorSystem_ConditionRobotInHabitat_H__
#define __Engine_BehaviorSystem_ConditionRobotInHabitat_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/beiConditionDebugFactors.h"

namespace Anki {
namespace Vector {

class ConditionRobotInHabitat : public IBEICondition
{
public:
  // constructor
  explicit ConditionRobotInHabitat(const Json::Value& config);
  ~ConditionRobotInHabitat() = default;
  
  virtual void BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const override;
  
protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};
  
} // namespace
} // namespace

#endif // endif __Engine_BehaviorSystem_ConditionRobotInHabitat_H__

