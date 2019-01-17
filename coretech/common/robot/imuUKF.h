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

#include "coretech/common/engine/math/rotation.h"
#include "coretech/common/engine/robotTimeStamp.h"


namespace Anki {

class ImuUKF {
public:

  ImuUKF();
  
  // Measurement Model Update
  void Update(const Point3f& accel, const Point3f& gyro, unsigned int t);

  void Reset(const Rotation3d& rot);

  inline Rotation3d GetRotation() const { return _state.rotation; }

  // rotation quaternion plus rotational velocity: [q0,q1,q2,q3,⍵1,⍵2,⍵3]
  struct State {
    Rotation3d rotation = UnitQuaternion();
    Point3f    velocity = {0., 0., 0.}; 
  };

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // UKF Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // update predicted state to current timestamp and increase uncertainty
  void ProcessUpdate(float dt_s);

  // update state according to measurement and decrease uncertainty
  void MeasurementUpdate(const Point<6,float>& measurement);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // UKF Variables
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  State _state;                   // current state estimate
  SmallSquareMatrix<6,float> _P;  // current state covariance matrix

  SmallMatrix<6,12,float> _W;     // a set of points representing covariance of the state 
  std::array<State,12> _Y;        // a set of points representing the average state

  unsigned int _lastMeasurement_ms;    // time of last measurement update
      
};

} // namespace Anki

#endif // IMU_FILTER_H_
