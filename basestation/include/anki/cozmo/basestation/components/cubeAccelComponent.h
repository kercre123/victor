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

#include "anki/cozmo/basestation/components/cubeAccelComponentListeners.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/types.h"

#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include "clad/robotInterface/messageFromActiveObject.h"

#include <map>

static const Anki::TimeStamp_t kDefaultWindowSize_ms = 50;

namespace Anki {
namespace Cozmo {

class Robot;

class CubeAccelComponent : private Util::noncopyable
{
public:
  CubeAccelComponent(Robot& robot);
  virtual ~CubeAccelComponent();

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
  void HandleObjectAccel(const ObjectAccel& objectAccel);

private:
  
  void ReceiveObjectAccelData(const AnkiEvent<RobotInterface::RobotToEngine>& msg);

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
  
  Robot& _robot;
  
  std::map<ObjectID, AccelHistory> _objectAccelHistory;
  
  Signal::SmartHandle _eventHandler;
  
};

}
}

#endif //__Anki_Cozmo_Basestation_Components_CubeAccelComponent_H__
