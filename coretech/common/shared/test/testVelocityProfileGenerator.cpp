#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "anki/common/shared/velocityProfileGenerator.h"
#include <iostream>

// Enables logging VPGs to file
#define ENABLE_VPG_LOGGING 0


#define ASSERT_NEAR_EQ(a,b) ASSERT_NEAR(a,b,FLOATING_POINT_COMPARISON_TOLERANCE)

using namespace std;
using namespace Anki;


#if(ENABLE_VPG_LOGGING)
static FILE* _vpgFile = nullptr;
#endif
void OpenVPGLog(const char* filename,
                float startPos, float endPos,
                float startVel, float maxVel, float endVel,
                float accel, float timestep,
                float startFrac, float endFrac, float duration) {
#if(ENABLE_VPG_LOGGING)
  if (_vpgFile != nullptr) {
    printf("ERROR (testVelocityProfileGenerator): Log file was not closed properly. Closing and opening new file.\n");
    fclose(_vpgFile);
  }
  
  _vpgFile = fopen(filename, "w");
  
  char buf[1024];
  int numCharWritten = sprintf(buf, "startPos, %f\n"
                                    "endPos, %f\n"
                                    "startVel, %f\n"
                                    "maxVel, %f\n"
                                    "endVel, %f\n"
                                    "accel, %f\n"
                                    "timestep, %f\n"
                                    "startFrac, %f\n"
                                    "endFrac, %f\n"
                                    "duration, %f\n\n"
                                    "time, curVel, curPos\n"  // header for subsequent data
                                    "%f, %f %f\n",
                                    startPos,
                                    endPos,
                                    startVel,
                                    maxVel,
                                    endVel,
                                    accel,
                                    timestep,
                                    startFrac,
                                    endFrac,
                                    duration,
                                    0.f, startVel, startPos);
  fwrite(buf, numCharWritten, 1, _vpgFile);
#endif
}

void LogVPGData(float time, float curVel, float curPos) {
#if(ENABLE_VPG_LOGGING)
  if (_vpgFile == nullptr) {
    printf("ERROR (testVelocityProfileGenerator): Can't write to VPG log before opening.\n");
    return;
  }
  
  char buf[64];
  int numCharWritten = sprintf(buf,  "%f, %f, %f\n", time, curVel, curPos);
  fwrite(buf, numCharWritten, 1, _vpgFile);
#endif
}


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
  
  float currVel = startVel, currPos = startPos;
  while(!vpg.TargetReached()) {
    vpg.Step(currVel, currPos);
    ASSERT_LE(currVel, maxSpeed);
  }
  
  ASSERT_FLOAT_EQ(currPos, endPos);
}


GTEST_TEST(TestVPG, TestNormal_StartVelGreaterThanMaxVel)
{
  VelocityProfileGenerator vpg;
  
  float startVel = 3;
  float startPos = 0;
  float maxVel = 1;
  float accel = 1;
  float endVel = 0.1;
  float endPos = 10;
  float timestep = 0.005;
  
  vpg.StartProfile(startVel, startPos, maxVel, accel, endVel, endPos, timestep);
  
  OpenVPGLog("TestNormal_StartVelGreaterThanMaxVel.csv",
             startPos, endPos,
             startVel, maxVel, endVel,
             accel, timestep,
             -1, -1, -1);

  
  float currVel = startVel, currPos = startPos, lastVel = startVel;
  while(!vpg.TargetReached()) {
    float currTime = vpg.Step(currVel, currPos);
    LogVPGData(currTime, currVel, currPos);
    ASSERT_LE(currVel, lastVel);
  }
  
  ASSERT_FLOAT_EQ(currPos, endPos);
}


GTEST_TEST(TestVPG, TestNormal_NoTargetMode)
{
  VelocityProfileGenerator vpg;
  
  float startVel = 1;
  float startPos = 0;
  float accel = 1;
  float endVel = 3;
  float timestep = 0.005;
  
  vpg.StartProfile(startVel, startPos, endVel, accel, timestep);
  
  OpenVPGLog("TestNormal_NoTargetMode.csv",
             startPos, -1,
             startVel, -1, endVel,
             accel, timestep,
             -1, -1, -1);
  
  
  float currVel = startVel, currPos = startPos, lastVel = startVel;
  int numSteps = 0;
  while(!vpg.TargetReached()) {
    float currTime = vpg.Step(currVel, currPos);
    LogVPGData(currTime, currVel, currPos);
    ASSERT_GE(currVel, lastVel);
    if (++numSteps == 2000) {
      break;
    }
  }
  
  ASSERT_EQ(numSteps, 2000);
  ASSERT_FLOAT_EQ(currVel, endVel);
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
  
  OpenVPGLog("TestFixedDuration_Trapezoid.csv",
             startPos, endPos,
             startVel, maxVel, 0,
             maxAcc, timestep,
             startAccDurationFraction, endAccDurationFraction, duration);
  
  while(!vpg.TargetReached()) {
    float currTime = vpg.Step(currVel, currPos);
    LogVPGData(currTime, currVel, currPos);
    
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

