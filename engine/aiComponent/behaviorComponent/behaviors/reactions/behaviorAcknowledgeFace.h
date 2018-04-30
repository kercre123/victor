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

#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/shared/radians.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

#include "coretech/vision/engine/faceIdTypes.h"

#include "clad/types/objectFamilies.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
  struct RobotObservedFace;
}
  
  
class BehaviorAcknowledgeFace : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;

public:

  virtual void AddListener(IReactToFaceListener* listener) override;
  
  void SetFacesToAcknowledge(const std::vector<SmartFaceID> targetFaces){_desiredTargets = targetFaces;}
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeFace(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

  virtual bool WantsToBeActivatedBehavior() const override;

  
private:
  void BeginIteration();
  void FinishIteration();

  // sets _targetFace to the best face to look at from _desiredTargets. Returns true if it found one to react
  // to, false otherwise
  bool UpdateBestTarget();

  // current target
  SmartFaceID _targetFace;

  // everything we want to react to before we stop (to handle multiple faces in the same frame)
  std::vector<SmartFaceID> _desiredTargets;
  
  bool _shouldStart = false;

  // whether or not the robot has played it's "big initial greeting" animation, which should only happen once
  // per session
  bool _hasPlayedInitialGreeting = false;
  
  
  std::set<IReactToFaceListener*> _faceListeners;
  
}; // class BehaviorAcknowledgeFace

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeFace_H__
