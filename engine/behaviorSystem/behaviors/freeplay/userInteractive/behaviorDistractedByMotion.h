/**
 * File: BehaviorDistractedByMotion.h
 *
 * Author: Lorenzo Riano
 * Created: 9/20/17
 *
 * Description: This is a behavior that turns the robot towards motion. Motion can happen in
 *              different areas of the camera: top, left and right. The robot will either move
 *              in place towards the motion (for left and right) or will look up.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef COZMO_BEHAVIORDISTRACTEDBYMOTION_H
#define COZMO_BEHAVIORDISTRACTEDBYMOTION_H

#include "engine/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorDistractedByMotion : public IBehavior
{
public:
  BehaviorDistractedByMotion(Robot &robot, const Json::Value &config);

  bool CarryingObjectHandledInternally() const override;

protected:
  Result InitInternal(Robot &robot) override;

  bool IsRunnableInternal(const Robot &robot) const override;

};


}
}


#endif //COZMO_BEHAVIORDISTRACTEDBYMOTION_H
