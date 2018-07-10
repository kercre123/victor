/**
 * File: salientPointDetectorComponent.h
 *
 * Author: Lorenzo Riano
 * Created: 06/04/18
 *
 * Description: A component for salient points. Listens to messages and categorizes them based on their type
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_ObjectDetectorComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_ObjectDetectorComponent_H__

#include "clad/types/behaviorComponent/postBehaviorSuggestions.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include "clad/types/salientPointTypes.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/vision/engine/faceIdTypes.h"
#include "engine/aiComponent/aiBeacon.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/externalInterface/externalInterface_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include<list>


namespace Anki {
namespace Cozmo {

// Forward declarations
class Robot;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SalientPointsDetectorComponent
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SalientPointsDetectorComponent : public IDependencyManagedComponent<AIComponentID>,
                                       private Util::noncopyable
{
public:


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent<AIComponentID> functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InitDependent(Cozmo::Robot* robot,
                     const AICompMap& dependentComps) override;

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  SalientPointsDetectorComponent();

  ~SalientPointsDetectorComponent() override ;

  // IDependencyManagedComponent<AIComponentID> functions
  void UpdateDependent(const AICompMap& dependentComps) override;
  // end IDependencyManagedComponent<AIComponentID> functions

  bool PersonDetected() const;

  // Remove all the Person salient points from the local container and move them to the new one
  template <typename Container>
  void GetLastPersonDetectedData(Container& salientPoints) const {

    //Solution adapted from https://stackoverflow.com/a/32155973/1047543

    // partition: all elements that should not be moved come before
    // (note that the condition is false) all elements that should be moved.
    // stable_partition maintains relative order in each group
    auto p = std::stable_partition(_latestSalientPoints.begin(), _latestSalientPoints.end(),
                                   [](const Vision::SalientPoint& p) {
                                     return p.salientType != Vision::SalientPointType::Person;
                                   }
    );
    salientPoints.insert(salientPoints.end(), std::make_move_iterator(p),
                         std::make_move_iterator(_latestSalientPoints.end()));
    _latestSalientPoints.erase(p, _latestSalientPoints.end());
  }

  template <typename Container>
  void AddSalientPoints(const Container& c) {
    // TODO need a circular buffer here, otherwise the list will keep growing until somebody fetches the data
    // The Utils::CircularBuffer lacks all the sweet iterators I'm using here
    _latestSalientPoints.insert(_latestSalientPoints.end(), std::begin(c), std::end(c));

  }

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // A list of latest received salient points. Will be overwritten if a new batch arrives
  mutable std::list<Vision::SalientPoint> _latestSalientPoints; // TODO mutable is temporary for testing

  mutable float _timeSinceLastObservation = 0; // TODO this is temporary for testing

  Robot* _robot;

};


} // namespace Cozmo
} // namespace Anki

#endif //
