/**
 * File: imuUKF.h
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

#ifndef _IMU_UKF_H_
#define _IMU_UKF_H_

#include "coretech/common/shared/math/rotation.h"

namespace Anki {

class ImuUKF {
public:

  // Constructor
  ImuUKF();
  
  // Measurement Model Update
  void Update(const Point<3,double>& accel, const Point<3,double>& gyro, float timestamp_s, bool isMoving = true);

  // Clear current uncertainty and reset the orientation to the specified rotation
  void Reset(const Rotation3d& rot);

  // Public Accessors
  inline Rotation3d GetRotation()      const { return _state.GetRotation(); }
  inline Point3f    GetRotationSpeed() const { return _state.GetVelocity().CastTo<float>() - _state.GetGyroBias().CastTo<float>(); }
  inline Point3f    GetBias()          const { return _state.GetGyroBias().CastTo<float>(); }

private:

  // rotation quaternion, rotational velocity, and gyro bias: 
  //   [q0,q1,q2,q3,⍵1,⍵2,⍵3,b0,b2,b2]
  class State : public Point<10,double> {
  public:
    State() : Point<10,double>{1.,0.,0.,0.,0.,0.,0.,0.,0.,0.} {}
    State(Point<10,double>&& p) : Point<10,double>(p) {}

    State(const UnitQuaternion& q, const Point<3,double>& w, const Point<3,double> b) 
    : Point<10,double>( Join(Join(q,w),b) ) {}

    UnitQuaternion GetRotation() const { return this->Slice<0,3>(); }
    Point3<double> GetVelocity() const { return this->Slice<4,6>(); }
    Point3<double> GetGyroBias() const { return this->Slice<7,9>(); }

    static constexpr size_t Dim = 10;
  };
    
  class Error : public Point<9,double> {
  public:
    Error(Point<9,double>&& p) : Point<9,double>(p) {}
    Point3<double> GetRotation() const { return this->Slice<0,2>(); }
    Point3<double> GetVelocity() const { return this->Slice<3,5>(); }
    Point3<double> GetGyroBias() const { return this->Slice<6,8>(); }

    // Dim is 1 less than the State, since the rotation quaternion only has 3 DoF (4 Dims and 1 constraint)
    static constexpr size_t Dim = 9;
  };

  // Update predicted state to current timestamp and increase uncertainty
  void ProcessUpdate(double dt_s);

  // Calculates the residual according to the provided measurement
  Point<9,double> MeasurementUpdate(const Point<9,double>& measurement);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Variables
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  State                                         _state;      // current state estimate
  SmallSquareMatrix<Error::Dim, double>         _P;          // current state covariance matrix
  SmallMatrix<State::Dim, Error::Dim*2, double> _Y;          // a discrete set of points representing the state estimate
  SmallMatrix<Error::Dim, Error::Dim*2, double> _W;          // a discrete set of points representing covariance of the state 

  static const SmallSquareMatrix<Error::Dim,double> _Q, _R;   // process and measurement uncertainty
  float _lastMeasurement_s;                                   // time of last measurement update for calculating predicted process update
};

} // namespace Anki

#endif // IMU_FILTER_H_
