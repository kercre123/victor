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

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

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
  
  
class BehaviorAcknowledgeFace : public IBehavior
{
private:
  using super = IBehavior;

public:
  virtual bool CarryingObjectHandledInternally() const override {return false;}

  virtual void AddListener(IReactToFaceListener* listener) override;

  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeFace(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeFace& preReqData ) const override;

  
private:
  void BeginIteration(Robot& robot);
  void FinishIteration(Robot& robot);

  // sets _targetFace to the best face to look at from _desiredTargets. Returns true if it found one to react
  // to, false otherwise
  bool UpdateBestTarget(const Robot& robot);

  // current target
  Vision::FaceID_t _targetFace = Vision::UnknownFaceID;

  // everything we want to react to before we stop (to handle multiple faces in the same frame)
  mutable std::set< Vision::FaceID_t > _desiredTargets;
  
  bool _shouldStart = false;

  // whether or not the robot has played it's "big initial greeting" animation, which should only happen once
  // per session
  bool _hasPlayedInitialGreeting = false;
  
  
  std::set<IReactToFaceListener*> _faceListeners;
  
}; // class BehaviorAcknowledgeFace

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeFace_H__
