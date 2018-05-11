/**
 * File: shakeListener.h
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener to detect shaking of a cube
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_CubeAccelListeners_ShakeListener_H__
#define __Engine_CubeAccelListeners_ShakeListener_H__

#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"

#include "engine/components/cubes/cubeAccelListeners/highPassFilterListener.h"

#include <functional>
#include <memory>

namespace Anki {
namespace Cozmo {
  
struct ActiveAccel;
  
namespace CubeAccelListeners {

// Parameterized shake filter
// Attempts to only detect shaking (back and forth movement of any kind)
class ShakeListener : public ICubeAccelListener
{
public:
  // Typical values for any kind of shake detection are
  // highPassFilterCoeff = 0.5, highPassLowerThresh = 2.5, highPassUpperThresh = 3.9
  // The callback will be called when shaking is detected. The callback parameter is the amount
  // of shaking (how violent the shaking is)
  // The larger hpUpperThresh is the larger/more violent the shake need to initially trigger
  // shake detection.
  // The smaller hpLowerThresh is the smaller the shake needed to continue to trigger shake
  // detection once an initial shake has been detected
  ShakeListener(const float highPassFilterCoeff,
                const float highPassLowerThresh,
                const float highPassUpperThresh,
                std::function<void(const float)> callback);
  
protected:
  virtual void InitInternal(const ActiveAccel& accel) override;
  
  virtual void UpdateInternal(const ActiveAccel& accel) override;
  
private:
  // High pass filter used by this filter
  std::unique_ptr<HighPassFilterListener> _highPassFilter;
  
  // Callback called when shaking is detected. Passed in argument is the intensity of the shaking
  std::function<void(const float)> _callback;
  
  // Once shaking is detected use this threshold for further shake detection
  const float _lowerThreshSq;
  
  // Used for the initial shake detection (typically larger than the lowerThresh but doesn't have to be)
  const float _upperThreshSq;
  
  // Contains the output of the high pass filter
  std::shared_ptr<ActiveAccel> _highPassOutput;
  
  // Whether or not a shake has been detected
  bool _shakeDetected = false;
  
};
  
}
}
}

#endif //__Engine_CubeAccelListeners_ShakeListener_H__
