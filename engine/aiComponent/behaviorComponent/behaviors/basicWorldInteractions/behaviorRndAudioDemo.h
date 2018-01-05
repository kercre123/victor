/**
* File: behaviorRndAudioDemo.h
*
* Author: Matt Michini
* Created: 1/4/18
*
* Description:
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorRndAudioDemo_H__
#define __Cozmo_Basestation_Behaviors_BehaviorRndAudioDemo_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorRndAudioDemo : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorRndAudioDemo(const Json::Value& config);
  
public:
  virtual ~BehaviorRndAudioDemo() override {}
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual bool CarryingObjectHandledInternally() const override{ return false;}
  
protected:
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorRndAudioDemo_H__
