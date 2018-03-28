/**
 * File: movementStartStopListener.cpp
 *
 * Author: Matt Michini
 * Created: 03/26/2018
 *
 * Description: Listener for detecting when the cube starts or stops moving
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/cubes/cubeAccelListeners/movementStartStopListener.h"

#include "clad/types/activeObjectAccel.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace CubeAccelListeners {

namespace {
  // Parameters for movement listener
  const float kHpFilterCoef            = 0.8f;
  const float kMaxMovementScoreToAdd   = 25.f;
  const float kMovementScoreDecay      = 2.5f;
  const float kMaxAllowedMovementScore = 100.f;
  
  // Started/Stopped movement score thresholds
  const float kStartedMovingThresh = 50.f;
  const float kStoppedMovingThresh = 20.f;
}

MovementStartStopListener::MovementStartStopListener(std::function<void(void)> startedMovingCallback,
                                                     std::function<void(void)> stoppedMovingCallback)
: _startedMovingCallback(startedMovingCallback)
, _stoppedMovingCallback(stoppedMovingCallback)
, _startedMovingThresh(kStartedMovingThresh)
, _stoppedMovingThresh(kStoppedMovingThresh)
{
  auto movementCallback = [this](const float movementScore) {
    MovementCallback(movementScore);
  };
  
  _movementListener = std::make_unique<MovementListener>(kHpFilterCoef,
                                                         kMaxMovementScoreToAdd,
                                                         kMovementScoreDecay,
                                                         kMaxAllowedMovementScore,
                                                         movementCallback);
  
  DEV_ASSERT((_startedMovingThresh > _stoppedMovingThresh) &&
             (_startedMovingThresh < kMaxAllowedMovementScore) &&
             (_stoppedMovingThresh < kMaxAllowedMovementScore),
             "MovementStartStopListener.MovementStartStopListener.InvalidThresholds");
}

void MovementStartStopListener::InitInternal(const ActiveAccel& accel)
{
  // This will init the movement listener
  _movementListener->Update(accel);
}

void MovementStartStopListener::UpdateInternal(const ActiveAccel& accel)
{
  _movementListener->Update(accel);
}
  
void MovementStartStopListener::MovementCallback(const float movementScore)
{
  if (_moving && (movementScore < _stoppedMovingThresh)) {
    if (_stoppedMovingCallback != nullptr) {
      _stoppedMovingCallback();
    }
    _moving = false;
  } else if (!_moving && (movementScore > _startedMovingThresh)) {
    if (_startedMovingCallback != nullptr) {
      _startedMovingCallback();
    }
    _moving = true;
  }
}

}
}
}
