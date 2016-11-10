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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorPoseBasedAcknowledgementInterface.h"

#include "clad/types/objectFamilies.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}
class ObservableObject;
  
class BehaviorAcknowledgeObject : public IBehaviorPoseBasedAcknowledgement
{
private:
  using super = IBehaviorPoseBasedAcknowledgement;
public:

  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeObject(Robot& robot, const Json::Value& config);

  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternalAcknowledgement(Robot& robot) override;

  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;

  virtual void AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
private:
  
  // TODO: Set these from JSON config too?
  std::set<ObjectFamily> _objectFamilies = {{
    ObjectFamily::LightCube,
    ObjectFamily::Block
  }};

  // NOTE: this may get called twice (once from ShouldConsiderObservedObjectHelper and once from AlwaysHandleInternal)
  void HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  
  void BeginIteration(Robot& robot);
  void LookForStackedCubes(Robot& robot);
  void BackupToSeeGhostCube(Robot& robot);
  void FinishIteration(Robot& robot);
  
  
  // helper function to set the ghost object's pose
  void SetGhostBlockPoseRelObject(Robot& robot, const ObservableObject* obj, float zOffset);
  // helper function to check stack visibility
  bool CheckIfGhostBlockVisible(Robot& robot, const ObservableObject* obj, float zOffset, bool& shouldRetry);
  // helper function for looking at ghost block location
  template<typename T>
  void LookAtGhostBlock(Robot& robot, void(T::*callback)(Robot&));
  
  
  // NOTE: uses s32 instead of ObjectID to match IBehaviorPoseBasedAcknowledgement's generic ids
  ObjectID _currTarget;

  // fake object to try to look at to check for possible stacks
  std::unique_ptr<ObservableObject> _ghostStackedObject;

  bool _shouldStart = false;
  
  bool _shouldCheckBelowTarget;
  
  bool _shouldBackupToSeeAbove;
  
}; // class BehaviorAcknowledgeObject

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeObject_H__
