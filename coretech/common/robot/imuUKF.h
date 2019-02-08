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
  void Update(const Point<3,double>& accel, const Point<3,double>& gyro, float dt_s, bool isMoving = true);

  // Clear current uncertainty and reset the orientation to the specified rotation
  void Reset(const Rotation3d& rot);

  // Public Accessors
  inline Rotation3d GetRotation()      const { return _state.GetRotation(); }
  inline Point3f    GetRotationSpeed() const { return _state.GetVelocity().CastTo<float>() - _state.GetGyroBias().CastTo<float>(); }
  inline Point3f    GetBias()          const { return _state.GetGyroBias().CastTo<float>(); }

private:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Rotation Mappings
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // To avoid singularities in Kalman Gain calculations, we need to provide a homeomorphic mapping between 
  //   quaternions {q ∈ S³ ⊂ ℝ⁴} and some vector {e ∈ ℝ³}. This implementation uses rotation vectors scaled by
  //   rotation angle for {e ∈ ℝ³}. We could also use Rodriguez Paramters or Modified Rodriguez Paramters if 
  //   better linear approximations are needed.
  
  using RotationError = Point3<double>;

  // Transforms rotation vector into a quaternion
  inline UnitQuaternion ToQuat(const RotationError& v) { 
    const double alpha = v.Length();
    const double sinAlpha = sin(alpha * .5) / (NEAR_ZERO(alpha) ? 1. : alpha);
    return {cos(alpha * .5), sinAlpha * v.x(),  sinAlpha * v.y(),  sinAlpha * v.z()};
  };

  // Transforms quaternion into a rotation vector 
  inline RotationError FromQuat(const UnitQuaternion& q) { 
    RotationError axis = q.Slice<1,3>();
    const double alpha = asin( axis.MakeUnitLength() ) * 2;
    return axis * alpha;
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Class Definitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // rotation quaternion, rotational velocity, and gyro bias: 
  //   [q0,q1,q2,q3,⍵1,⍵2,⍵3,b0,b2,b2]
  class State : public Point<10,double> {
  public:
    State() { (*this)[0] = 1.; }
    State(Point<Size,double>&& p) : Point<Size,double>(p) {}

    State(const UnitQuaternion& q, const Point<3,double>& w, const Point<3,double> b) 
    : Point<Size,double>( Join(Join(q,w),b) ) {}

    UnitQuaternion GetRotation() const { return this->Slice<0,3>(); }
    Point3<double> GetVelocity() const { return this->Slice<4,6>(); }
    Point3<double> GetGyroBias() const { return this->Slice<7,9>(); }
  };
    
  // Error is 1 less dimension than the State, since the rotation quaternion only has 3 DoF (4 Dims and 1 constraint)
  //   We use Error for all of our covariance matrices, otherwise the constraint in State dimension will cause the
  //   covariance matrix to become rank deficient, and therefore not invertible during Kalman gain calculation.
  class Error : public Point<9,double> {
  public:
    Error(Point<Size,double>&& p) : Point<Size,double>(p) {}
    RotationError  GetRotation() const { return this->Slice<0,2>(); }
    Point3<double> GetVelocity() const { return this->Slice<3,5>(); }
    Point3<double> GetGyroBias() const { return this->Slice<6,8>(); }
  };

  class Measurement : public Point<9,double> {
  public:
    Measurement(const Point<3,double>& accel, const Point<3,double>& gyro, const Point<3,double>& bias) 
    : Point<Size,double>( Join(Join(accel,gyro),bias) ) {}
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Update Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Update predicted state to current timestamp and increase uncertainty
  void ProcessUpdate(double dt_s);

  // Calculates the residual according to the provided measurement
  void MeasurementUpdate(const Measurement& z);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Variables
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  State                                              _state;  // current state estimate
  SmallSquareMatrix<Error::Size, double>             _P;      // current state covariance matrix estimate
  SmallMatrix<State::Size, Error::Size*2, double>    _Y;      // a discrete set of points representing the state estimate, as columns in a matrix
  SmallMatrix<Error::Size, Error::Size*2, double>    _W;      // a discrete set of points representing covariance of the state, as columns in a matrix

  static const SmallSquareMatrix<Error::Size,double> _Q, _R;  // process and measurement uncertainty
};

} // namespace Anki

#endif // IMU_FILTER_H_
