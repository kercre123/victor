/**
 * File: imuUKF.cpp
 *
 * Author: Michael Willett
 * Created: 1/7/2019
 *
 * Description:
 *
 *   UKF implementation for orientation and gyro bias tracking
 *   Orientation of gyro axes is assumed to be identical to that of robot when the head is at 0 degrees.
 *   i.e. x-axis points forward, y-axis points to robot's left, z-axis points up.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "coretech/common/robot/imuUKF.h"
#include "coretech/common/shared/math/matrix_impl.h"

namespace Anki {

namespace {
  // Calculates the decomposition of a positive definite NxN matrix A s.t. A = L * L'
  // MATH INTUITION: We can use the columns of L to convert between a covariance matrix Pxx and Sigma
  //   points {X_i} in a consistent manner. Given a set of N Sigma points {X_i} stored as the columns
  //   of a matrix X, the covariance Pxx is calculated as X*X' / N. Therefore,  Pxx = A = L * L' / N.
  template<MatDimType N>
  SmallSquareMatrix<N,double> Cholesky(const SmallSquareMatrix<N,double>& A) {
    SmallSquareMatrix<N,double> L;
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j <= i; ++j) {
          double s = 0;
          for (size_t k = 0; k < j; ++k) {
              s += L(j, k) * L(i, k);
          }
          L(i, j) = (i == j) ? sqrt(A(i, i) - s) : (A(i, j) - s) / L(j, j);
      }
    }
    return L;
  }

  // Fast mean calculation for rows of a matrix
  template <MatDimType M, MatDimType N>
  Point<M,double> CalculateMean(const SmallMatrix<M,N,double>& A) {
    return A * Point<N,double>(1./N);
  }

  template<MatDimType M, MatDimType N, MatDimType O>
  SmallMatrix<M,O,double> GetCovariance(const SmallMatrix<M,N,double>& A, const SmallMatrix<O,N,double>& B) {
    return (A*B.GetTranspose()) * (1./N);
  }
  
  template<MatDimType M, MatDimType N>
  SmallSquareMatrix<M,double> GetCovariance(const SmallMatrix<M,N,double>& A) {
    return GetCovariance(A, A);
  }

  // Process Noise
  constexpr const double kRotStability_rad    = .0005;     // assume pitch & roll don't change super fast when driving
  constexpr const double kGyroStability_radps = 4.3;       // half max observable rotation from gyro
  constexpr const double kBiasStability_radps = .0000145;  // bias stability

  // Measurement Noise
  // NOTES: 1) measured rms noise on the gyro (~.003 rms) is higher on Z axis than the spec sheet (.00122 rms)
  //        2) we should be careful with using noise anyway - if the integration from the gyro is off and the
  //           calculated pitch/roll conflict with the accelerometer reading, using a lower noise on the gyro
  //           will result in trusting the integration more, causing very slow adjustments to gravity vector.
  constexpr const double kAccelNoise_rad      = .0018;  // rms noise
  constexpr const double kGyroNoise_radps     = .003;   // see note
  constexpr const double kBiasNoise_radps     = .00003; // 2 * bias stability

  // extra tuning param for how much we distribute the sigma points
  const double kWsigma =  1./(2*9.); // 1 / (2N) := equal weight for all sigma points
  const double kCholScale = sqrt(1./(2*kWsigma));

  // Gravity constants
  constexpr const Point<3,double> kGravity_G = {0., 0., 1.};
  constexpr const double          kG_over_mmps2  = 1./9810.;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// UKF Implementation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Process Uncertainty
const SmallSquareMatrix<ImuUKF::Error::Size,double> ImuUKF::_Q{{  
  Util::Square(kRotStability_rad), 0., 0., 0., 0., 0., 0., 0., 0.,
  0., Util::Square(kRotStability_rad), 0., 0., 0., 0., 0., 0., 0.,
  0., 0., Util::Square(kRotStability_rad), 0., 0., 0., 0., 0., 0.,
  0., 0., 0., Util::Square(kGyroStability_radps), 0., 0., 0., 0., 0.,
  0., 0., 0., 0., Util::Square(kGyroStability_radps), 0., 0., 0., 0.,
  0., 0., 0., 0., 0., Util::Square(kGyroStability_radps), 0., 0., 0.,
  0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radps), 0., 0.,
  0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radps), 0.,
  0., 0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radps)
}}; 

// Measurement Uncertainty
const SmallSquareMatrix<ImuUKF::Error::Size,double> ImuUKF::_R{{  
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImuUKF::ImuUKF()
{ 
  Reset( UnitQuaternion() ); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuUKF::Reset(const Rotation3d& rot) {
  _state = {rot.GetQuaternion(), {}, {}};
  _P = _Q;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuUKF::Update(const Point<3,double>& accel, const Point<3,double>& gyro, const float dt_s, bool isMoving)
{
  ProcessUpdate( dt_s );
  MeasurementUpdate( { accel * kG_over_mmps2, gyro, (isMoving ? _state.GetGyroBias() : gyro) } );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuUKF::ProcessUpdate(double dt_s)
{ 
  // sample the covariance, generating the set {𝑌ᵢ} and add the mean
  const auto S = Cholesky(_P + _Q) * kCholScale;
  for (int i = 0; i < Error::Size; ++i) {
    // get the Sigma points for the given state
    const Error Si = S.GetColumn(i);
    const auto  q  = ToQuat(Si.GetRotation());
    const auto  r1 = _state.GetRotation() * q;
    const auto  r2 = _state.GetRotation() * q.GetConj();
    const auto  w1 = _state.GetVelocity() + Si.GetVelocity();
    const auto  w2 = _state.GetVelocity() - Si.GetVelocity();
    const auto  b1 = _state.GetGyroBias() + Si.GetGyroBias();
    const auto  b2 = _state.GetGyroBias() - Si.GetGyroBias();

    // current process model assumes we continue moving at constant velocity
    _Y.SetColumn(2*i,     State{r1 * ToQuat((w1-b1) * dt_s), w1, b1} );
    _Y.SetColumn((2*i)+1, State{r2 * ToQuat((w2-b2) * dt_s), w2, b2} );
  }

  // NOTE: we are making a huge assumption here. Technically, quaternions cannot be averaged using
  //   typical average calculation: Σ(xᵢ)/N. However, we assume in this model that we are calling
  //   the process update frequently enough that the elements of _Y do not diverge very quickly,
  //   in which case an element wise mean will converge to the same result as more accurate
  //   quaternion mean calculation methods. If this does not hold in the future, I have verified
  //   that both a gradient decent method and largest Eigen Vector method work well.
  _state = CalculateMean(_Y);

  // Calculate Process Noise {𝑊ᵢ} by mean centering {𝑌ᵢ}
  const auto meanRot  = _state.GetRotation();
  const auto meanVel  = _state.GetVelocity();
  const auto meanBias = _state.GetGyroBias();
  for (int i = 0; i < 2*Error::Size; ++i) {
    // We want to get the mean centered rotaions {eᵢ} of a set of quaternions, {qᵢ}:
    //                  qᵢ := q_mean * eᵢ
    //    inv(q_mean) * qᵢ  = (inv(q_mean) * q_mean) * eᵢ = 1 * eᵢ
    const State yi    = _Y.GetColumn(i);
    const auto  err   = FromQuat(meanRot.GetConj() * yi.GetRotation());
    const auto  omega = yi.GetVelocity() - meanVel;
    const auto  bias  = yi.GetGyroBias() - meanBias;
    const auto  wi    = Join(Join(err, omega), bias);
    _W.SetColumn(i, wi);
  }

  // remove Sigma point scaling and update covariance
  _P = GetCovariance(_W) * kWsigma;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuUKF::MeasurementUpdate(const Measurement& z)
{
  // Calculate Predicted measurement distribution {𝑍ᵢ} given State estimate distribution {𝑌ᵢ}
  SmallMatrix<Error::Size,Error::Size*2,double> Z;
  for (int i = 0; i < 2*Error::Size; ++i) {
    // NOTE: The state rotation q defines a rotation that maps points in IMU frame to world frame.
    //   In order to calculate the expected accelerometer reading in IMU frame, we apply the
    //   inverse rotation of q to the gravity vector in world frame.
    const State yi = _Y.GetColumn(i);
    const auto zi = Join(Join( yi.GetRotation().GetConj() * kGravity_G, yi.GetVelocity() ), yi.GetGyroBias());
    Z.SetColumn(i, zi);
  }

  // mean center {𝑍ᵢ}
  const auto meanZ = CalculateMean(Z);
  for (int i = 0; i < 2*Error::Size; ++i) { 
    Z.SetColumn(i, Z.GetColumn(i) - meanZ); 
  }

  // get Kalman gain and update covariance after removing Sigma point scaling
  const auto Pvv = GetCovariance(Z) * kWsigma + _R;
  const auto Pxz = GetCovariance(_W, Z) * kWsigma;
  const auto K = Pxz * Pvv.GetInverse();
  _P -= K * Pvv * K.GetTranspose();

  // get measurement residual
  const Error residual = K*(z - meanZ);
    
  // Add the residual to the current state. 
  _state = State{ _state.GetRotation() * ToQuat(residual.GetRotation()),
                  _state.GetVelocity() + residual.GetVelocity(),
                  _state.GetGyroBias() + residual.GetGyroBias()
                };
}
      
} // namespace Anki
