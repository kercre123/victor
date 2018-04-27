/**
 * File: iCubeAccelListener.h
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

#ifndef __Engine_CubeAccelListeners_ICubeAccelListener_H__
#define __Engine_CubeAccelListeners_ICubeAccelListener_H__

namespace Anki {
namespace Cozmo {
  
struct ActiveAccel;
  
namespace CubeAccelListeners {

class ICubeAccelListener
{
public:
  virtual ~ICubeAccelListener() {};

  void Update(const ActiveAccel& accel);
  
protected:
  ICubeAccelListener() { };
  
  virtual void InitInternal(const ActiveAccel& accel) = 0;
  virtual void UpdateInternal(const ActiveAccel& accel) = 0;
  
private:
  bool _inited = false;
  
};

}
}
}

#endif //__Engine_CubeAccelListeners_ICubeAccelListener_H__
