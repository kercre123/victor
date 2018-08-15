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

#ifndef __Engine_CubeAccelListeners_HighPassFilterListener_H__
#define __Engine_CubeAccelListeners_HighPassFilterListener_H__

#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"

#include "clad/types/activeObjectAccel.h"

#include "coretech/common/engine/math/matrix_impl.h"

namespace Anki {
namespace Vector {

struct ActiveAccel;

namespace CubeAccelListeners {

// Typical high pass filter on each of the 3 axes of accelerometer data
class HighPassFilterListener : public ICubeAccelListener
{
public:
  // The reference to output is updated with the latest output of the filter
  // The lower the coeffs, the more sensitive the output of the filter is to
  // high frequencies.
  // As the coeffs increase, the filter becomes more sensitive to lower and lower frequencies
  HighPassFilterListener(const Vec3f& coeffs, std::weak_ptr<ActiveAccel> output);
  
protected:
  virtual void InitInternal(const ActiveAccel& accel) override;
  
  virtual void UpdateInternal(const ActiveAccel& accel) override;
  
private:
  // Coefficients for each axis
  const Vec3f _coeffs;
  
  // Weak pointer to a variable that will be updated with the output of the filter
  std::weak_ptr<ActiveAccel> _output;
  
  // Previous input to the filter
  ActiveAccel _prevInput;
  
};

}
}
}

#endif //__Engine_CubeAccelListeners_HighPassFilterListener_H__
