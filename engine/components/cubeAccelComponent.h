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

#include "engine/components/cubeAccelComponentListeners.h"
#include "engine/events/ankiEvent.h"
#include "engine/robotInterface/messageHandler.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/shared/types.h"
#include "engine/components/cubeAccelComponentListeners.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/robotComponents_fwd.h"
#include "engine/robotInterface/messageHandler.h"


#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"


#include <list>
#include <map>

static const Anki::TimeStamp_t kDefaultWindowSize_ms = 50;

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
  struct ObjectAvailable;
  struct ObjectAccel;
}

class Robot;

class CubeAccelComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  CubeAccelComponent();
  virtual ~CubeAccelComponent();

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
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////

  // Runs the listener on the accelerometer data stream coming from the specified object
  // Enables streaming object accel data if not already enabled for object
  // Sets the window size of this object's accelerometer data to be at least the specified
  // window size
  void AddListener(const ObjectID& objectID,
                   const std::shared_ptr<CubeAccelListeners::ICubeAccelListener>& listener,
                   const TimeStamp_t& windowSize_ms = kDefaultWindowSize_ms);

  // Removes the listener from the accelerometer data stream coming from the specified object
  // If this is the last listener then disables streaming object accel data for the object
  // Returns whether or not the listener was successfully removed
  bool RemoveListener(const ObjectID& objectID,
                      const std::shared_ptr<CubeAccelListeners::ICubeAccelListener>& listener);
  
  
  // Exposed for debug purposes - should not be called directly
  // whichLightCubeType should be 1, 2, or 3 for LightCube1, 2, or 3
  void Dev_HandleObjectAccel(const u32 whichLightCubeType, ExternalInterface::ObjectAccel& accel);
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  void HandleObjectAccel(const ExternalInterface::ObjectAccel& objectAccel);
  
private:
  
  struct AccelHistory
  {
    // Window size of data to keep in history
    TimeStamp_t windowSize_ms = kDefaultWindowSize_ms;
    
    // Historical accel data
    std::map<TimeStamp_t, ActiveAccel> history;
    
    // Incoming accel data is run on this set of listeners
    std::set<std::shared_ptr<CubeAccelListeners::ICubeAccelListener>> listeners;
  };
  
  // Culls history to it's window size
  void CullToWindowSize(AccelHistory& accelHistory);
  
  Robot* _robot = nullptr;
  
  std::map<ObjectID, AccelHistory> _objectAccelHistory;
  
  std::list<Signal::SmartHandle> _eventHandlers;
  
};

}
}

#endif //__Anki_Cozmo_Basestation_Components_CubeAccelComponent_H__
