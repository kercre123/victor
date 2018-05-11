/**
 * File: cubeAccelComponent.h
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Manages streamed object accel data
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_CubeAccelComponent_H__
#define __Anki_Cozmo_Basestation_Components_CubeAccelComponent_H__

#include "engine/cozmoObservableObject.h"
#include "engine/events/ankiEvent.h"
#include "engine/robotComponents_fwd.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/types/activeObjectAccel.h"

#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/shared/types.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <map>

static const Anki::TimeStamp_t kDefaultWindowSize_ms = 50;

namespace Anki {
namespace Cozmo {

struct CubeAccelData;
namespace CubeAccelListeners {
  class ICubeAccelListener;
  class MovementStartStopListener;
  class UpAxisChangedListener;
}

class Robot;

class CubeAccelComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  CubeAccelComponent();
  virtual ~CubeAccelComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  // Maintain the chain of initializations currently in robot - it might be possible to
  // change the order of initialization down the line, but be sure to check for ripple effects
  // when changing this function
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::BodyLights);
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  // Add a listener for the given ObjectId.
  // The listener gets run on the accelerometer data stream coming from the specified object.
  // Returns true if listener was successfully added.
  // Listeners are automatically removed when no one else is using them (shared pointer use count == 1)
  bool AddListener(const ObjectID& objectID,
                   const std::shared_ptr<CubeAccelListeners::ICubeAccelListener>& listener);

  template<typename T>
  void HandleMessage(const T& msg);
  
  void HandleCubeAccelData(const ActiveID& activeID,
                           const CubeAccelData& accelData);
  
private:
  
  // Incoming accel data is run on this set of listeners
  std::map<ObjectID, std::set<std::shared_ptr<CubeAccelListeners::ICubeAccelListener>>> _listenerMap;
  
  // Special listeners for detecting when cubes start/stop moving
  std::map<ObjectID, std::shared_ptr<CubeAccelListeners::MovementStartStopListener>> _movementListeners;
  
  std::map<ObjectID, std::shared_ptr<CubeAccelListeners::UpAxisChangedListener>> _upAxisChangedListeners;
  
  Robot* _robot = nullptr;
  
  std::list<Signal::SmartHandle> _eventHandlers;
  
  // Callbacks for object movement and up axis changed listeners
  void ObjectMovedOrStoppedCallback(const ObjectID objectId, const bool isMoving);
  void ObjectUpAxisChangedCallback(const ObjectID objectId, const UpAxis& upAxis);
  
};

}
}

#endif //__Anki_Cozmo_Basestation_Components_CubeAccelComponent_H__
