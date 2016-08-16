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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorPoseBasedAcknowledgementInterface.h"

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
  
  
class BehaviorAcknowledgeFace : public IReactionaryBehavior
{
private:
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;
  using super = IReactionaryBehavior;

public:
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeFace(Robot& robot, const Json::Value& config);

  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;

  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;

  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
private:

  void HandleFaceObserved(const Robot& robot, const ExternalInterface::RobotObservedFace& msg);

  void BeginIteration(Robot& robot);
  void FinishIteration(Robot& robot);

  // sets _targetFace to the best face to look at from _desiredTargets. Returns true if it found one to react
  // to, false otherwise
  bool GetBestTarget(const Robot& robot);

  // helper to add a face as something we want to react to (or not, if we are disabled). Returns true if it
  // added anything
  bool AddDesiredFace(Vision::FaceID_t faceID);
  
  // current target
  Vision::FaceID_t _targetFace = Vision::UnknownFaceID;

  // everything we want to react to before we stop (to handle multiple faces in the same frame)
  std::set< Vision::FaceID_t > _desiredTargets;
  
  bool _shouldStart = false;

  // face IDs (enrolled only) which we have done an initial reaction to
  std::set< Vision::FaceID_t > _hasReactedToFace;

  // true if we saw the face close last time, false otherwise (applies hysteresis internally)
  std::map< Vision::FaceID_t, bool > _faceWasClose;

  float _lastReactionTime_s = -1.0f;
  
}; // class BehaviorAcknowledgeFace

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeFace_H__
