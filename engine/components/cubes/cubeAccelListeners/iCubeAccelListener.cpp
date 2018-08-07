/**
 * File: iCubeAccelListener.cpp
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Interface for listeners that run on streaming cube accel data
 *              Used by CubeAccelComponent
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"

#include "clad/types/activeObjectAccel.h"

namespace Anki {
namespace Vector {
namespace CubeAccelListeners {
  

void ICubeAccelListener::Update(const ActiveAccel& accel)
{
  if(!_inited)
  {
    InitInternal(accel);
    _inited = true;
  }
  else
  {
    UpdateInternal(accel);
  }
}
  
}
}
}
