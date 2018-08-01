/**
 * File: conditionObjectKnown.h
 *
 * Author: ross
 * Created: April 17 2018
 *
 * Description: Condition that is true when an object has been seen recently
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_BeiConditions_ConditionObjectKnown_H__
#define __Engine_BeiConditions_ConditionObjectKnown_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Cozmo {
  
enum class ObjectType : int32_t;
class ObservableObject;
  
class ConditionObjectKnown : public IBEICondition
{
public:
  explicit ConditionObjectKnown(const Json::Value& config);
  ConditionObjectKnown(const Json::Value& config, TimeStamp_t maxAge_ms );
  
  virtual ~ConditionObjectKnown();
  
  // returns the object(s) that made this true, or empty if the saved info no longer maps to any
  // objects (maybe it's gone now)
  const std::vector<const ObservableObject*> GetObjects(BehaviorExternalInterface& behaviorExternalInterface) const;
  
protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive) override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override {
    requiredVisionModes.insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low });
  }
  
private:
  
  // The object types we care about
  std::set<ObjectType> _targetTypes;
  
  TimeStamp_t _maxAge_ms;
  bool _setMaxAge = false; // for asserting multiple ctors
  
  struct ObjectInfo {
    ObjectInfo( RobotTimeStamp_t t, ObjectID o ) : observedTime(t), objectID(o), matchedThisTickOnly(false) {}
    RobotTimeStamp_t observedTime;
    ObjectID objectID;
    bool matchedThisTickOnly;
  };
  mutable std::vector<ObjectInfo> _lastObjects;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Engine_BeiConditions_ConditionObjectKnown_H__
