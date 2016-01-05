/**
* File: velocityProfileGenerator.h
* Author: Kevin Yoon (kevin)
* Date: 11/14/2013
*
* Description:
*   Class for returning ideal speed and velocity given start velocity, start position,
*   end velocity, end position, acceleration (same value used for deceleration),
*   maximum speed, and the update timestep.
*
*   Useful for feeding a controller a progressive target rather than just the final one.
*
**/

#ifndef VELOCITY_PROFILE_GENERATOR_H_
#define VELOCITY_PROFILE_GENERATOR_H_

namespace Anki {
  class VelocityProfileGenerator {
  public:

    VelocityProfileGenerator();

    // Starts a trajectory with the given parameters.
    // Resets current time to 0.
    //
    // startVel:  Starting velocity
    // startPos:  Starting position
    // maxSpeed:  Maximum speed permitted on trajectory
    // accel:     Acceleration (and deceleration)
    // endVel:    Final velocity when target is reached
    // endPos:    Final position
    // timeStep:  Amount of seconds that transpire when Step() is called.
    void StartProfile(float startVel, float startPos,
      float maxSpeed, float accel,
      float endVel, float endPos,
      float timeStep);
    
    // Starts a trajectory for a target velocity only. No target position.
    void StartProfile(float startVel,
                      float startPos,
                      float endVel,
                      float accel,
                      float timeStep);
    
    
    /*
    bool StartProfile_fixedDuration(float startPos, float startVel, float startAccel,
                                    float endPos, float endVel, float endAccel,
                                    float duration,
                                    float timeStep);
     */

    // Starts a velocity profile with the given parameters.
    // Assumes end velocity of 0.
    //
    //          pos_start: Starting position
    //            pos_vel: Starting velocity
    // acc_start_duration: Initial time period during which we accelerate to some velocity (i.e. vel_mid)
    //            pos_end: End position
    //   acc_end_duration: Final time period following the period in which we're coasting at vel_mid to approach a final speed of 0.
    //            vel_max: Max velocity permitted. Fails if any velocity in the profile exceeds this amount.
    //            acc_max: Max acceleration permitted. Fails if any acceleration in the profile exceeds this amount.
    //           duration: Time period that the entire traversal from pos_start to pos_end should take.
    //           timeStep: Time resolution at which subsequent calls to Step() will compute position and velocity.
    bool StartProfile_fixedDuration(float pos_start, float vel_start, float acc_start_duration,
                                    float pos_end, float acc_end_duration,
                                    float vel_max,
                                    float acc_max,
                                    float duration,
                                    float timeStep);

    
    // Returns the current time.
    // currVel: Current velocity
    // currPos: Current position
    //
    // TODO: Assuming that timestep is small enough that we don't need to care about
    //       changing speed in the middle of a timestep.
    float Step(float &currVel, float &currPos);

    void DumpProfileToFile(const char* filename);

    bool TargetReached();
    
    // Accessors that only return valid values after calling
    // StartProfile() or StartProfile_fixedDuration().
    float GetMaxReachableVel() { return maxReachableVel_; }
    float GetTotalDistToTarget() { return totalDistToTarget_; }
    float GetStartAccelDist() { return startAccelDist_; }
    float GetEndAccelDist() { return endAccelDist_; }
    float GetStartAccel() { return deltaVelPerTimeStepStart_ / timeStep_; }
    float GetEndAccel() { return deltaVelPerTimeStepEnd_ / timeStep_; }
    
    
  private:
    // Profile parameters
    float startVel_;
    float startPos_;
    float maxVel_;
    float accel_;
    float endVel_;
    float endPos_;
    float timeStep_;
    
    // If true, then TargetReached() is always false.
    bool noTargetMode_;

    // Change in velocity per time stemp
    float deltaVelPerTimeStepStart_;
    float deltaVelPerTimeStepEnd_;

    // Current time in the profile
    float currTime_;

    // Current distance to target
    float totalDistToTarget_;

    // At what distance from the target we should start decelerating to endVel
    float decelDistToTarget_;

    // Max reachable speed (which may not be as high as the desired maxSpeed)
    float maxReachableVel_;
    
    // Whether or not it's decelerating to endVel_
    bool isDecel_;

    // Whether or not the target has been reached
    bool targetReached_;

    // Current velocity
    double currVel_;

    // Current position
    double currPos_;
    
    // Distances traversed during start and end acceleration phases
    float startAccelDist_;
    float endAccelDist_;
  };
} // namespace Anki

#endif
