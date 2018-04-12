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
  const float kMaxMovementScoreToAdd   = 500.f;
  const float kMovementScoreDecay      = 300.f;
  const float kMaxAllowedMovementScore = 20'000.f;
  
  // Started/Stopped movement score thresholds.
  // Simulator accelerometer behaves differently from real one,
  // so use different thresholds
#ifdef SIMULATOR
  const float kStartedMovingThresh = 3'000.f;
  const float kStoppedMovingThresh = 1'500.f;
#else
  const float kStartedMovingThresh = 10'000.f;
  const float kStoppedMovingThresh = 5'000.f;
#endif
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
