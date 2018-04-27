/**
 * File: shakeListener.cpp
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener to detect shaking of a cube
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/cubes/cubeAccelListeners/shakeListener.h"

#include "clad/types/activeObjectAccel.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace CubeAccelListeners {

ShakeListener::ShakeListener(const float highPassFilterCoeff,
                             const float highPassLowerThresh,
                             const float highPassUpperThresh,
                             std::function<void(const float)> callback)
: _callback(callback)
, _lowerThreshSq(highPassLowerThresh*highPassLowerThresh)
, _upperThreshSq(highPassUpperThresh*highPassUpperThresh)
{
  _highPassOutput.reset(new ActiveAccel());
  _highPassFilter.reset(new HighPassFilterListener(highPassFilterCoeff, _highPassOutput));
}

void ShakeListener::InitInternal(const ActiveAccel& accel)
{
  _highPassFilter->Update(accel); // This will init the high pass filter
}

void ShakeListener::UpdateInternal(const ActiveAccel& accel)
{
  _highPassFilter->Update(accel);
  
  const float magSq = _highPassOutput->x * _highPassOutput->x +
                      _highPassOutput->y * _highPassOutput->y +
                      _highPassOutput->z * _highPassOutput->z;
  
  // Once the highPassFilter has exceeded the upperThreshold use the lowerThreshold
  // to detect smaller movements
  _shakeDetected = (_shakeDetected ?
                    magSq > _lowerThreshSq :
                    magSq > _upperThreshSq);
  
  if(_shakeDetected && _callback != nullptr)
  {
    _callback(magSq);
  }
}

}
}
}
