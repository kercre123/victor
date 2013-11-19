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
    
    
    // Returns the current time.
    // currVel: Current velocity
    // currPos: Current position
    //
    // TODO: Assuming that timestep is small enough that we don't need to care about
    //       changing speed in the middle of a timestep.
    float Step(float &currVel, float &currPos);
    
    
  private:
    // Profile parameters
    float startVel_;
    float startPos_;
    float maxVel_;
    float accel_;
    float endVel_;
    float endPos_;
    float timeStep_;
  
    // Change in velocity per time stemp
    float deltaVelPerTimeStep_;
    
    // Current time in the profile
    float currTime_;
    
    // Current distance to target
    float totalDistToTarget_;
    
    // At what distance from the target we should start decelerating to endVel
    float decelDistToTarget_;
    
    // Max reachable speed (which may not be as high as the desired maxSpeed)
    float maxReachableVel_;
  
    // +1 if approaches maxReachableVel_ from -ve side
    // -1 if approaches maxReachableVel_ from +ve side
    float direction_;
    
    // Whether or not it's decelerating to endVel_
    bool isDecel_;
    
    // Whether or not the target has been reached
    bool targetReached_;
    
    // Current velocity
    float currVel_;
    
    // Current position
    float currPos_;
    
  };
} // namespace Anki

#endif
