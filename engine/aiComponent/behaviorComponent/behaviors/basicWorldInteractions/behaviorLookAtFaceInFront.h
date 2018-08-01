/**
 * File: BehaviorLookAtFaceInFront.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-24
 *
 * Description: Robot looks up to see if there's a face in front of it, and centers on a face if found
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLookAtFaceInFront__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLookAtFaceInFront__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Cozmo {

class BehaviorLookAtFaceInFront : public ICozmoBehavior
{
public: 
  virtual ~BehaviorLookAtFaceInFront();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorLookAtFaceInFront(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    bool waitingForFaces; // in lieu of a state
    RobotTimeStamp_t robotTimeStampAtActivation;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  // returns true if smartID is set with a valid face to look at
  bool GetFaceIDToLookAt(SmartFaceID& smartID) const;

  void TransitionToLookUp();
  void TransitionToLookAtFace(const SmartFaceID& smartID);
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorLookAtFaceInFront__
