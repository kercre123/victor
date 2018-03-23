/**
 * File: lowPassFilterListener.h
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener that low-pass filters the cube accelerometer data
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_CubeAccelListeners_LowPassFilterListener_H__
#define __Engine_CubeAccelListeners_LowPassFilterListener_H__

#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"

#include "coretech/common/engine/math/point.h"

namespace Anki {
namespace Cozmo {
  
struct ActiveAccel;
  
namespace CubeAccelListeners {

// Typical low pass filter on each of the 3 axes of accelerometer data
class LowPassFilterListener : public ICubeAccelListener
{
public:
  // The reference to output is updated with the latest output of the filter
  // The lower the coeffs, the more the data is filtered (rapid changes won't affect filter much)
  // The higher the coeffs, the less the data is filtered
  LowPassFilterListener(const Vec3f& coeffs, std::weak_ptr<ActiveAccel> output);
  
protected:
  virtual void InitInternal(const ActiveAccel& accel) override;
  
  virtual void UpdateInternal(const ActiveAccel& accel) override;
  
private:
  // Coefficients for each axis
  const Vec3f _coeffs;
  
  // Reference to a variable that will be updated with the output of the filter
  // Make sure reference variable does not go out of scope in the owner of the filter
  std::weak_ptr<ActiveAccel> _output;
  
};


}
}
}

#endif //__Engine_CubeAccelListeners_LowPassFilterListener_H__
