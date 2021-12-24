/**
 * File: rotationListener.h
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener to detect rotation of a cube
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_CubeAccelListeners_RotationListener_H__
#define __Engine_CubeAccelListeners_RotationListener_H__

#include <memory>

#include "coretech/common/shared/math/rotation.h"
#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"

namespace Anki {
namespace Vector {

struct ActiveAccel;

namespace CubeAccelListeners {

// Rotation filter to calculate the 3d rotation of an object using the gravity
// vector Only works while object is still
// TODO: Use MovementFilter to know when object is not moving
class RotationListener : public ICubeAccelListener {
 public:
  RotationListener(std::weak_ptr<Rotation3d> output);

 protected:
  virtual void InitInternal(const ActiveAccel& accel);

  virtual void UpdateInternal(const ActiveAccel& accel);

 private:
  std::weak_ptr<Rotation3d> _output;
};

}  // namespace CubeAccelListeners
}  // namespace Vector
}  // namespace Anki

#endif  //__Engine_CubeAccelListeners_RotationListener_H__
