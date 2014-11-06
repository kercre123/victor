#include "gtest/gtest.h"

#include "anki/common/shared/velocityProfileGenerator.h"
#include <iostream>

// For test debug printing
#define DEBUG_TEST_VPG


#define ASSERT_NEAR_EQ(a,b) ASSERT_NEAR(a,b,FLOATING_POINT_COMPARISON_TOLERANCE)

using namespace std;
using namespace Anki;


GTEST_TEST(TestVPG, TestNormal_PositiveRamp)
{
  VelocityProfileGenerator vpg;

  float startVel = 0;
  float startPos = 0;
  float maxSpeed = 1;
  float accel = 1;
  float endVel = 0.1;
  float endPos = 10;
  float timestep = 0.005;
  
  vpg.StartProfile(startVel, startPos, maxSpeed, accel, endVel, endPos, timestep);
  
  float currVel, currPos;
  while(!vpg.TargetReached()) {
    vpg.Step(currVel, currPos);
    ASSERT_LE(currVel, maxSpeed);
  }
  
  ASSERT_FLOAT_EQ(currPos, endPos);
}



GTEST_TEST(TestVPG, TestFixedDuration_Trapezoid)
{
  VelocityProfileGenerator vpg;
  
  float startPos = 0;
  float startVel = 0;
  float startAccDurationFraction = 0.25;
  float endPos = 10;
  float endAccDurationFraction = 0.25;
  float duration = 10;
  float timestep = 0.005;
  
  float maxVel = 10;
  float maxAcc = 1000;
  
  float stepCountAccuracyThresh = 0.01;
  
  // Positive trapezoid
  bool res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                            endPos, endAccDurationFraction*duration,
                                                            maxVel, maxAcc,
                                                            duration, timestep);
  ASSERT_TRUE(res);
  
  // Verify steps
  int expectedNumStepsToTarget = duration / timestep;
  int numSteps = 0;
  float currPos, currVel;
  
  while(!vpg.TargetReached()) {
    vpg.Step(currVel, currPos);
    ++numSteps;
    ASSERT_LE(numSteps, expectedNumStepsToTarget * (1.f+stepCountAccuracyThresh));
  }
  ASSERT_GE(numSteps, expectedNumStepsToTarget * (1.f-stepCountAccuracyThresh));
  
  // Negative trapezoid
  endPos = -10;
  
  res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                       endPos, endAccDurationFraction*duration,
                                                       maxVel, maxAcc,
                                                       duration, timestep);
  ASSERT_TRUE(res);
  
  // Verify steps
  expectedNumStepsToTarget = duration / timestep;
  numSteps = 0;
  
  while(!vpg.TargetReached()) {
    vpg.Step(currVel, currPos);
    ++numSteps;
    ASSERT_LE(numSteps, expectedNumStepsToTarget * (1.f+stepCountAccuracyThresh));
  }
  ASSERT_GE(numSteps, expectedNumStepsToTarget * (1.f-stepCountAccuracyThresh));
  
}


GTEST_TEST(TestVPG, TestFixedDuration_DistanceOppositeSignOfStartVel)
{
  VelocityProfileGenerator vpg;
  
  float startPos = 0;
  float startVel = 1;
  float startAccDurationFraction = 0.25;
  float endPos = -10;
  float endAccDurationFraction = 0.25;
  float duration = 10;
  float timestep = 0.005;

  float maxVel = 10;
  float maxAcc = 1000;
  
  float stepCountAccuracyThresh = 0.01;

  // Start with positive velocity, target in negative direction
  bool res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                            endPos, endAccDurationFraction*duration,
                                                            maxVel, maxAcc,
                                                            duration, timestep);
  ASSERT_TRUE(res);

  // Verify steps
  int expectedNumStepsToTarget = duration / timestep;
  int numSteps = 0;
  float currPos, currVel;
  
  while(!vpg.TargetReached()) {
    vpg.Step(currVel, currPos);
    ++numSteps;
    ASSERT_LE(numSteps, expectedNumStepsToTarget * (1.f+stepCountAccuracyThresh));
  }
  ASSERT_GE(numSteps, expectedNumStepsToTarget * (1.f-stepCountAccuracyThresh));

  
  
  // Start with negative velocity, target in positive direction
  startVel = -1;
  endPos = 10;
  res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                            endPos, endAccDurationFraction*duration,
                                                            maxVel, maxAcc,
                                                            duration, timestep);
  ASSERT_TRUE(res);
  
  // Verify steps
  expectedNumStepsToTarget = duration / timestep;
  numSteps = 0;
  
  while(!vpg.TargetReached()) {
    vpg.Step(currVel, currPos);
    ++numSteps;
    ASSERT_LE(numSteps, expectedNumStepsToTarget * (1.f+stepCountAccuracyThresh));
  }
  ASSERT_GE(numSteps, expectedNumStepsToTarget * (1.f-stepCountAccuracyThresh));
  
}


