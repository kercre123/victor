/**
 * File: imuFilter.h
 *
 * Author: Michael Willett
 * Created: 1/7/2019
 *
 * Description:
 *
 *   UKF implementation for orientation and gyro bias tracking
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef IMU_UKF_H_
#define IMU_UKF_H_

#include "clad/types/imu.h"
#include "coretech/common/shared/radians.h"
#include "coretech/common/shared/types.h"

#include "coretech/common/engine/math/rotation.h"
#include "coretech/common/engine/robotTimeStamp.h"


using AngularVelocity = Anki::Point3f;
using ErrorVector = Anki::Point3f;

namespace Anki {
namespace Vector {

class ImuUKF {
public:


  ImuUKF();
  
  // Measurement Model Update
  void Update(const Point3f& accelerometer, const Point3f& gyro, RobotTimeStamp_t t);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // f32 GetYaw();     // turning left is positive
  // f32 GetPitch();   // turning down is positive
  // f32 GetRoll();    // rolling right is positive

  // Get pointer to array of gyro biases
  // inline const f32* GetGyroBias() { return _bias; }

  // Rotation speed in rad/sec
  // inline f32 GetYawSpeed() { return _state[6]; }

  // rotation quaternion plus rotational velocity: [q0,q1,q2,q3,⍵1,⍵2,⍵3]
  struct State {
    UnitQuaternion  rotation;
    AngularVelocity velocity; 
  };

  struct Noise {
    ErrorVector rotationErr;
    ErrorVector velocityErr;

    Point<6,float> ToPoint() const;
  };

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // UKF Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // after applying a measurement or Process update, Sigma {𝑋ᵢ} points are transformed in a non-linear
  // manor. This process resamples the {𝑋ᵢ} from their current distribution
  void ResetSigmaPoints();

  // update predicted state to current timestamp and increase uncertainty
  void ProcessUpdate(float dt_s);

  // update state according to measurement and decrease uncertainty
  void MeasurementUpdate(const Point<6,float>& measurement);

  // given a set of State vectors, calculate the mean state
  State CalculateMean(const std::array<State,12>& Y_);

  // given a set of State vectors and a mean, recalculate covariance
  void CalculateProcessNoise(const State& mean);

  void CalculateMeasurmentNoise();
  Noise GetMeanCenteredMeasurement();


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // UKF Variables
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  State _state;              
  // f32 _bias[3];               // gyro-bias for roll, pitch, and yaw respectively

  SmallSquareMatrix<6,float> _P;      // current state covariance matrix
  SmallSquareMatrix<6,float> _R;      // measurement process uncertainty
  SmallSquareMatrix<6,float> _Q;      // process update uncertainty

  std::array<Noise,12> _W, _Z;
  std::array<State,12> _X, _Y;

  RobotTimeStamp_t _lastMeasurement_ms;    // time of last measurement update
      
};

} // namespace Vector
} // namespace Anki

#endif // IMU_FILTER_H_
