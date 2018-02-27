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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/aiWhiteboard.h"

#include "coretech/common/engine/math/pose.h"
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
class BehaviorExploreVisitPossibleMarker : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorExploreVisitPossibleMarker(const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorExploreVisitPossibleMarker() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override {} // TODO?
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // create the proper action to see the cube
  void ApproachPossibleCube(ObjectType objectType, const Pose3d& possibleCubePose);

  // mark the pose as invalid since we tried to search for something there and failed
  void MarkPossiblePoseAsEmpty(ObjectType objectType, const Pose3d& pose);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // known possible objects we can obtain from the whiteboard (and cache for a run)
  mutable AIWhiteboard::PossibleObjectVector _possibleObjects;

};
  

} // namespace Cozmo
} // namespace Anki

#endif //
