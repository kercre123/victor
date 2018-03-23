/**
 * File: rotationListener.cpp
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener to detect rotation of a cube
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/cubes/cubeAccelListeners/rotationListener.h"

#include "clad/types/activeObjectAccel.h"

#include "coretech/common/engine/math/matrix_impl.h"
#include "coretech/common/engine/math/point_impl.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace CubeAccelListeners {

RotationListener::RotationListener(std::weak_ptr<Rotation3d> output)
: _output(output)
{

}

void RotationListener::InitInternal(const ActiveAccel& accel)
{
  UpdateInternal(accel);
}

void RotationListener::UpdateInternal(const ActiveAccel& accel)
{
  auto output = _output.lock();
  if(output == nullptr)
  {
    DEV_ASSERT(false, "RotationListener.UpdateInternal.NullOutput");
    return;
  }
  
  // Math to calculate rotation between two vectors explained here
  // https://math.stackexchange.com/questions/180418/calculate-rotation-matrix-to-align-vector-a-to-vector-b-in-3d
  
  // Calculate the 3d rotation between the current gravity vector and the
  // ideal upright cube gravity vector
  static const Vec3f idealVec = {0,0,-1};
  static const Matrix_3x3f I({1,0,0,0,1,0,0,0,1});
  
  Vec3f gravVec = Vec3f(accel.x, accel.y, -accel.z);
  gravVec.MakeUnitLength();
  
  const Vec3f v = CrossProduct(gravVec, idealVec);
  const float c = DotProduct(gravVec, idealVec);
  
  // If c == -1 then the calculation of mult results in divide by zero so
  // just don't update _output
  if(c != -1)
  {
    // Vx is the skew-symmetric cross-product matrix of v
    const float initVals[] = { 0,    -v.z(), v.y(),
                               v.z(), 0,    -v.x(),
                              -v.y(), v.x(), 0};
    Matrix_3x3f Vx(initVals);
    
    const float mult = 1.f / (1.f + c);
    const Matrix_3x3f Vx2 = Vx * Vx;
    
    Matrix_3x3f R = I;
    R += Vx;
    R += (Vx2 * mult);
    
    *output = Rotation3d(std::move(R));
  }
}
  
}
}
}
