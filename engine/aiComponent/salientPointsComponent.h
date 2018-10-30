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

#ifndef __Cozmo_Basestation_BehaviorSystem_SalientPointsComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_SalientPointsComponent_H__

#include "clad/types/behaviorComponent/postBehaviorSuggestions.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include "clad/types/salientPointTypes.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/vision/engine/faceIdTypes.h"
#include "coretech/common/engine/robotTimeStamp.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/externalInterface/externalInterface_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <map>


namespace Anki {
namespace Vector {

// Forward declarations
class Robot;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SalientPointsComponent
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SalientPointsComponent : public IDependencyManagedComponent<AIComponentID>,
                                       private Util::noncopyable
{
public:


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent<AIComponentID> functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InitDependent(Vector::Robot* robot,
                     const AICompMap& dependentComps) override;

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  SalientPointsComponent();

  ~SalientPointsComponent() override = default;

  // Get all the SalientPoints of a specific type since a specific timestamp
  void GetSalientPointSinceTime(std::list<Vision::SalientPoint>& salientPoints,
                                const Vision::SalientPointType& type, const RobotTimeStamp_t timestamp = 0) const;

  // Returns true wheter a SalientPoint of a spefic type has been seen since a specific timestamp
  bool SalientPointDetected(const Vision::SalientPointType& type, const RobotTimeStamp_t timestamp = 0) const;

  // Adds a list of SalientPoint. All the points are stored according to their type. If a specific list of points is
  // already stored for a type, it will be replaced.
  void AddSalientPoints(const std::list<Vision::SalientPoint>& c);

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // a list of the latest salient points organized per type. The points are replaced every time new ones of the
  // same type arrive
  std::map<Vision::SalientPointType, std::list<Vision::SalientPoint>> _salientPoints;

  mutable float _timeSinceLastObservation = 0; // TODO this is temporary for testing

  Robot* _robot;

};


} // namespace Vector
} // namespace Anki

#endif //
