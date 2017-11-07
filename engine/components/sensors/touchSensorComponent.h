/**
 * File: touchSensorComponent.h
 *
 * Author: Arjun Menon
 * Created: 9/7/2017
 *
 * Description: Component for managing the capacitative touch sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_Components_TouchSensorComponent_H__
#define __Engine_Components_TouchSensorComponent_H__

#include "engine/components/sensors/iSensorComponent.h"

#include "clad/types/touchGestureTypes.h"

namespace Anki {
namespace Cozmo {

class TouchSensorComponent : public ISensorComponent
{
public:
  TouchSensorComponent(Robot& robot);
  ~TouchSensorComponent() = default;
  
protected:
  virtual void UpdateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;

public:
  
  TouchGesture GetLatestTouchGesture() const {
    return _touchGesture;
  }

  u32 GetLatestRawTouchValue() const { return _lastRawTouchValue; }
  
private:
  
  // the latest computed result of touch gesture
  TouchGesture _touchGesture;

  u16 _lastRawTouchValue = 0;

};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_TouchSensorComponent_H__
