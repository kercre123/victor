/**
 * File: highPassFilterListener.h
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener that high-pass filters the cube accelerometer data
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/cubes/cubeAccelListeners/highPassFilterListener.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Vector {
namespace CubeAccelListeners {

HighPassFilterListener::HighPassFilterListener(const Vec3f& coeffs,
                                               std::weak_ptr<ActiveAccel> output)
: _coeffs(coeffs)
, _output(output)
, _prevInput(0,0,0)
{

}

void HighPassFilterListener::InitInternal(const ActiveAccel& accel)
{
  auto output = _output.lock();
  if(output == nullptr)
  {
    DEV_ASSERT(false, "HighPassFilterListener.InitInternal.NullOutput");
    return;
  }
  
  _prevInput = accel;
  *output = ActiveAccel(0.f, 0.f, 0.f);
}

void HighPassFilterListener::UpdateInternal(const ActiveAccel& accel)
{
  auto output = _output.lock();
  if(output == nullptr)
  {
    DEV_ASSERT(false, "HighPassFilterListener.UpdateInternal.NullOutput");
    return;
  }
  
  output->x = _coeffs.x() * (output->x + accel.x - _prevInput.x);
  output->y = _coeffs.y() * (output->y + accel.y - _prevInput.y);
  output->z = _coeffs.z() * (output->z + accel.z - _prevInput.z);
  _prevInput = accel;
}

}
}
}
