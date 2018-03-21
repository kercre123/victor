/**
 * File: cubeAccelComponentListener.h
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Various listeners that run on streaming cube accel data
 *              Used by CubeAccelComponent
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_CubeAccelComponentListener_H__
#define __Anki_Cozmo_Basestation_Components_CubeAccelComponentListener_H__

#include "clad/types/activeObjectAccel.h"

#include "coretech/common/engine/math/rotation.h"

namespace Anki {
namespace Cozmo {
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

// Empty filter
// Used if Engine doesn't care about the accel data but outside sources like game/sdk does
class DummyListener : public ICubeAccelListener
{
public:
  DummyListener() { };
  
protected:
  virtual void InitInternal(const ActiveAccel& accel) override { };
  virtual void UpdateInternal(const ActiveAccel& accel) override { };
};

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

// Parameterized shake filter
// Attempts to only detect shaking (back and forth movement of any kind)
class ShakeListener : public ICubeAccelListener
{
public:
  // Typical values for any kind of shake detection are
  // highPassFilterCoeff = 0.5, highPassLowerThresh = 2.5, highPassUpperThresh = 3.9
  // The callback will be called when shaking is detected. The callback parameter is the amount
  // of shaking (how violent the shaking is)
  // The larger hpUpperThresh is the larger/more violent the shake need to initially trigger
  // shake detection.
  // The smaller hpLowerThresh is the smaller the shake needed to continue to trigger shake
  // detection once an initial shake has been detected
  ShakeListener(const float highPassFilterCoeff,
                const float highPassLowerThresh,
                const float highPassUpperThresh,
                std::function<void(const float)> callback);
  
protected:
  virtual void InitInternal(const ActiveAccel& accel) override;
  
  virtual void UpdateInternal(const ActiveAccel& accel) override;
  
private:
  // High pass filter used by this filter
  std::unique_ptr<HighPassFilterListener> _highPassFilter;
  
  // Callback called when shaking is detected. Passed in argument is the intensity of the shaking
  std::function<void(const float)> _callback;
  
  // Once shaking is detected use this threshold for further shake detection
  const float _lowerThreshSq;
  
  // Used for the initial shake detection (typically larger than the lowerThresh but doesn't have to be)
  const float _upperThreshSq;
  
  // Contains the output of the high pass filter
  std::shared_ptr<ActiveAccel> _highPassOutput;
  
  // Whether or not a shake has been detected
  bool _shakeDetected = false;
  
};

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

// Rotation filter to calculate the 3d rotation of an object using the gravity vector
// Only works while object is still
// TODO: Use MovementFilter to know when object is not moving
class RotationListener: public ICubeAccelListener
{
public:
  RotationListener(std::weak_ptr<Rotation3d> output);
  
protected:
  virtual void InitInternal(const ActiveAccel& accel);
  
  virtual void UpdateInternal(const ActiveAccel& accel);
  
private:
  std::weak_ptr<Rotation3d> _output;
  
};

}
}
}

#endif //__Anki_Cozmo_Basestation_Components_CubeAccelComponentListener_H__
