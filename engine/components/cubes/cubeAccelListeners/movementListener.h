/**
 * File: movementListener.h
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Listener for detecting movement of the cube
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_CubeAccelListeners_MovementListener_H__
#define __Engine_CubeAccelListeners_MovementListener_H__

#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"

#include "engine/components/cubes/cubeAccelListeners/highPassFilterListener.h"

namespace Anki {
namespace Cozmo {
  
struct ActiveAccel;
  
namespace CubeAccelListeners {

// Parameterized movement filter
// Attempts to detect movement of any kind including shaking/vibrations
class MovementListener : public ICubeAccelListener
{
public:
  // Typical values for any kind of movement detection are
  // hpFilterCoeff = 0.8, maxMovementScoreToAdd = 25, movementScoreDecay = 2.5,
  // maxAllowedMovementScore = 100
  // The smaller maxToAdd is the lower the movementScore will be from table shaking/random vibrations
  // so those are easier to filter out (still reported as movement so callback will be called). Does mean
  // that it will take longer for actual movements for the movementScore to reach maxAllowedScore
  // The larger the decay the quicker we stop reporting movement after stopping but also reduces
  // the ability to detect small movements
  MovementListener(const float hpFilterCoeff,
                   const float maxMovementScoreToAdd,
                   const float movementScoreDecay,
                   const float maxAllowedMovementScore,
                   std::function<void(const float)> callback);
  
protected:
  virtual void InitInternal(const ActiveAccel& accel) override;

  virtual void UpdateInternal(const ActiveAccel& accel) override;
  
private:
  // High pass filter used by this filter
  std::unique_ptr<HighPassFilterListener> _highPassFilter;
  
  // Callback called when movement is detected. Passed in argument is the intensity of the movement
  std::function<void(const float)> _callback;
  
  // Contains the output of the high pass filter
  std::shared_ptr<ActiveAccel> _highPassOutput;
  
  // The max amount movement score can increase each update
  const float _maxMovementScoreToAdd;
  
  // How much the movement score should decay each update
  const float _movementScoreDecay;
  
  // The max allowed movement score
  const float _maxAllowedMovementScore;
  
  // Score representing how much movement has been detected
  float _movementScore = 0;
};


}
}
}

#endif //__Engine_CubeAccelListeners_MovementListener_H__
