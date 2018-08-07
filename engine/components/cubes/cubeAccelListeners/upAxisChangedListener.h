/**
 * File: upAxisChangedListener.h
 *
 * Author: Matt Michini
 * Created: 03/23/2018
 *
 * Description: Listener for detecting changes of the cube's upward pointing axis
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_CubeAccelListeners_UpAxisChangedListener_H__
#define __Engine_CubeAccelListeners_UpAxisChangedListener_H__

#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"

#include "engine/components/cubes/cubeAccelListeners/lowPassFilterListener.h"

#include "clad/types/activeObjectAccel.h"

#include <functional>
#include <memory>

namespace Anki {
namespace Vector {

struct ActiveAccel;
  
namespace CubeAccelListeners {

// Calls a callback whenever the cube's dominant "upAxis" changes and remains stable for a while
class UpAxisChangedListener : public ICubeAccelListener
{
public:
  
  UpAxisChangedListener(std::function<void(const UpAxis&)> callback);
  
protected:
  virtual void InitInternal(const ActiveAccel& accel) override;

  virtual void UpdateInternal(const ActiveAccel& accel) override;
  
private:

  UpAxis _upAxis = UpAxis::UnknownAxis;
  
  // Callback called when upAxis changes. Passed in argument is the new UpAxis
  std::function<void(const UpAxis&)> _callback;

  std::shared_ptr<ActiveAccel> _lowPassOutput;
  
  std::unique_ptr<LowPassFilterListener> _lowPassFilterListener;
  
  void Init(const ActiveAccel& accel);
   
  bool _inited = false;
  
};


}
}
}

#endif //__Engine_CubeAccelListeners_UpAxisChangedListener_H__
