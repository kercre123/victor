/**
 * File: potentialObjectsForLocalizingTo.h
 *
 * Author: Andrew Stein
 * Created: 7/5/2016
 *
 * Description:
 *   Helper class used during AddAndUpdate for tracking potential objects to localize to.
 *   Keeps the closest observed/matching object pair in each coordinate frame.
 *   Updates the matching object's pose to the observed pose for those that are not used
 *   or when they are replaced by a closer pair, if they are in the current origin.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_PotentialObjectsForLocalizingTo_H__
#define __Anki_Cozmo_PotentialObjectsForLocalizingTo_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/poseOrigin.h"
#include "coretech/common/engine/objectIDs.h"

#include <map>

namespace Anki {
  
namespace Vector {

// Forward decl.
class Robot;
class ObservableObject;
  
class PotentialObjectsForLocalizingTo
{
public:
  
  PotentialObjectsForLocalizingTo(Robot& robot);

  // Add the observed/matched pair as a potential localization object if it
  // is closer than the current one stored for its coordinate frame.
  // If in the robot's current coordinate frame, the passed-in matchedObject's
  // pose will be set to the observedObject's pose if it goes unused (i.e. is not
  // closer), or the currently-stored matchedObject's pose will be set to the
  // corresponding observedObject's pose before it is replaced by the closer
  // passed-in pair.
  bool Insert(const std::shared_ptr<ObservableObject>& observedObject,
              ObservableObject* matchedObject,
              f32 observedDistance,
              bool observationAlreadyUsed);
  
  // Localize the robot we were constructed with, using any observed/matched
  // pairs that have been inserted thus far. Does nothing if there are no pairs.
  Result LocalizeRobot();
  
private:
  
  Robot& _robot;
  
  // Struct for storing pairs of currently observed objects and their
  // matching object that is currently known.
  struct ObservedAndMatchedPair {
    std::shared_ptr<ObservableObject> observedObject;
    ObservableObject* matchedObject;
    ObjectID matchedID; // normally left unset, but may be used if matchedObject is set to null
    f32 distance;
    bool observationAlreadyUsed; // see note in BlockWorld about variable with the same name
  };
  
  // Set the matched object's pose to the observed pose because the observation is not used for localization
  void UseDiscardedObservation(ObservedAndMatchedPair& pair, bool wasRobotMoving);
  
  std::map<PoseOriginID_t, ObservedAndMatchedPair> _pairMap;
  
  // Completely skip all localization logic if the robot is not in a state
  // that it can localize to an object anyway
  const bool _kCanRobotLocalizeToObjects;
  
  bool CouldUseObjectForLocalization(const ObservableObject* matchingObject);
  
}; // class PotentialObjectsForLocalizingTo

} // namespace Anki
} // namespace Vector

#endif // __Anki_Cozmo_PotentialObjectsForLocalizingTo_H__
