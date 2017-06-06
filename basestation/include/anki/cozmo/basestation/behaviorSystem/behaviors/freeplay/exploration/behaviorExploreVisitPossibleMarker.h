/**
 * File: behaviorExploreVisitPossibleMarker
 *
 * Author: Raul
 * Created: 04/06/16
 *
 * Description: Behavior to check out what we thought previously it might be a marker. We could not confirm it was
 * a marker, so we want to get closer and confirm that.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorExploreVisitPossibleMarker_H__
#define __Cozmo_Basestation_Behaviors_BehaviorExploreVisitPossibleMarker_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"

#include "anki/common/basestation/math/pose.h"
#include "clad/types/objectTypes.h"

#include <vector>
#include <set>

namespace Anki {
namespace Cozmo {

// Forward declaration
namespace ExternalInterface {
struct RobotObservedObject;
}
class IAction;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorExploreVisitPossibleMarker
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorExploreVisitPossibleMarker : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorExploreVisitPossibleMarker(Robot& robot, const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorExploreVisitPossibleMarker() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}

protected:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result InitInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override {} // TODO?
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // create the proper action to see the cube
  void ApproachPossibleCube(Robot& robot, ObjectType objectType, const Pose3d& possibleCubePose);

  // mark the pose as invalid since we tried to search for something there and failed
  void MarkPossiblePoseAsEmpty(Robot& robot, ObjectType objectType, const Pose3d& pose);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // known possible objects we can obtain from the whiteboard (and cache for a run)
  mutable AIWhiteboard::PossibleObjectVector _possibleObjects;

};
  

} // namespace Cozmo
} // namespace Anki

#endif //
