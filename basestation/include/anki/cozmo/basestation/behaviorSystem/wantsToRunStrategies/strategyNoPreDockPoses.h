/**
* File: strategyNoPreDockPoses.h
*
* Author: Kevin M. Karol
* Created: 12/08/16
*
* Description: Strategy for responding to the fact that an object has no predock
* poses
*
* Copyright: Anki, Inc. 2016
*
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyNoPreDockPoses_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyNoPreDockPoses_H__

#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

//Forward declarations
class ReactionObjectData;
class Robot;
  
class StrategyNoPreDockPoses : public IWantsToRunStrategy{
public:
  StrategyNoPreDockPoses(Robot& robot, const Json::Value& config);

protected:
  virtual bool WantsToRunInternal(const Robot& robot) const override;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyNoPreDockPoses_H__
