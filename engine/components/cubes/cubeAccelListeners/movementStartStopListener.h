/**
 * File: movementStartStopListener.h
 *
 * Author: Matt Michini
 * Created: 03/26/2018
 *
 * Description: Listener for detecting when the cube starts or stops moving
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_CubeAccelListeners_MovementStartStopListener_H__
#define __Engine_CubeAccelListeners_MovementStartStopListener_H__

#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"

#include "engine/components/cubes/cubeAccelListeners/movementListener.h"

namespace Anki {
namespace Cozmo {
  
struct ActiveAccel;
  
namespace CubeAccelListeners {

class MovementStartStopListener : public ICubeAccelListener
{
public:
  MovementStartStopListener(std::function<void(void)> startedMovingCallback,
                            std::function<void(void)> stoppedMovingCallback);
  
protected:
  virtual void InitInternal(const ActiveAccel& accel) override;

  virtual void UpdateInternal(const ActiveAccel& accel) override;
  
private:
  
  // Function that movement listener calls whenever the cube is moving
  void MovementCallback(const float movementScore);
  
  // High pass filter used by this filter
  std::unique_ptr<MovementListener> _movementListener;
  
  std::function<void(void)> _startedMovingCallback;
  std::function<void(void)> _stoppedMovingCallback;
  
  // Movement score thresholds (corresponding to output of the movement listener)
  // used to determine when cube is considered to start and stop moving
  const float _startedMovingThresh;
  const float _stoppedMovingThresh;
  bool _moving = false;
  
};


}
}
}

#endif //__Engine_CubeAccelListeners_MovementStartStopListener_H__
