/**
 * File: radiansMath.h
 *
 * Author: Kevin Yoon
 * Created: 3/23/2018
 * 
 * Description: Additional utilities for doing math on Radians
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef _ANKICORETECH_COMMON_RADIANS_MATH_H_
#define _ANKICORETECH_COMMON_RADIANS_MATH_H_

#include "coretech/common/shared/radians.h"
#include <vector>

namespace Anki {
namespace RadiansMath {
  
  // meanAngle: The compute mean of all angles in angles
  // Returns true as long angles is non-empty
  bool ComputeMean(const std::vector<Radians>& angles, Radians& meanAngle);
  
  // meanAngle: Mean of all the angles in angles. (Same as what ComputeMean() returns)
  // meanToMinAngleDist_rad: Angular distance to furthest angle (up to -PI) from the mean in the CW direction
  // meanToMaxAngleDist_rad: Angular distance to furthest angle (up to PI) from the mean in the CCW direction
  bool ComputeMinMaxMean(const std::vector<Radians>& angles,
                         Radians& meanAngle,
                         float& meanToMinAngleDist_rad,
                         float& meanToMaxAngleDist_rad);

} // namespace RadiansMath
} // namespace Anki

#endif // _ANKICORETECH_COMMON_RADIANS_MATH_H_

