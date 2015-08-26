/**
 * File: velocityProfileGenerator.cpp
 * Author: Kevin Yoon (kevin)
 * Date: 11/14/2013
 *
 **/

#include <assert.h>
#include <math.h>
#include "anki/common/shared/utilities_shared.h"
#include "anki/common/shared/velocityProfileGenerator.h"
#include "anki/common/constantsAndMacros.h"

#include <stdio.h>

#define DEBUG_VPG 0

// For enabling DumpProfileToFile().
// For testing purposes only.
#define ENABLE_PROFILE_DUMP 0
#if(ENABLE_PROFILE_DUMP)
#include <iostream>
#include <fstream>
#endif

namespace Anki {
 
  VelocityProfileGenerator::VelocityProfileGenerator()
  {
    targetReached_ = true;
  }

  void VelocityProfileGenerator::StartProfile(float startVel,
                                              float startPos,
                                              float endVel,
                                              float accel,
                                              float timeStep)
  {
    noTargetMode_ = true;
    
    startVel_ = startVel;
    currVel_ = startVel;
    currPos_ = startPos;
    endVel_ = endVel;
    maxReachableVel_ = endVel;
    accel_ = fabs(accel);
    timeStep_ = fabs(timeStep);
    
    // Compute change in velocity per timestep
    deltaVelPerTimeStepStart_ = accel_ * timeStep_ * (endVel_ - startVel_ > 0 ? 1.f : -1.f);
    
    targetReached_ = false;
    
    
    // Set values that don't really apply in noTargetMode to 0, even though 0 is wrong,
    // but at least they're not random values from the last time StartProfile was called.
    totalDistToTarget_ = 0;
    startAccelDist_ = 0;
    endAccelDist_ = 0;
    deltaVelPerTimeStepEnd_ = 0;
  }
  
