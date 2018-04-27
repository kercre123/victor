/**
 * File: movementListener.cpp
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener for detecting movement of the cube
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/cubes/cubeAccelListeners/movementListener.h"

#include "clad/types/activeObjectAccel.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace CubeAccelListeners {

MovementListener::MovementListener(const float hpFilterCoeff,
                                   const float maxMovementScoreToAdd,
                                   const float movementScoreDecay,
                                   const float maxAllowedMovementScore,
                                   std::function<void(const float)> callback)
: _callback(callback)
, _maxMovementScoreToAdd(maxMovementScoreToAdd)
, _movementScoreDecay(movementScoreDecay)
, _maxAllowedMovementScore(maxAllowedMovementScore)
{
  _highPassOutput.reset(new ActiveAccel());
  _highPassFilter.reset(new HighPassFilterListener(hpFilterCoeff, _highPassOutput));
}

void MovementListener::InitInternal(const ActiveAccel& accel)
{
  _highPassFilter->Update(accel); // This will init the high pass filter
}

void MovementListener::UpdateInternal(const ActiveAccel& accel)
{
  _highPassFilter->Update(accel);
  
  const float magSq = (_highPassOutput->x * _highPassOutput->x) +
                      (_highPassOutput->y * _highPassOutput->y) +
                      (_highPassOutput->z * _highPassOutput->z);
  const float mag = std::sqrt(magSq);
  
  _movementScore += std::min(_maxMovementScoreToAdd, mag);
  
  if(_movementScore > _movementScoreDecay)
  {
    _movementScore -= _movementScoreDecay;
  }
  else
  {
    _movementScore = 0;
  }
  
  _movementScore = std::min(_maxAllowedMovementScore, _movementScore);
  
  if(_movementScore > 0 && _callback != nullptr)
  {
    _callback(_movementScore);
  }
}

}
}
}
