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

#include "coretech/common/robot/imuESKF.h"
#include "coretech/common/shared/math/matrix_impl.h"

namespace Anki {

namespace {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Constants
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Measurement Noise
  // NOTES: 1) measured rms noise on the gyro (~.003 rms) is higher on Z axis than the spec sheet (.00122 rms)
  //        2) we should be careful with using noise anyway - if the integration from the gyro is off and the
  //           calculated pitch/roll conflict with the accelerometer reading, using a lower noise on the gyro
  //           will result in trusting the integration more, causing very slow adjustments to gravity vector.
  constexpr const double kAccelNoise_rad      = .0018;    // rms noise
  constexpr const double kGyroNoise_radps     = .003;     // see note
  constexpr const double kBiasNoise_radps     = .0000145; // bias stability

  // Measurement Uncertainty
  const SmallSquareMatrix<9,double> MeasurementUncertainty{{  
    Util::Square(kAccelNoise_rad), 0., 0., 0., 0., 0., 0., 0., 0.,
    0., Util::Square(kAccelNoise_rad), 0., 0., 0., 0., 0., 0., 0.,
    0., 0., Util::Square(kAccelNoise_rad), 0., 0., 0., 0., 0., 0.,
    0., 0., 0., Util::Square(kGyroNoise_radps), 0., 0., 0., 0., 0.,
    0., 0., 0., 0., Util::Square(kGyroNoise_radps), 0., 0., 0., 0.,
    0., 0., 0., 0., 0., Util::Square(kGyroNoise_radps), 0., 0., 0.,
    0., 0., 0., 0., 0., 0., Util::Square(kBiasNoise_radps), 0., 0.,
    0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasNoise_radps), 0.,
    0., 0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasNoise_radps)
  }}; 

  // Gravity constants
  constexpr const Point<3,double> kGravity_G     = {0., 0., 1.};
  constexpr const double          kG_over_mmps2  = 1./9810.;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ESKF Jacobian Matrices
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // The process uncertainty as a rate of change in rotation - this can be tuned to get best performance
  constexpr const double kVelocityUncertainty_radps  = .1;

  // Process uncertainty as a function of just time and velocity uncertainty. Typically this is a 
  //   tunable diagonal matrix, but the linked paper provides a derivation for a highly performant
  //   implementation specific for IMUs
  SmallSquareMatrix<9,double> ProcessUncertaintyMatrix(double dt) {
    const double d3 = Util::Square( kVelocityUncertainty_radps ) * dt * dt * dt / 3;
    const double d2 = Util::Square( kVelocityUncertainty_radps ) * dt * dt / 2;
    const double d1 = Util::Square( kVelocityUncertainty_radps ) * dt;

    return SmallSquareMatrix<9,double>{{
       d3,  0.,  0., -d2,  0.,  0.,  d2,  0.,  0.,
       0.,  d3,  0.,  0., -d2,  0.,  0.,  d2,  0.,
       0.,  0.,  d3,  0.,  0., -d2,  0.,  0.,  d2,
      -d2,  0.,  0.,  d1,  0.,  0.,  0.,  0.,  0., 
       0., -d2,  0.,  0.,  d1,  0.,  0.,  0.,  0., 
       0.,  0., -d2,  0.,  0.,  d1,  0.,  0.,  0., 
       d2,  0.,  0.,  0.,  0.,  0.,  d1,  0.,  0., 
       0.,  d2,  0.,  0.,  0.,  0.,  0.,  d1,  0., 
       0.,  0.,  d2,  0.,  0.,  0.,  0.,  0.,  d1 
    }};
  }

  // Given a rotation error 𝛿 and elapsed time Δt, estimate the matrix F such that  x' ≅ F * x 
  SmallSquareMatrix<9,double> ProcessTransformationMatrix(const UnitQuaternion& d, double dt) {
    // calculate the rotation matrix for the rotation defined by 𝛿
    const double d01 = d[0]*d[1];
    const double d02 = d[0]*d[2];
    const double d03 = d[0]*d[3];
    const double d11 = d[1]*d[1];
    const double d12 = d[1]*d[2];
    const double d13 = d[1]*d[3];
    const double d22 = d[2]*d[2];
    const double d23 = d[2]*d[3];
    const double d33 = d[3]*d[3];

    const double r00 = 1 - (d22 + d33);
    const double r01 = 2 * (d12 - d03);
    const double r02 = 2 * (d13 + d02);
    const double r10 = 2 * (d12 + d03);
    const double r11 = 1 - (d11 + d33);
    const double r12 = 2 * (d23 - d01);
    const double r20 = 2 * (d13 - d02);
    const double r21 = 2 * (d23 + d01);
    const double r22 = 1 - (d11 + d22);

    // q' = inv(𝛿) * q + (w - b) * Δt
    // w' = w
    // b' = b
    return SmallSquareMatrix<9,double>{{
      r00, r10, r20, dt,  0.,  0., -dt,  0.,  0.,
      r01, r11, r21, 0.,  dt,  0.,  0., -dt,  0.,
      r02, r12, r22, 0.,  0.,  dt,  0.,  0., -dt,
      0.,  0.,  0.,  1.,  0.,  0.,  0.,  0.,  0., 
      0.,  0.,  0.,  0.,  1.,  0.,  0.,  0.,  0., 
      0.,  0.,  0.,  0.,  0.,  1.,  0.,  0.,  0., 
      0.,  0.,  0.,  0.,  0.,  0.,  1.,  0.,  0., 
      0.,  0.,  0.,  0.,  0.,  0.,  0.,  1.,  0., 
      0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  1. 
    }};
  }

  // Given the measurement model, calculate the jacobian around the current state
  SmallSquareMatrix<9,double> MeasurementJacobianMatrix(const Point3<double>& z) {
    return SmallSquareMatrix<9,double>{{
         0., -z[2],  z[1], 0., 0., 0., 0., 0., 0., 
       z[2],    0., -z[0], 0., 0., 0., 0., 0., 0., 
      -z[1],  z[0],    0., 0., 0., 0., 0., 0., 0., 
         0.,    0.,    0., 1., 0., 0., 0., 0., 0., 
         0.,    0.,    0., 0., 1., 0., 0., 0., 0., 
         0.,    0.,    0., 0., 0., 1., 0., 0., 0., 
         0.,    0.,    0., 0., 0., 0., 1., 0., 0., 
         0.,    0.,    0., 0., 0., 0., 0., 1., 0., 
         0.,    0.,    0., 0., 0., 0., 0., 0., 1. 
    }};
  }

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

  // Given a prior state q, and an error 𝛿, such that the new state p = q ⨁ 𝛿, according the mapping defined above
  //   calculate the jacobian of the new state given the prior state.
  SmallSquareMatrix<9,double> TransitionJacobianMatrix(const RotationError& e) {
    return SmallSquareMatrix<9,double>{{
            1.,  .5*e[2], -.5*e[1], 0., 0., 0., 0., 0., 0.,
      -.5*e[2],       1.,  .5*e[0], 0., 0., 0., 0., 0., 0.,
      .5*e[1], -.5*e[0],       1., 0., 0., 0., 0., 0., 0.,
            0.,       0.,       0., 1., 0., 0., 0., 0., 0., 
            0.,       0.,       0., 0., 1., 0., 0., 0., 0., 
            0.,       0.,       0., 0., 0., 1., 0., 0., 0., 
            0.,       0.,       0., 0., 0., 0., 1., 0., 0., 
            0.,       0.,       0., 0., 0., 0., 0., 1., 0., 
            0.,       0.,       0., 0., 0., 0., 0., 0., 1. 
    }};
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ESKF Implementation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImuESKF::ImuESKF()
{ 
  Reset( UnitQuaternion() ); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuESKF::Reset(const Rotation3d& rot) {
  _state = {rot.GetQuaternion(), {}, {}};
  _P = ProcessUncertaintyMatrix( .01 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuESKF::Update(const Point<3,double>& accel, const Point<3,double>& gyro, const float dt_s, bool isMoving)
{
  ProcessUpdate( dt_s );

  // if we don't believe we are moving use the gyro as a bias measurement,
  //   otherwise just assume the bias is constant 
  MeasurementUpdate( { accel * kG_over_mmps2, gyro, (isMoving ? _state.GetGyroBias() : gyro) } );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuESKF::ProcessUpdate(double dt_s)
{ 
  const auto dq = ToQuat((_state.GetVelocity() - _state.GetGyroBias()) * dt_s);
  const auto F  = ProcessTransformationMatrix(dq, dt_s);
  const auto Q  = ProcessUncertaintyMatrix(dt_s);

  // only project the rotation forward in time, velocity and bias are assumed constant
  _state.SetRotation(_state.GetRotation() * dq); 
  _P = F * (_P + Q) * F.GetTranspose();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuESKF::MeasurementUpdate(const Measurement& z)
{
  // Calculate Predicted measurement given the current state estimate
  const auto g_Local = _state.GetRotation().GetConj() * kGravity_G;
  const auto zt = Join(Join( g_Local, _state.GetVelocity() ), _state.GetGyroBias());
  
  // Calculate Kalman gain K and the residual
  const auto R   = MeasurementUncertainty;
  const auto H   = MeasurementJacobianMatrix(zt);
  const auto Ht  = H.GetTranspose();
  const auto S   = (H * _P * Ht) + R;
  const auto K   = _P * Ht * S.GetInverse();
  const auto res = K * (z - zt);
  const auto T   = TransitionJacobianMatrix(res.Slice<0,2>());

  // Add the residual to the current state
  _state.SetRotation( _state.GetRotation() * ToQuat(res.Slice<0,2>()) );
  _state.SetVelocity( _state.GetVelocity() + res.Slice<3,5>() );
  _state.SetGyroBias( _state.GetGyroBias() + res.Slice<6,8>() );

  // Update the covariance
  _P = T*(_P - K*H*_P)*T.GetTranspose();
}
      
} // namespace Anki
