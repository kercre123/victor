/**
 * File: lightsComponent.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-21
 *
 * Description: Central place for logic and control of robot and cube lights
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_LightsComponent_H__
#define __Anki_Cozmo_Basestation_Components_LightsComponent_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/common/types.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal.hpp"
#include <map>
#include <list>

namespace Anki {
namespace Cozmo {

class Robot;

class LightsComponent : private Util::noncopyable
{
public:
  LightsComponent(Robot& robot);

  // TODO:(bn) handle robot lights here as well as cube

  void Update();

  // tell this component to stop controlling these lights (e.g. because game is going to do it), or start
  // again. Disabling turns them all to "off"
  void SetEnableComponent(bool enable);

  template<typename T>
  void HandleMessage(const T& msg);
  
private:

  enum class CubeLightsState {
    Off,
    Connected,
    Visible
  };

  struct ObjectInfo {
    ObjectInfo();

    // desired state is what we want, currState is the last thing we've sent down (this avoids spamming too
    // many messages)
    
    CubeLightsState desiredState;
    CubeLightsState currState;
    TimeStamp_t     lastObservedTime_ms;
  };
  
  std::map< ObjectID, ObjectInfo > _cubeInfo;

  void UpdateToDesiredLights();
  void SetLights(ObjectID object, CubeLightsState state);

  // returns true if we should change the lights from currState to nextState
  static bool ShouldOverrideState(CubeLightsState currState, CubeLightsState nextState);

  Robot& _robot;

  bool _enabled = true;
  
  std::list<Signal::SmartHandle> _eventHandles;
};

}
}

#endif
