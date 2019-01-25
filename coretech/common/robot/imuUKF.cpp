/**
 * File: imuFilter.cpp
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
#include "coretech/common/engine/math/matrix_impl.h"

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
  inline UnitQuaternion ToQuat(Point<3,double> v) { 
    const double alpha = v.MakeUnitLength();
    const double sinAlpha = sin(alpha * .5);
    return {cos(alpha * .5), sinAlpha * v.x(),  sinAlpha * v.y(),  sinAlpha * v.z()};
  };
  
  // transforms quaternion into a rotation vector 
  inline Point<3,double> FromQuat(const UnitQuaternion& q) { 
    Point<3,double> axis = q.Slice<1,3>();
    const double alpha = asin( axis.MakeUnitLength() ) * 2;
    return axis * alpha;
  };

  // given two vectors, find the rotation that maps `from` onto `to`
  UnitQuaternion FindRotation(const Point<3,double>& from, const Point<3,double>& to) {
    Point<3,double> a = from;
    Point<3,double> b = to;
    a.MakeUnitLength();
    b.MakeUnitLength();
    
    const double cosAlpha = DotProduct(a, b);
    const double sinAlpha2 = sqrt( (1-cosAlpha) / 2 );
    const double cosAlpha2 = sqrt( (1+cosAlpha) / 2 );
    const auto v = CrossProduct(a,b) * sinAlpha2;
    return {cosAlpha2, v.x(), v.y(), v.z()};
  }

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

  // Tunable Parameters
  constexpr const double          kRotAccel_radpsSq      = 4.36;     // currently this is half max rot velocity reading, 
                                                                     // so we assume a ramp of 0 to max measurement in 2 seconds
  constexpr const double          kBiasStability_radpsSq = .0000145; // from spec sheet

  // Measurement Noise
  
  // this is rms noise + max cross axis sensitivity over gravity + package alignment error
  constexpr const double          kAccelNoise_mmpsSq     = .0205;  // atan2(17.7 + 98.1 + 85.6, 9810.);   
  constexpr const double          kGyroNoise_radps       = .00123; // rms
  constexpr const double          kBiasNoise_radps       = kGyroNoise_radps + .052;       // gyro noise + temp offset
  
  constexpr const Point<3,double> kGravity_mmpsSq        = {0., 0., 9810.};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// UKF Implementation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImuUKF::ImuUKF()
{ 
  Reset( UnitQuaternion() ); 
}

void ImuUKF::Reset(const Rotation3d& rot) {
  _state = {rot.GetQuaternion(), Point<3,double>(), Point<3,double>()};
  _P = SmallSquareMatrix<State::Dim,double> {{
    .01, 0., 0., 0., 0., 0., 0., 0., 0., 
    0., .01, 0., 0., 0., 0., 0., 0., 0.,
    0., 0., .01, 0., 0., 0., 0., 0., 0.,
    0., 0., 0., .0001, 0., 0., 0., 0., 0.,
    0., 0., 0., 0., .0001, 0., 0., 0., 0.,
    0., 0., 0., 0., 0., .0001, 0., 0., 0.,
    0., 0., 0., 0., 0., 0., .00001, 0., 0.,
    0., 0., 0., 0., 0., 0., 0., .00001, 0.,
    0., 0., 0., 0., 0., 0., 0., 0., .00001
  }};
}
 
// Process Uncertainty
const SmallSquareMatrix<ImuUKF::State::Dim,double> ImuUKF::_Q{{  
  .0001, 0., 0., 0., 0., 0., 0., 0., 0.,
  0., .0001, 0., 0., 0., 0., 0., 0., 0.,
  0., 0., .0001, 0., 0., 0., 0., 0., 0.,
  0., 0., 0., Util::Square(kRotAccel_radpsSq), 0., 0., 0., 0., 0.,
  0., 0., 0., 0., Util::Square(kRotAccel_radpsSq), 0., 0., 0., 0.,
  0., 0., 0., 0., 0., Util::Square(kRotAccel_radpsSq) , 0., 0., 0.,
  0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radpsSq), 0., 0.,
  0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radpsSq), 0.,
  0., 0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasStability_radpsSq)
}}; 

// Measurement Uncertainty
const SmallSquareMatrix<ImuUKF::State::Dim,double> ImuUKF::_R{{  
  Util::Square(kAccelNoise_mmpsSq), 0., 0., 0., 0., 0., 0., 0., 0.,
  0., Util::Square(kAccelNoise_mmpsSq), 0., 0., 0., 0., 0., 0., 0.,
  0., 0., Util::Square(kAccelNoise_mmpsSq), 0., 0., 0., 0., 0., 0.,
  0., 0., 0., Util::Square(kGyroNoise_radps), 0., 0., 0., 0., 0.,
  0., 0., 0., 0., Util::Square(kGyroNoise_radps), 0., 0., 0., 0.,
  0., 0., 0., 0., 0., Util::Square(kGyroNoise_radps), 0., 0., 0.,
  0., 0., 0., 0., 0., 0., Util::Square(kBiasNoise_radps), 0., 0.,
  0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasNoise_radps), 0.,
  0., 0., 0., 0., 0., 0., 0., 0., Util::Square(kBiasNoise_radps)
}}; 

void ImuUKF::Update(const Point<3,double>& accel, const Point<3,double>& gyro, const float timestamp_s)
{
  if (!NEAR_ZERO(_lastMeasurement_s)) {
    const auto measurement = Concatenate(Concatenate(accel, gyro), _state.GetGyroBias() );
    ProcessUpdate( timestamp_s - _lastMeasurement_s );
    const auto residual = MeasurementUpdate( measurement );

    // NOTE: I think there is a more computationally efficient way for getting the 
    //       correct rotation using just the residual and rotG, but this makes more
    //       intuitive sense for now...
    const auto rotG = _state.GetRotation().GetConj() * kGravity_mmpsSq;
    const auto rotCorrection = FindRotation(rotG, rotG + residual.Slice<0,2>());
    _state = State{ _state.GetRotation() * rotCorrection,
                    _state.GetVelocity() + residual.Slice<3,5>(),
                    _state.GetGyroBias() };
  }
  _lastMeasurement_s = timestamp_s;
}


void ImuUKF::UpdateBias(const Point<3,double>& accel, const Point<3,double>& gyro, float timestamp_s)
{
  if (!NEAR_ZERO(_lastMeasurement_s)) {
    ProcessUpdate( timestamp_s - _lastMeasurement_s );
    const auto measurement = Concatenate(Concatenate(accel, gyro), gyro );
    const auto residual = MeasurementUpdate( measurement );

    // NOTE: I think there is a more computationally efficient way for getting the 
    //       correct rotation using just the residual and rotG, but this makes more
    //       intuitive sense for now...
    const auto rotG = _state.GetRotation().GetConj() * kGravity_mmpsSq;
    const auto rotCorrection = FindRotation(rotG, rotG + residual.Slice<0,2>());
    _state = State{ _state.GetRotation() * rotCorrection,
                    _state.GetVelocity() + residual.Slice<3,5>(),
                    _state.GetGyroBias() + residual.Slice<6,8>() };
  }
  _lastMeasurement_s = timestamp_s;
}

void ImuUKF::ProcessUpdate(double dt_s)
{ 
  // sample the covariance, generating the set {𝑊ᵢ} and add the mean
  const SmallSquareMatrix<State::Dim,double> Qdt = _Q * dt_s;
  const auto S = Cholesky(_P + Qdt) * sqrt(2*State::Dim);
  for (int i = 0; i < State::Dim; ++i) {
    // current process model assumes we continue moving at constant velocity
    const auto Si = S.GetColumn(i);
    const auto q  = ToQuat(Si.Slice<0,2>());
    const auto w1 = _state.GetVelocity() + Si.Slice<3,5>();
    const auto w2 = _state.GetVelocity() - Si.Slice<3,5>();
    const auto b1 = _state.GetGyroBias() + Si.Slice<6,8>();
    const auto b2 = _state.GetGyroBias() - Si.Slice<6,8>();
    const State s1(_state.GetRotation() * q * ToQuat((w1-b1) * dt_s), w1, b1);
    const State s2(_state.GetRotation() * q.GetConj() * ToQuat((w2-b2) * dt_s), w2, b2);

    _Y.SetColumn(2*i,s1);
    _Y.SetColumn((2*i)+1, s2);
  }

  // NOTE: we are making a huge assumption here. Technically, quaternions cannot be averaged using
  //       typical average calculation: Σ(xᵢ)/N. However, we assume in this model that we are calling
  //       the process update frequently enough that the elements of _Y do not diverge very quickly,
  //       in which case an element wise mean will converge to the same result as more accurate
  //       quaternion mean calculation methods. If this does not hold in the future, I have verified
  //       that both a gradient decent method and largest Eigen Vector method work well.
  _state = CalculateMean(_Y);

  // Calculate Process Noise by mean centering Y
  const auto meanRot  = _state.GetRotation();
  const auto meanVel  = _state.GetVelocity();
  const auto meanBias = _state.GetGyroBias();
  for (int i = 0; i < 2*State::Dim; ++i) {
    const State yi = _Y.GetColumn(i);
    const auto err = FromQuat(meanRot.GetConj() * yi.GetRotation());
    const auto omega = yi.GetVelocity() - meanVel;
    const auto bias = yi.GetGyroBias() - meanBias;
    _W.SetColumn(i, Concatenate(Concatenate(err, omega), bias));
  }
  _P = GetCovariance(_W);
}

Point<9,double> ImuUKF::MeasurementUpdate(const Point<9,double>& measurement)
{
  // Calculate Predicted Measurement Distribution {Zᵢ}
  SmallMatrix<State::Dim,State::Dim*2,double> Z;
  for (int i = 0; i < 2*State::Dim; ++i) {
    const State yi = _Y.GetColumn(i);
    Z.SetColumn(i, Concatenate(Concatenate( yi.GetRotation().GetConj() * kGravity_mmpsSq, yi.GetVelocity() ), yi.GetGyroBias()));
  }

  // mean center {Zᵢ}
  const auto meanZ = CalculateMean(Z);
  for (int i = 0; i < 2*State::Dim; ++i) { 
    Z.SetColumn(i, Z.GetColumn(i) - meanZ); 
  }

  // get Covariance
  const auto Pvv = GetCovariance(Z) + _R;
  const auto Pxz = GetCovariance(_W, Z.GetTranspose());


  // get Kalman gain and update covariance
  const auto K = Pxz * Pvv.CopyInverse();
  _P -= K * Pvv * K.GetTranspose();

  // get measurement residual
  return K*(measurement - meanZ);
}
      
} // namespace Anki
