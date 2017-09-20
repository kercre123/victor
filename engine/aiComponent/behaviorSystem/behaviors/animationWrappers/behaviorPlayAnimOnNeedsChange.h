/**
* File: behaviorPlayAnimOnNeedsChange.h
*
* Author: Kevin M. Karol
* Created: 6/30/17
*
* Description: Behavior which plays an animation and transitions
* cozmo into expressing needs state
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlayAnimOnNeedsChange_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlayAnimOnNeedsChange_H__

#include "engine/aiComponent/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimSequenceWithFace.h"

#include "clad/types/needsSystemTypes.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorPlayAnimOnNeedsChange : public BehaviorPlayAnimSequenceWithFace
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlayAnimOnNeedsChange(const Json::Value& config);
  
public:
  virtual ~BehaviorPlayAnimOnNeedsChange();

protected:
  virtual bool IsRunnableAnimSeqInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  
private:
  struct Params{
    NeedId _need;
  };
  
  Params _params;
  
  /// Methods
  bool ShouldGetInBePlayed(BehaviorExternalInterface& behaviorExternalInterface) const;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlayAnimOnNeedsChange_H__
