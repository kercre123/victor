/**
 * File: behaviorAcknowledgeObject.h
 *
 * Author:  Andrew Stein
 * Created: 2016-06-14
 *
 * Description:  Simple quick reaction to a "new" object, just to show Cozmo has noticed it.
 *               Cozmo just turns towards the object and then plays a reaction animation.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeObject_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeObject_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/shared/radians.h"

#include "anki/cozmo/basestation/behaviors/behaviorPoseBasedAcknowledgementInterface.h"

#include "clad/types/objectFamilies.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}
  
  
class BehaviorAcknowledgeObject : public IBehaviorPoseBasedAcknowledgement
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeObject(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual float EvaluateScoreInternal(const Robot& robot) const override;
  
  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override;
  
private:
  
  // TODO: Set these from JSON config too?
  std::set<ObjectFamily> _objectFamilies = {{
    ObjectFamily::LightCube,
    ObjectFamily::Block
  }};
  
  void HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  void HandleObjectMarkedUnknown(const Robot& robot, ObjectID objectID);
  
  ObjectID _targetObject;
  
}; // class BehaviorAcknowledgeObject

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeObject_H__
