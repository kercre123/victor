/**
 * File: behaviorAcknowledgeFace.h
 *
 * Author:  Andrew Stein
 * Created: 2016-06-16
 *
 * Description:  Simple quick reaction to a "new" face, just to show Cozmo has noticed you.
 *               Cozmo just turns towards the face and then plays a reaction animation.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeFace_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/shared/radians.h"

#include "anki/cozmo/basestation/behaviors/behaviorPoseBasedAcknowledgementInterface.h"

#include "anki/vision/basestation/faceIdTypes.h"

#include "clad/types/objectFamilies.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
  struct RobotObservedFace;
}
  
  
class BehaviorAcknowledgeFace : public IBehaviorPoseBasedAcknowledgement
{

  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeFace(Robot& robot, const Json::Value& config);

  virtual Result InitInternalReactionary(Robot& robot) override;

  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;

  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
private:

  void HandleFaceObserved(const Robot& robot, const ExternalInterface::RobotObservedFace& msg);
  void HandleFaceDeleted(const Robot& robot, Vision::FaceID_t faceID);
  
  Vision::FaceID_t _targetFace = Vision::UnknownFaceID;
  
}; // class BehaviorAcknowledgeFace

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeFace_H__
