/**
 * File: AIWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ObjectDetectorComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_ObjectDetectorComponent_H__

#include "engine/aiComponent/aiBeacon.h"

#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/externalInterface/externalInterface_fwd.h"

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/vision/engine/faceIdTypes.h"

#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include "clad/types/behaviorComponent/postBehaviorSuggestions.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <queue>
#include <set>
#include <vector>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

// Forward declarations
class BlockWorldFilter;  
class ObservableObject;
class Robot;
class SmartFaceID;

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
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
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

  mutable float _timeSinceLastObservation = 0; // TODO this is temporary for testing

};


} // namespace Cozmo
} // namespace Anki

#endif //
