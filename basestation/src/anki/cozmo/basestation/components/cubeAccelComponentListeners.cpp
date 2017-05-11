/**
 * File: cubeAccelComponentListeners.cpp
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Various Listeners that run on streaming cube accel data
 *              Used by CubeAccelComponent
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/components/cubeAccelComponentListeners.h"

#include "anki/common/basestation/math/matrix_impl.h"
#include "anki/common/basestation/math/point_impl.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace CubeAccelListeners {
  
#pragma mark ---- ICubeAccelListener ----

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
      
#pragma mark ---- LowPassListener ----

LowPassFilterListener::LowPassFilterListener(const Vec3f& coeffs, std::weak_ptr<ActiveAccel> output)
: _coeffs(coeffs)
, _output(output)
{

}

void LowPassFilterListener::InitInternal(const ActiveAccel& accel)
{
  auto output = _output.lock();
  if(output == nullptr)
  {
    DEV_ASSERT(false, "LowPassFilterListener.InitInternal.NullOutput");
    return;
  }
  
  *output = accel;
}

void LowPassFilterListener::UpdateInternal(const ActiveAccel& accel)
{
  auto output = _output.lock();
  if(output == nullptr)
  {
    DEV_ASSERT(false, "LowPassFilterListener.UpdateInteral.NullOutput");
    return;
  }
  
  output->x = accel.x * _coeffs.x() + output->x * (1.f - _coeffs.x());
  output->y = accel.y * _coeffs.y() + output->y * (1.f - _coeffs.y());
  output->z = accel.z * _coeffs.z() + output->z * (1.f - _coeffs.z());
}

#pragma mark ---- HighPassFilterListener ----

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
  *output = accel;
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

#pragma mark ---- ShakeListener ----

ShakeListener::ShakeListener(const float highPassFilterCoeff,
                             const float highPassLowerThresh,
                             const float highPassUpperThresh,
                             std::function<void(const float)> callback)
: _callback(callback)
, _lowerThreshSq(highPassLowerThresh*highPassLowerThresh)
, _upperThreshSq(highPassUpperThresh*highPassUpperThresh)
{
  _highPassOutput.reset(new ActiveAccel());
  _highPassFilter.reset(new HighPassFilterListener(highPassFilterCoeff, _highPassOutput));
}

void ShakeListener::InitInternal(const ActiveAccel& accel)
{
  _highPassFilter->Update(accel); // This will init the high pass filter
}

void ShakeListener::UpdateInternal(const ActiveAccel& accel)
{
  _highPassFilter->Update(accel);
  
  const float magSq = _highPassOutput->x * _highPassOutput->x +
                      _highPassOutput->y * _highPassOutput->y +
                      _highPassOutput->z * _highPassOutput->z;
  
  // Once the highPassFilter has exceeded the upperThreshold use the lowerThreshold
  // to detect smaller movements
  _shakeDetected = (_shakeDetected ?
                    magSq > _lowerThreshSq :
                    magSq > _upperThreshSq);
  
  if(_shakeDetected && _callback != nullptr)
  {
    _callback(magSq);
  }
}
  
#pragma mark ---- MovementListener ----

MovementListener::MovementListener(const float hpFilterCoeff,
                                   const float maxMovementScoreToAdd,
                                   const float movementScoreDecay,
                                   const float maxAllowedMovementScore,
                                   std::function<void(const float)> callback)
: _callback(callback)
, _maxMovementScoreToAdd(maxMovementScoreToAdd)
, _movementScoreDecay(movementScoreDecay)
, _maxAllowedMovementScore(maxAllowedMovementScore)
{
  _highPassOutput.reset(new ActiveAccel());
  _highPassFilter.reset(new HighPassFilterListener(hpFilterCoeff, _highPassOutput));
}

void MovementListener::InitInternal(const ActiveAccel& accel)
{
  _highPassFilter->Update(accel); // This will init the high pass filter
}

void MovementListener::UpdateInternal(const ActiveAccel& accel)
{
  _highPassFilter->Update(accel);
  
  const float magSq = _highPassOutput->x * _highPassOutput->x +
                      _highPassOutput->y * _highPassOutput->y +
                      _highPassOutput->z * _highPassOutput->z;
  
  _movementScore += std::min(_maxMovementScoreToAdd, magSq);
  
  if(_movementScore > _movementScoreDecay)
  {
    _movementScore -= _movementScoreDecay;
  }
  else
  {
    _movementScore = 0;
  }
  
  _movementScore = std::min(_maxAllowedMovementScore, _movementScore);
  
  if(_movementScore > 0 && _callback != nullptr)
  {
    _callback(_movementScore);
  }
}

#pragma mark ---- RotationListener ----

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
