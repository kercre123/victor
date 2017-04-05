/**
 * File: behaviorPreReqAnimSequence.h
 *
 * Author: Al Chaussee
 * Created: 3/20/17
 *
 * Description: Defines prerequisites that can be passed into
 * behavior's isRunnable() that requires an animation sequence
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAnimSequence_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAnimSequence_H__

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BehaviorPreReqAnimSequence {
public:
  BehaviorPreReqAnimSequence(const std::vector<AnimationTrigger>& anims)
  :_anims(anims){}
  
  const std::vector<AnimationTrigger>& GetAnims() const { return _anims;}
  
private:
  const std::vector<AnimationTrigger> _anims;
  
};
  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAnimSequence_H__


