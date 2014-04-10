/**
 * File: velocityProfileGenerator.cpp
 * Author: Kevin Yoon (kevin)
 * Date: 11/14/2013
 *
 **/

#include <assert.h>
#include <math.h>
#include "anki/common/shared/velocityProfileGenerator.h"
#include "anki/common/constantsAndMacros.h"

#define DEBUG_VPG 0
#if(DEBUG_VPG && defined(SIMULATOR))
#include <stdio.h>
#endif

namespace Anki {
 
  VelocityProfileGenerator::VelocityProfileGenerator()
  {
    targetReached_ = true;
  }
  
  void VelocityProfileGenerator::StartProfile(float startVel, float startPos,
                                              float maxSpeed, float accel,
                                              float endVel, float endPos,
                                              float timeStep)
  {
    startVel_ = startVel;
    startPos_ = startPos;
    maxVel_ = fabs(maxSpeed);
    accel_ = fabs(accel);
    endVel_ = fabs(endVel);
    endPos_ = endPos;
    timeStep_ = fabs(timeStep);

    assert(maxVel_ >= fabs(endVel_));
    assert(accel_ > 0);
    assert(timeStep_ > 0);
    
    // Compute direction, maxVel, and endVel based on startPos and endPos
    direction_ = 1;
    if (startPos > endPos) {
      direction_ = -1;
      maxVel_ *= -1;
      endVel_ *= -1;
    }
    maxReachableVel_ = maxVel_;
    
    
    // Compute the distance that would be travelled if we went from
    // startVel to maxSpeed and then immediately to terminalVel
    // Calculate as two back-to-back trapezoids:
    // Trap1: h=(Vm-Vs)/a, b1=Vm, b2=Vs
    // Trap2: h=(Vm-Vf)/a, b1=Vm, b2=Vf
    // Distance: d
    // Max speed: Vm
    // End (final) speed: Vf
    // Start speed: Vs
    // Acceleration: a
    // d = ( 2*(Vm)^2 - (Vf)^2 - (Vs)^2 ) / (2 * a)
    float d = ( (2*maxVel_*maxVel_) - (endVel_*endVel_) - (startVel_*startVel_) ) / (2 * accel_);
    
    
    // If d is more than the distance that needs to be traversed then
    // know we can't reach maxVel before we need to decelerate.
    // Compute the max speed that actually can be reached.
    // By manipulating the formula above we get:
    //
    // V_max' = sqrt( (2 * a * d' + (V_f)^2 + (V_s)^2) / 2 )
    // where d' = ABS(laneChangePixelsRequired)
    // and V_max' = max speed that can be reached
    totalDistToTarget_ = endPos_ - startPos_;
    if (fabs(d) > fabs(totalDistToTarget_)) {
      maxReachableVel_ = sqrt( ((2*accel*fabs(totalDistToTarget_)) + (endVel_*endVel_) + (startVel_*startVel_)) * 0.5f );
      maxReachableVel_ *= direction_;
#if(DEBUG_VPG)
      printf("VPG new V_max: %f (d = %f)\n", maxReachableVel_, d);
#endif
    }
    
    // Compute distance required to decelerate from maxReachableVel_ (Vs) to endVel_ (Vf).
    // d = 1/2 a*t^2 + Vs*t , where t=(Vf-Vs)/a
    // But a is actually negative because we're decelering so t=(Vs-Vf)/a and
    // d = 1/2 * -a * ((Vs - Vf)/a)^2 + Vs * (Vs - Vf)/a
    // d = (Vs*Vs - Vf*Vf)/(2a)
    decelDistToTarget_ = 0;
    if (fabs(maxReachableVel_) > fabs(endVel_)) {
      decelDistToTarget_ = (maxReachableVel_*maxReachableVel_ - endVel_*endVel_) / (2*accel_);
    }
    
    // Compute change in velocity per timestep
    deltaVelPerTimeStep_ = accel_ * timeStep_ * direction_;
    
    
    // Init state vars
    targetReached_ = false;
    isDecel_ = false;
    currVel_ = startVel_;
    currPos_ = startPos_;
    currTime_ = 0;
    
#if(DEBUG_VPG)
    printf("VPG: startVel %f, startPos %f, maxSpeed %f, accel %f\n", startVel_, startPos_, maxVel_, accel_);
    printf("     endVel %f, endPos %f, timestep %f\n", endVel_, endPos_, timeStep_);
    printf("     deltaVel %f, maxReachableVel %f, totalDist %f, decelDistToTarget %f, dir %f\n", deltaVelPerTimeStep_, maxReachableVel_, totalDistToTarget_, decelDistToTarget_, direction_);
#endif
  }
  
  
  float VelocityProfileGenerator::Step(float &currVel, float &currPos)
  {
    currTime_ += timeStep_;
    
    // If target already reached, then return final position.
    if (targetReached_) {
      currVel = endVel_;
      currPos = endPos_;
      return currTime_;
    }
    
    // Update current velocity
    if (isDecel_) {
      
      // If we're not already at endVel_...
      if (currVel_ != endVel_) {
        if (direction_ > 0) {
          currVel_ = MAX(endVel_, currVel_ - deltaVelPerTimeStep_);
        } else {
          currVel_ = MIN(endVel_, currVel_ - deltaVelPerTimeStep_);
        }
      }
      
    } else {
      // If we're not already at maxReachableVel_...
      if (currVel_ != maxReachableVel_) {
        if (direction_ > 0) {
          currVel_ = MIN(maxReachableVel_, currVel_ + deltaVelPerTimeStep_);
        } else {
          currVel_ = MAX(maxReachableVel_, currVel_ + deltaVelPerTimeStep_);
        }
      }
    }
    
    // Update position
    currPos_ += currVel_ * timeStep_;
    float currDistToTarget = endPos_ - currPos_;
    
    // Check whether or not deceleration should begin
    if (fabs(currDistToTarget) < decelDistToTarget_) {
      isDecel_ = true;
    }
    
    // Check whether or not final position has been reached
    if ( currDistToTarget * totalDistToTarget_ < 0 ) {
      targetReached_ = true;
      currVel_ = endVel_;
      currPos_ = endPos_;
    }
    
    
#if(DEBUG_VPG)
    printf("VPG::Step() - currVel %f, currPos %f, currDistToTarget %f, isDecel %d\n", currVel_, currPos_, currDistToTarget, isDecel_);
#endif
    
    
    currVel = currVel_;
    currPos = currPos_;
    return currTime_;
  }
  
} // namespace Anki


