/**
 * File: lowPassFilterListener.cpp
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener that low-pass filters the cube accelerometer data
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/cubes/cubeAccelListeners/lowPassFilterListener.h"

#include "coretech/common/engine/math/point_impl.h"
#include "clad/types/activeObjectAccel.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace CubeAccelListeners {
  
LowPassFilterListener::LowPassFilterListener(const Vec3f& coeffs, std::weak_ptr<ActiveAccel> output)
: _coeffs(coeffs)
, _output(output)
{

}

void LowPassFilterListener::InitInternal(const ActiveAccel& accel)
{
  auto output = _output.lock();
  if(output == nullptr)
  {
    DEV_ASSERT(false, "LowPassFilterListener.InitInternal.NullOutput");
    return;
  }
  
  *output = accel;
}

void LowPassFilterListener::UpdateInternal(const ActiveAccel& accel)
{
  auto output = _output.lock();
  if(output == nullptr)
  {
    DEV_ASSERT(false, "LowPassFilterListener.UpdateInteral.NullOutput");
    return;
  }
  
  output->x = accel.x * _coeffs.x() + output->x * (1.f - _coeffs.x());
  output->y = accel.y * _coeffs.y() + output->y * (1.f - _coeffs.y());
  output->z = accel.z * _coeffs.z() + output->z * (1.f - _coeffs.z());
}

}
}
}
