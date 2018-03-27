/**
 * File: upAxisChangedListener.cpp
 *
 * Author: Matt Michini
 * Created: 03/23/2018
 *
 * Description: Listener for detecting changes of the cube's upward pointing axis
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/cubes/cubeAccelListeners/upAxisChangedListener.h"

#include "coretech/common/engine/math/point_impl.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace CubeAccelListeners {

namespace {
  Vec3f kLowPassFilterCoefs{0.03f, 0.03f, 0.03f};
}

UpAxisChangedListener::UpAxisChangedListener(std::function<void(const UpAxis&)> callback)
  : _callback(callback)
  , _lowPassOutput(std::make_shared<ActiveAccel>())
  , _lowPassFilterListener(std::make_unique<LowPassFilterListener>(kLowPassFilterCoefs, _lowPassOutput))
{
  
}

void UpAxisChangedListener::InitInternal(const ActiveAccel& accel)
{
  Init(accel);
}

void UpAxisChangedListener::UpdateInternal(const ActiveAccel& accel)
{
  if (!_inited) {
    Init(accel);
    return;
  }
  
  _lowPassFilterListener->Update(accel);
  
  // Compute up axis from accel data:
  auto currUpAxis = _upAxis;
  
  static const UpAxis upAxisMap[3][2] = {
    {UpAxis::XPositive, UpAxis::XNegative},
    {UpAxis::YPositive, UpAxis::YNegative},
    {UpAxis::ZPositive, UpAxis::ZNegative},
  };
  
  const float accelVec[3] = {
    _lowPassOutput->x,
    _lowPassOutput->y,
    _lowPassOutput->z,
  };
  
  float maxAccel = 0;
  for (int i=0 ; i<3 ; i++) {
    const auto accel = accelVec[i];
    if (std::abs(accel) > maxAccel) {
      currUpAxis = upAxisMap[i][(accel > 0) ? 0 : 1];
      maxAccel = std::abs(accel);
    }
  }
  
  if (currUpAxis != _upAxis) {
    _callback(currUpAxis);
    _upAxis = currUpAxis;
  }
}
  
void UpAxisChangedListener::Init(const ActiveAccel& accel) {
  _lowPassFilterListener->Update(accel);
  _inited = true;
}

}
}
}
