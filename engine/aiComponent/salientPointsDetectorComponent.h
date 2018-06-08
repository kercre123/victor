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
  explicit SalientPointsDetectorComponent();

  ~SalientPointsDetectorComponent() override ;

  // IDependencyManagedComponent<AIComponentID> functions
  void UpdateDependent(const AICompMap& dependentComps) override;
  // end IDependencyManagedComponent<AIComponentID> functions

  bool PersonDetected() const;

  // returns false if no person data is available (e.g. it's been fetched already)
  template <typename Container>
  void GetLastPersonDetectedData(Container& salientPoints) const {

//    PRINT_CH_INFO("Behaviors","SalientPointsDetectorComponent.GetLastPersonDetectedData", "Passed list has %d points",
//                  int(salientPoints.size()));
//    PRINT_CH_INFO("Behaviors","SalientPointsDetectorComponent.GetLastPersonDetectedData", "There's %d salient points",
//                  int(_latestSalientPoints.size()));


    std::copy_if(_latestSalientPoints.begin(), _latestSalientPoints.end(), std::back_inserter(salientPoints),
                 [](const Vision::SalientPoint& p) {
                   PRINT_CH_INFO("Behaviors","SalientPointsDetectorComponent.GetLastPersonDetectedData",
                                 "Checking a point with type %s",
                                Vision::SalientPointTypeToString(p.salientType));
                   return p.salientType == Vision::SalientPointType::Person;
                 }
    );
//    PRINT_CH_INFO("Behaviors","SalientPointsDetectorComponent.GetLastPersonDetectedData", " %zu elements have been copied",
//                  salientPoints.size());
  }

  template <typename Container>
  void addSalientPoints(const Container& c) {
    std::copy(std::begin(c), std::end(c), std::begin(_latestSalientPoints));
  }

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // A list of latest received salient points. Will be overwritten if a new batch arrives
  mutable std::list<Vision::SalientPoint> _latestSalientPoints; // TODO mutable is temporary for checking

  mutable float _timeSinceLastObservation = 0; // TODO this is temporary for testing

  Robot* _robot;

};


} // namespace Cozmo
} // namespace Anki

#endif //
