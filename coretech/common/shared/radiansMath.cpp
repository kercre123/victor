/**
 * File: radiansMath.cpp
 *
 * Author: Kevin Yoon
 * Created: 3/23/2018
 *
 * Description: Additional utilities for doing math on Radians
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "coretech/common/shared/radiansMath.h"
#include "util/logging/logging.h"

#include <math.h>

namespace Anki {
namespace RadiansMath {
  
// See https://en.wikipedia.org/wiki/Mean_of_circular_quantities  
bool ComputeMean(const std::vector<Radians>& angles, Radians& meanAngle)
{
  if (angles.size() == 0) {
    return false;
  }
  
  float sumOfSines = 0;
  float sumOfCosines = 0;
  for (auto& angle : angles) {
    const float angle_rad = angle.ToFloat();
    sumOfSines   += sinf(angle_rad);
    sumOfCosines += cosf(angle_rad);
  }
  
  meanAngle = Radians(atan2f(sumOfSines, sumOfCosines));
  return true;
}
 
bool ComputeMinMaxMean(const std::vector<Radians>& angles,
                       Radians& meanAngle,
                       float& meanToMinAngleDist_rad,
                       float& meanToMaxAngleDist_rad)
{
  if (!ComputeMean(angles, meanAngle)) {
    return false;
  }
  
  meanToMinAngleDist_rad = 0.f;
  meanToMaxAngleDist_rad = 0.f;
  
  for (auto& angle : angles) {
    float diff = (angle - meanAngle).ToFloat();
    if (meanToMinAngleDist_rad > diff) {
      meanToMinAngleDist_rad = diff;
    }
    if (meanToMaxAngleDist_rad < diff) {
      meanToMaxAngleDist_rad = diff;
    }
  }

  return true;
}
  
  
} // namespace RadiansMath
} // namespace Anki
