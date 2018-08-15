/**
* File: conditionFaceKnown.h
*
* Author:  ross
* Created: May 15 2018
*
* Description: Condition that is true if any (optionally named) face is known within a recent timeframe
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFaceKnown_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFaceKnown_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionFaceKnown : public IBEICondition
{
public:
  ConditionFaceKnown(const Json::Value& config);
  virtual ~ConditionFaceKnown() = default;

protected:
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const override {
    requests.insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low });
  }
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  float _maxFaceDist_mm; // ignored if negative
  int _maxFaceAge_s; // ignored if negative
  bool _mustBeNamed;

};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFaceKnown_H__
