/**
 * File: salientPointDetectorComponent.h
 *
 * Author: Lorenzo Riano
 * Created: 06/04/18
 *
 * Description: A component for salient points. Listens to messages and categorize them based on their type
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
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"
#include "engine/externalInterface/externalInterface_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"


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
  explicit SalientPointsDetectorComponent(Robot& robot);

  ~SalientPointsDetectorComponent() override ;

  // IDependencyManagedComponent<AIComponentID> functions
  void UpdateDependent(const AICompMap& dependentComps) override;
  // end IDependencyManagedComponent<AIComponentID> functions

  bool PersonDetected() const;
  void GetLastPersonDetectedData(Vision::SalientPoint& salientPoint) const;

  template<typename T>
  void HandleMessage(const T& msg);

private:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // the robot this whiteboard belongs to
  Robot& _robot;
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  mutable Vision::SalientPoint _lastPersonDetected; // TODO made mutable here for testing

  mutable float _timeSinceLastObservation = 0; // TODO this is temporary for testing

};


} // namespace Cozmo
} // namespace Anki

#endif //
