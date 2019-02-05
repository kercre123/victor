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
  // calculates the decomposition of a positive definite NxN matrix A s.t. A = L' * L    
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
  
  // transforms rotation vector into a quaternion
  inline UnitQuaternion ToQuat(const Point<3,double>& v) { 
    const double alpha = v.Length();
    const double sinAlpha = sin(alpha * .5) / (NEAR_ZERO(alpha) ? 1. : alpha);
    return {cos(alpha * .5), sinAlpha * v.x(),  sinAlpha * v.y(),  sinAlpha * v.z()};
  };
  
  // transforms quaternion into a rotation vector 
  inline Point<3,double> FromQuat(const UnitQuaternion& q) { 
    Point<3,double> axis = q.Slice<1,3>();
    const double alpha = asin( axis.MakeUnitLength() ) * 2;
    return axis * alpha;
  };

  // fast mean calculation for columns of a matrix
  template <MatDimType M, MatDimType N>
  Point<M,double> CalculateMean(const SmallMatrix<M,N,double>& A) {
    return A * Point<N,double>(1./N);
  }

  template<MatDimType M, MatDimType N, MatDimType O>
  SmallMatrix<M,O,double> GetCovariance(const SmallMatrix<M,N,double>& A, const SmallMatrix<N,O,double>& B) {
    return (A*B) * (1./N);
  }
  
  template<MatDimType M, MatDimType N>
  SmallSquareMatrix<M,double> GetCovariance(const SmallMatrix<M,N,double>& A) {
    return GetCovariance(A, A.GetTranspose());
  }

  // Process Noise
  constexpr const double kRotStability_rad    = .001;      // assume pitch & roll don't change super fast when driving
  constexpr const double kGyroStability_radps = .2;        // leave this relatively high to trust most recent gyro data
  constexpr const double kBiasStability_radps = .0000145;  // bias stability

  // Measurement Noise
  // NOTES: 1) measured rms noise on the gyro (~.006 rms) is higher on Z axis than the spec sheet (.00122 rms)
  //        2) we should be careful with using noise anyway - if the integration from the gyro is off and the
  //           calculated pitch/roll conflict with the accelerometer reading, using a lower noise on the gyro
  //           will result in trusting the integration more, causing very slow adjustments to gravity vector.
  constexpr const double kAccelNoise_rad      = .0018;  // rms noise
  constexpr const double kGyroNoise_radps     = .008;   // see note
  constexpr const double kBiasNoise_radps     = .006;   // measured gyro noise


  // Why enforce a slower update rate rather than lower process noise? I'm glad you asked! I honestly have 
  // no idea why we get better data from this - but it was found when compariing the same filter running
  // on the robot and engine process where the only difference was the update frequency. When running
  // on the robot thread, we consistently over-integrated the gyro around the gravity vector. My assumption
  // is that this is a result of inflating the gyro noise to allow for gravity compensation, but I have
  // been unable to verify.
  constexpr const double kUpdatePeriod_s = .02;

  // Gravity constants
  constexpr const Point<3,double> kGravity_mmpsSq = {0., 0., 9810.};
  constexpr const double          kG_over_mmpsSq  = 1./9810.;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// UKF Implementation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Process Uncertainty
const SmallSquareMatrix<ImuUKF::Error::Dim,double> ImuUKF::_Q{{  
  Util::Square(kRotStability_rad), 0., 0., 0., 0., 0., 0., 0., 0.,
  0., Util::Square(kRotStability_rad), 0., 0., 0., 0., 0., 0., 0.,
  0., 0., Util::Square(kRotStability_rad), 0., 0., 0., 0., 0., 0.,
  0., 0., 0., Util::Square(kGyroStability_radps), 0., 0., 0., 0., 0.,
  0., 0., 0., 0., Util::Square(kGyroStability_radps), 0., 0., 0., 0.,
  0., 0., 0., 0., 0., Util::Square(kGyroStability_radps) , 0., 0., 0.,
  0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radps), 0., 0.,
  0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radps), 0.,
  0., 0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radps)
}}; 