  void VelocityProfileGenerator::StartProfile(float startVel, float startPos,
                                              float maxSpeed, float accel,
                                              float endVel, float endPos,
                                              float timeStep)
  {
    noTargetMode_ = false;
    
    startVel_ = startVel;
    startPos_ = startPos;
    maxVel_ = fabs(maxSpeed);
    accel_ = fabs(accel);
    endVel_ = fabs(endVel);
    endPos_ = endPos;
    timeStep_ = fabs(timeStep);

    assert(maxVel_ >= endVel_);
    assert(accel_ > 0);
    assert(timeStep_ > 0);
    
    // Compute direction based on startPos and endPos
    // and use it to determine sign of maxVel and endVel.
    float direction = 1;
    if (startPos > endPos) {
      direction = -1;
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
      maxReachableVel_ *= direction;
#if(DEBUG_VPG)
      CoreTechPrint("VPG new V_max: %f (d = %f)\n", maxReachableVel_, d);
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
    
    
    // Compute whether or not initial "acceleration" phase should actually
    // accelerate or decelerate.
    float startAccDirection = 1;
    if (startVel_ > maxReachableVel_) {
      startAccDirection = -1;
    }

    // Do same for "deceleration" phase
    float endAccDirection = -1;
    if (endVel_ > maxReachableVel_) {
      endAccDirection = 1;
    }
    
    // Compute change in velocity per timestep
    deltaVelPerTimeStepStart_ = accel_ * timeStep_ * startAccDirection;
    deltaVelPerTimeStepEnd_ = ABS(deltaVelPerTimeStepStart_) * endAccDirection;
    
    // Init state vars
    targetReached_ = false;
    isDecel_ = false;
    currVel_ = startVel_;
    currPos_ = startPos_;
    currTime_ = 0;
    
    startAccelDist_ = fabsf(0.5f * (maxReachableVel_ - startVel_) / deltaVelPerTimeStepStart_ * timeStep_) * (maxReachableVel_ >= 0 ? 1 : -1);
    endAccelDist_ = fabsf(0.5f * (endVel_ - maxReachableVel_) / deltaVelPerTimeStepEnd_ * timeStep_) * (maxReachableVel_ >= 0 ? 1 : -1);
    
#if(DEBUG_VPG)
    CoreTechPrint("VPG: startVel %f, startPos %f, maxSpeed %f, accel %f\n", startVel_, startPos_, maxVel_, accel_);
    CoreTechPrint("     endVel %f, endPos %f, timestep %f\n", endVel_, endPos_, timeStep_);
    CoreTechPrint("     deltaVel %f, maxReachableVel %f, totalDist %f, decelDistToTarget %f, dir %f\n", deltaVelPerTimeStepStart_, maxReachableVel_, totalDistToTarget_, decelDistToTarget_, direction);
#endif
  }
  
/*
  // sign of acc_start and acc_end matter!
  bool VelocityProfileGenerator::StartProfile_fixedDuration(float pos_start, float vel_start, float acc_start,
                                                            float pos_end, float vel_end, float acc_end,
                                                            float duration,
                                                            float timeStep)
  {
    // Compute distance to traverse
    float dist = pos_end - pos_start;
    
    // The total duration is composed of three time chunks:
    // ts: time of acceleration from vel_start to vel_mid at rate of acc_start
    // tm: time of constant motion at vel_mid
    // te: time of acceleration from vel_mid to vel_end at rate of acc_end
    //
    // Thus, duration = ts + tm + te                                              (1)
    //
    // We also know that
    //    vel_mid = Vs + (As * ts) == Ve - (Ae * te)
    //         ts = (Ve - Vs - (Ae * te))/As
    //         ts = (Ve - Vs)/As - (Ae/As) * te                                   (2)
    //
    // and that for the case where vel_start, vel_mid, and vel_end all have the same sign
    //    dist =   Vs*ts + (As * ts^2)/2     (first acceleration component)
    //           + (Ve - (Ae * te)) * tm     (constant vel component)
    //           + (Ve*te - (Ae * te^2)/2    (last deceleration component)        (3)
    //
    // From (2), let ts = m*te + b, where m = -(Ae/As) and b = (Ve - Vs)/As
    // From (1), we can say tm = duration - ts - te = duration - (m+1)*te - b
    // Substituting these into (3) we can solve for te
    
    
    float m = -acc_end / acc_start;
    float b = (vel_end - vel_start)/acc_start;

    
    // First check that vel_start, vel_mid, and vel_end all have the same sign.
    //    if (vel_start > 0 && acc_start > 0)
    
      
    
    // Let the final quadratic equation be of the form A*te^2 + B*te + C = 0
    
    float A = -(acc_end*acc_end)/(2*acc_start) + (0.5*acc_end);
    float B = (-2*vel_start/acc_start + b-duration)*acc_end;
    float C = vel_start*b + 0.5*acc_start*b*b + vel_end*duration - vel_end*b - dist;
    
    float discr = B*B - (4*A*C);
    if (discr < 0) {
      return false;
    }
    
    float sqrDisc = sqrtf(discr);
    float te = (-B + sqrDisc)/(2*A);
    float ts = te * m + b;
    
    if (te + ts > duration) {
      te = (-B - sqrDisc)/(2*A);
      ts = te * m + b;
    }
    
    maxReachableVel_ = vel_start + acc_start * ts;
    deltaVelPerTimeStepStart_ = acc_start * timeStep;
    deltaVelPerTimeStepEnd_ = acc_end * timeStep;
    
    
    printf("ts: %f, te: %f\n", ts, te);
    
    return true;
  }
*/  

  
  // There are 3 types of profiles, and their mirror images across the time axis, that this
  // can generate:
  //
  // 1) vel_start and vel_mid are of the same sign, vel_mid is of greater magnitude than vel_start.
  // 2) vel_start and vel_mid are of the same sign, vel_mid is of lesser magnitude than vel_start.
  // 3) vel_start and vel_mid are of opposite signs.
  bool VelocityProfileGenerator::StartProfile_fixedDuration(float pos_start, float vel_start, float acc_start_duration,
                                                            float pos_end, float acc_end_duration,
                                                            float vel_max,
                                                            float acc_max,
                                                            float duration,
                                                            float timeStep)
  {
    
    noTargetMode_ = false;
    
    startVel_ = vel_start;
    startPos_ = pos_start;
    maxVel_ = fabsf(vel_max);
    endVel_ = 0;
    endPos_ = pos_end;
    timeStep_ = fabs(timeStep);

    acc_max = fabsf(acc_max);
    vel_max = fabsf(vel_max);
    
    // Check that the acceleration durations are valid given the total duration
    if (acc_start_duration + acc_end_duration > duration) {
      CoreTechPrint("VPG FAIL: acc_start_duration + acc_end_duration exceeds total duration (%f + %f > %f)\n", acc_start_duration, acc_end_duration, duration);
      return false;
    }
    
    float ts = acc_start_duration;
    float te = acc_end_duration;
    float tm = duration - ts - te;
    
    // Compute distance to traverse
    float dist = pos_end - pos_start;
    
    // Figure out if vel_mid is +ve or -ve by setting it to 0 and seeing what distance is covered
    float distWhenVelMid0 = 0.5f*(vel_start * acc_start_duration);
    bool isVelMidGT0 = (dist >= distWhenVelMid0);
    
    // If accelerating to a vel_mid of 0 for the acc_start phase actually passes the end_pos,
    // then return false as this is probably not an intended profile.
    if (((dist > 0) == (distWhenVelMid0 > 0)) && (fabsf(distWhenVelMid0) > fabsf(dist))) {
      CoreTechPrint("VPG FAIL: end_pos reached during starting acc phase. Consider reducing acc_start duration or decreasing vel_start magnitude.\n");
      return false;
    }
    
    
    float distWhenVelMidIsVelStart = vel_start * (duration - acc_end_duration) + 0.5f*(vel_start * acc_end_duration);
    bool isVelMidGTVelStart = (dist >= distWhenVelMidIsVelStart);
    
    bool accStartCrosses0 = (vel_start < 0.f) == isVelMidGT0;
    
    float vs = vel_start;
    float vm;
   
    bool flipped = vs < 0;
    if (flipped) {
      vs = -vs;
      dist = -dist;
      isVelMidGT0 = !isVelMidGT0;
      isVelMidGTVelStart = !isVelMidGTVelStart;
    }
   
    // Compute vel_mid depending on which case we're dealing with
    if (accStartCrosses0) {
      // Case 3
      float A = -(0.5f*ts + tm + 0.5f*te);
      float B = vs*tm + 0.5f*vs*te + dist;
      float C = 0.5f*vs*vs*ts - vs*dist;
      float discr = B*B - 4*A*C;
      
      if (discr < 0){
        CoreTechPrint("WARNING: discr < 0  (A = %f, B = %f, C = %f)\n", A, B, C);
        return false;
      }
      
      float sqrtDiscr = sqrtf(discr);
      vm = (-B + sqrtDiscr) / (2*A);
      
      if (vm > 0) {
        // vm should be negative
        CoreTechPrint("WARNING: discr < 0  (A = %f, B = %f, C = %f)\n", A, B, C);
        return false;
      }
      
    } else {
      // Cases 1 and 2
      vm = (dist - (0.5f*vs*ts)) / (0.5f*ts + tm + 0.5f*te);
    }
    
    
    // Check if any velocity exceeds the max
    if ( (fabsf(vs) > vel_max) || (fabsf(vm) > vel_max)) {
      CoreTechPrint("WARNING: vs = %f, vm = %f, vel_max = %f\n", vs, vm, vel_max);
      return false;
    }
    
    // Check if any acceleration exceeds the max allowed
    float acc_start = fabsf(vm-vs)/ts;
    float acc_end = fabsf(vm/te);
    if ((acc_start > acc_max) || (acc_end > acc_max)) {
      CoreTechPrint("WARNING: acc_start = %f, acc_end = %f, acc_max = %f\n", acc_start, acc_end, acc_max);
      return false;
    }
    
    
    if (flipped) {
      vs = -vs;
      dist = -dist;
      isVelMidGT0 = !isVelMidGT0;
      isVelMidGTVelStart = !isVelMidGTVelStart;
      vm = -vm;
    }
    
    totalDistToTarget_ = dist;
    maxReachableVel_ = vm;
    deltaVelPerTimeStepStart_ = ((vm-vs)/ts) * timeStep;
    deltaVelPerTimeStepEnd_ = (-vm/te) * timeStep;
    decelDistToTarget_ = fabsf(0.5f*vm*te);
    
    // Init state vars
    targetReached_ = false;
    isDecel_ = false;
    currVel_ = vs;
    currPos_ = pos_start;
    currTime_ = 0;

    startAccelDist_ = fabsf(0.5f * (maxReachableVel_ - startVel_) * ts) * (maxReachableVel_ >= 0 ? 1 : -1);
    endAccelDist_ = fabsf(0.5f * (endVel_ - maxReachableVel_) * te) * (maxReachableVel_ >= 0 ? 1 : -1);

    
#if(DEBUG_VPG)
    CoreTechPrint("VPG: startVel %f, startPos %f, endVel %f, endPos %f\n", startVel_, startPos_, endVel_, endPos_);
    CoreTechPrint("     ts %f, tm %f, te %f, total duration %f\n", ts, tm, te, duration);
    CoreTechPrint("     deltaVelStart %f, deltaVelEnd %f, maxReachableVel %f\n", deltaVelPerTimeStepStart_, deltaVelPerTimeStepEnd_, maxReachableVel_);
    CoreTechPrint("     totalDist %f, decelDistToTarget %f\n", totalDistToTarget_, decelDistToTarget_);
#endif
    
    return true;
  }  
  
  float VelocityProfileGenerator::Step(float &currVel, float &currPos)
  {
    currTime_ += timeStep_;
    
    // If in noTargetMode, the logic is much simpler
    if (noTargetMode_) {
      currPos_ += currVel_ * timeStep_;
      
      // If endVel not yet reached then update velocity
      if (currVel_ != endVel_) {
        currVel_ += deltaVelPerTimeStepStart_;
        if ((maxReachableVel_ > endVel_) != (currVel_ > endVel_)) {
          currVel_ = endVel_;
        }
      }
      
      currVel = currVel_;
      currPos = currPos_;
      return currTime_;
    }
    
    
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
        currVel_ += deltaVelPerTimeStepEnd_;
        if ((maxReachableVel_ > endVel_) != (currVel_ > endVel_)) {
          currVel_ = endVel_;
        }
      }
      
    } else {
      // If we're not already at maxReachableVel_...
      if (currVel_ != maxReachableVel_) {
        currVel_ += deltaVelPerTimeStepStart_;
        if ((startVel_ > maxReachableVel_) != (currVel_ > maxReachableVel_)) {
          currVel_ = maxReachableVel_;
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
    if ( ((currDistToTarget > 0) != (totalDistToTarget_ > 0)) || (isDecel_ && (currVel_ == endVel_)) ) {
      targetReached_ = true;
      currVel_ = endVel_;
      currPos_ = endPos_;
    }
    
    
#if(DEBUG_VPG)
    CoreTechPrint("VPG::Step() - currVel %f, currPos %f, currDistToTarget %f, isDecel %d\n", currVel_, currPos_, currDistToTarget, isDecel_);
#endif
    
    
    currVel = currVel_;
    currPos = currPos_;
    return currTime_;
  }
  


  void VelocityProfileGenerator::DumpProfileToFile(const char* filename)
  {
#if(ENABLE_PROFILE_DUMP)
    float currVel, currPos;

    std::ofstream csvFile;
    csvFile.open(filename);
    csvFile << "Time, Position, Velocity\n";
    while (!targetReached_) {
      csvFile << currTime_ << ", " << currPos_ << ", " << currVel_ << "\n";
      Step(currVel, currPos);
    }
    csvFile << currTime_ << ", " << currPos_ << ", " << currVel_ << std::endl;
    csvFile.close();
#endif
  }
  
  
  bool VelocityProfileGenerator::TargetReached()
  {
    return targetReached_;
  }

} // namespace Anki


