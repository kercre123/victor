/**
 * File: imuESKF.h
 *
 * Author: Michael Willett
 * Created: 2/25/2019
 *
 * Description:
 *
 *   ESKF implementation for orientation and gyro bias tracking
 *   see implementation details in: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC6339217/pdf/sensors-19-00149.pdf
 *      Kalman Filtering for Attitude Estimation with Quaternions and Concepts
 *      from Manifold Theory.
 *         Pablo Bernal-Polo and Humberto Martinez-Barbera
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef _IMU_ESKF_H_
#define _IMU_ESKF_H_

#include "coretech/common/shared/math/rotation.h"

namespace Anki {

class ImuESKF {
public:

  // Constructor
  ImuESKF();
  
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
  // Class Definitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // rotation quaternion, rotational velocity, and gyro bias: 
  //   [q0,q1,q2,q3,⍵1,⍵2,⍵3,b0,b2,b2]
  class State : public Point<10,double> {
  public:
    State() : Point<Size,double>(0.) { SetRotation( UnitQuaternion() ); }
    State(Point<Size,double>&& p) : Point<Size,double>(p) {}

    State(const UnitQuaternion& q, const Point<3,double>& w, const Point<3,double> b) 
    : Point<Size,double>( Join(Join(q,w),b) ) {}

    UnitQuaternion GetRotation() const   { return this->Slice<0,3>(); }
    Point3<double> GetVelocity() const   { return this->Slice<4,6>(); }
    Point3<double> GetGyroBias() const   { return this->Slice<7,9>(); }

    void SetRotation(UnitQuaternion&& q) { std::move( &q[0], &q[4], &(*this)[0] ); }
    void SetVelocity(Point3<double>&& w) { std::move( &w[0], &w[3], &(*this)[4] ); }
    void SetGyroBias(Point3<double>&& b) { std::move( &b[0], &b[3], &(*this)[7] ); }
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
  State                        _state;  // current state estimate
  SmallSquareMatrix<9, double> _P;      // current state covariance matrix estimate
};

} // namespace Anki

#endif // IMU_FILTER_H_