// Measurement Uncertainty
const SmallSquareMatrix<ImuUKF::Error::Dim,double> ImuUKF::_R{{  
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
  _lastMeasurement_s = 0.;
  _P = _Q;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuUKF::Update(const Point<3,double>& accel, const Point<3,double>& gyro, const float timestamp_s, bool isMoving)
{
  if (timestamp_s - _lastMeasurement_s < kUpdatePeriod_s) { return; }

  ProcessUpdate( NEAR_ZERO(_lastMeasurement_s) ? 0. : timestamp_s - _lastMeasurement_s );

  const auto measurement = Join(Join(accel, gyro), (isMoving ? _state.GetGyroBias() : gyro));
  const Error residual = MeasurementUpdate( measurement );

  // Add the residual to the current state. 
  // NOTE: Right now I am artificially scaling the bias back just to limit rapid change during startup.
  //       When initializing it can be quite noisy, resulting in an unstable z-rotation for the first ~30 seconds
  const double biasScale = isMoving ? 0. : .02;
  _state = State{ _state.GetRotation() * ToQuat(residual.GetRotation() * kG_over_mmpsSq),
                  _state.GetVelocity() + residual.GetVelocity(),
                  _state.GetGyroBias() + (residual.GetGyroBias() * biasScale)
                };

  _lastMeasurement_s = timestamp_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuUKF::ProcessUpdate(double dt_s)
{ 
  // sample the covariance, generating the set {𝑌ᵢ} and add the mean
  const auto S = Cholesky(_P + _Q) * sqrt(2*Error::Dim);
  for (int i = 0; i < Error::Dim; ++i) {
    // current process model assumes we continue moving at constant velocity
    const Error Si = S.GetColumn(i);
    const auto  q  = ToQuat(Si.GetRotation());
    const auto  w1 = _state.GetVelocity() + Si.GetVelocity();
    const auto  w2 = _state.GetVelocity() - Si.GetVelocity();
    const auto  b1 = _state.GetGyroBias() + Si.GetGyroBias();
    const auto  b2 = _state.GetGyroBias() - Si.GetGyroBias();

    _Y.SetColumn(2*i,     State{_state.GetRotation() * q * ToQuat((w1-b1) * dt_s), w1, b1} );
    _Y.SetColumn((2*i)+1, State{_state.GetRotation() * q.GetConj() * ToQuat((w2-b2) * dt_s), w2, b2} );
  }

  // NOTE: we are making a huge assumption here. Technically, quaternions cannot be averaged using
  //       typical average calculation: Σ(xᵢ)/N. However, we assume in this model that we are calling
  //       the process update frequently enough that the elements of _Y do not diverge very quickly,
  //       in which case an element wise mean will converge to the same result as more accurate
  //       quaternion mean calculation methods. If this does not hold in the future, I have verified
  //       that both a gradient decent method and largest Eigen Vector method work well.
  _state = CalculateMean(_Y);

  // Calculate Process Noise {𝑊ᵢ} by mean centering {𝑌ᵢ}
  const auto meanRot  = _state.GetRotation();
  const auto meanVel  = _state.GetVelocity();
  const auto meanBias = _state.GetGyroBias();
  for (int i = 0; i < 2*Error::Dim; ++i) {
    const State yi = _Y.GetColumn(i);
    const auto err = FromQuat(meanRot.GetConj() * yi.GetRotation());
    const auto omega = yi.GetVelocity() - meanVel;
    const auto bias = yi.GetGyroBias() - meanBias;
    const auto wi = Join(Join(err, omega), bias);
    _W.SetColumn(i, wi);
  }
  _P = GetCovariance(_W);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Point<9,double> ImuUKF::MeasurementUpdate(const Point<9,double>& measurement)
{
  // Calculate Predicted Measurement Distribution {𝑍ᵢ}
  SmallMatrix<Error::Dim,Error::Dim*2,double> Z;
  for (int i = 0; i < 2*Error::Dim; ++i) {
    const State yi = _Y.GetColumn(i);
    const auto zi = Join(Join( yi.GetRotation().GetConj() * kGravity_mmpsSq, yi.GetVelocity() ), yi.GetGyroBias());
    Z.SetColumn(i, zi);
  }

  // mean center {𝑍ᵢ}
  const auto meanZ = CalculateMean(Z);
  for (int i = 0; i < 2*Error::Dim; ++i) { 
    Z.SetColumn(i, Z.GetColumn(i) - meanZ); 
  }

  // get Kalman gain and update covariance
  const auto Pvv = GetCovariance(Z) + _R;
  const auto Pxz = GetCovariance(_W, Z.GetTranspose());
  const auto K = Pxz * Pvv.GetInverse();
  _P -= K * Pvv * K.GetTranspose();

  // get measurement residual
  return K*(measurement - meanZ);
}
      
} // namespace Anki