GTEST_TEST(TestVPG, TestFixedDuration_StartVelApproachesTargetAndDecelRequired)
{
  VelocityProfileGenerator vpg;
  
  float startPos = 0;
  float startVel = 5;
  float startAccDurationFraction = 0.25;
  float endPos = 10;
  float endAccDurationFraction = 0.25;
  float duration = 10;
  float timestep = 0.005;
  
  float maxVel = 10;
  float maxAcc = 1000;
  
  float stepCountAccuracyThresh = 0.01;
  
  // Positive start vel
  bool res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                            endPos, endAccDurationFraction*duration,
                                                            maxVel, maxAcc,
                                                            duration, timestep);
  ASSERT_TRUE(res);
  
  // Verify steps
  int expectedNumStepsToTarget = duration / timestep;
  int numSteps = 0;
  float currPos, currVel;
  
  while(!vpg.TargetReached()) {
    vpg.Step(currVel, currPos);
    ++numSteps;
    ASSERT_LE(numSteps, expectedNumStepsToTarget * (1.f+stepCountAccuracyThresh));
  }
  ASSERT_GE(numSteps, expectedNumStepsToTarget * (1.f-stepCountAccuracyThresh));
  
  

  // Negative start vel
  startVel = -5;
  endPos = -10;
  res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                       endPos, endAccDurationFraction*duration,
                                                       maxVel, maxAcc,
                                                       duration, timestep);
  ASSERT_TRUE(res);
  
  // Verify steps
  expectedNumStepsToTarget = duration / timestep;
  numSteps = 0;
  
  while(!vpg.TargetReached()) {
    vpg.Step(currVel, currPos);
    ++numSteps;
    ASSERT_LE(numSteps, expectedNumStepsToTarget * (1.f+stepCountAccuracyThresh));
  }
  ASSERT_GE(numSteps, expectedNumStepsToTarget * (1.f-stepCountAccuracyThresh));

}


GTEST_TEST(TestVPG, TestFixedDuration_Failure_StartAccPassesThroughEndPos)
{
  VelocityProfileGenerator vpg;
  
  float startPos = 0;
  float startVel = 10;
  float startAccDurationFraction = 0.25;
  float endPos = 10;
  float endAccDurationFraction = 0.25;
  float duration = 10;
  float timestep = 0.005;
  
  float maxVel = 10;
  float maxAcc = 1000;

  
  // acc_start_duration is too long / vel_start is too high
  bool res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                            endPos, endAccDurationFraction*duration,
                                                            maxVel, maxAcc,
                                                            duration, timestep);
  ASSERT_FALSE(res);
}
  
GTEST_TEST(TestVPG, TestFixedDuration_Failure_AccPhasesTooLong)
{
  VelocityProfileGenerator vpg;
  
  float startPos = 0;
  float startVel = 10;
  float startAccDurationFraction = 0.5;
  float endPos = 10;
  float endAccDurationFraction = 0.51;
  float duration = 10;
  float timestep = 0.005;
  
  float maxVel = 10;
  float maxAcc = 1000;
  
  bool res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                            endPos, endAccDurationFraction*duration,
                                                            maxVel, maxAcc,
                                                            duration, timestep);
  ASSERT_FALSE(res);
}
  
GTEST_TEST(TestVPG, TestFixedDuration_VelMaxExceeded)
{
  VelocityProfileGenerator vpg;
  
  float startPos = 0;
  float startVel = 0;
  float startAccDurationFraction = 0.25;
  float endPos = 100;
  float endAccDurationFraction = 0.25;
  float duration = 1;
  float timestep = 0.005;
  
  float maxVel = 10;
  float maxAcc = 1000;

  bool res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                       endPos, endAccDurationFraction*duration,
                                                       maxVel, maxAcc,
                                                       duration, timestep);
  ASSERT_FALSE(res);
}

GTEST_TEST(TestVPG, TestFixedDuration_AccMaxExceeded)
{
  VelocityProfileGenerator vpg;
  
  float startPos = 0;
  float startVel = 0;
  float startAccDurationFraction = 0.001;
  float endPos = 10;
  float endAccDurationFraction = 0.25;
  float duration = 10;
  float timestep = 0.005;
  
  float maxVel = 10;
  float maxAcc = 10;
  
  bool res = vpg.StartProfile_fixedDuration(startPos, startVel, startAccDurationFraction*duration,
                                                            endPos, endAccDurationFraction*duration,
                                                            maxVel, maxAcc,
                                                            duration, timestep);
  ASSERT_FALSE(res);
}

