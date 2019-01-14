/**
 * File: imuFilter.h
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

#include "imuUKF.h"
#include "coretech/common/engine/math/matrix_impl.h"

#define StateDim 6

namespace Anki {
namespace Vector {

namespace {
  // calculates the decomposition of a positive definite n x n matrix A s.t. A = L' * L    
  template<MatDimType N>
  SmallSquareMatrix<N,float> Cholesky(const SmallSquareMatrix<N,float>& A) {
    SmallSquareMatrix<N,float> L;

    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j <= i; ++j) {
          float s = 0;
          for (size_t k = 0; k < j; ++k) {
              s += L(j, k) * L(i, k);
          }
          L(i, j) = (i == j) ? sqrt(A(i, i) - s) : (A(i, j) - s) / L(j, j);
      }
    }
    return L;
  }
  
  // transforms rotation vector into a quaternion
  Rotation3d ErrorToQuat(Point3f v) {
    const float alpha = v.MakeUnitLength();
    return Rotation3d(alpha, NEAR_ZERO(alpha) ? Z_AXIS_3D() : v);
  };
  
  // transforms a quaternion into a rotation vector
  Point3f QuaternionToError(const Rotation3d& q) {
    return q.GetAxis() * q.GetAngle().ToFloat();
  };

  template <MatDimType N>
  ImuUKF::State CalculateMean(const std::array<ImuUKF::State,N>& states) {
    int t = 0;
    Rotation3d qt = states[0].rotation;
    Point3f avgErr;
    do {
      avgErr = {0.f, 0.f, 0.f};
      for (const auto& e : states) {
        avgErr += QuaternionToError(e.rotation * qt.GetInverse());
      }
      avgErr /= N;

      qt = ErrorToQuat(avgErr) * qt;
    } while( FLT_GT(avgErr.LengthSq(), .000001f) && ++t < 10);
    
    // mean velocity is just the linear mean
    Point3f meanVel;
    for (int i = 0; i < N; ++i) {
      meanVel += states[i].velocity;
    }
    meanVel /= N;

    return {qt, meanVel};
  }

  // given a mean and covariance matrix, generate a set of sigma points that represent this distribution
  std::array<ImuUKF::State,12> GetSigmaPoints(const ImuUKF::State& mean, const SmallSquareMatrix<6,float>& cov)
  {
    // sample the covariance, generating the set {𝑊ᵢ} and add the mean
    const auto S = Cholesky( cov ) * sqrtf(StateDim);
    std::array<ImuUKF::State,12> sigma;
    for (int i = 0; i < StateDim; ++i) {
      const auto Si = S.GetColumn(i);
      sigma[i].rotation = mean.rotation * ErrorToQuat({Si[0], Si[1], Si[2]}); 
      sigma[i].velocity = mean.velocity + Point3f{Si[3], Si[4], Si[5]};

      sigma[i + StateDim].rotation = mean.rotation * ErrorToQuat({-Si[0], -Si[1], -Si[2]}); 
      sigma[i + StateDim].velocity = mean.velocity - Point3f{Si[3], Si[4], Si[5]};
    }
    return sigma;
  }

  
  template<MatDimType M, MatDimType N>
  SmallSquareMatrix<M,float> GetCovariance(const SmallMatrix<M,N,float>& A, const SmallMatrix<N,M,float>& B) {
    return (A*B) * (1.f/((float)N));
  }
  
  template<MatDimType M, MatDimType N>
  SmallSquareMatrix<M,float> GetCovariance(const SmallMatrix<M,N,float>& A) {
    return GetCovariance(A, A.GetTranspose());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// UKF Implementation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImuUKF::ImuUKF()
: _Q{{  100.f, 0.f, 0.f, 0.f, 0.f, 0.f,
        0.f, 100.f, 0.f, 0.f, 0.f, 0.f,
        0.f, 0.f, 100.f, 0.f, 0.f, 0.f,
        0.f, 0.f, 0.f, .01f, 0.f, 0.f,
        0.f, 0.f, 0.f, 0.f, .01f, 0.f,
        0.f, 0.f, 0.f, 0.f, 0.f, .01f
      }} // Process Uncertainty
, _R{{  .1f, 0.f, 0.f, 0.f, 0.f, 0.f,
        0.f, .1f, 0.f, 0.f, 0.f, 0.f,
        0.f, 0.f, .1f, 0.f, 0.f, 0.f,
        0.f, 0.f, 0.f, .01f, 0.f, 0.f,
        0.f, 0.f, 0.f, 0.f, .01f, 0.f,
        0.f, 0.f, 0.f, 0.f, 0.f, .01f
      }} // Measurement Uncertainty
{}

void ImuUKF::Update(const Point3f& accel, const Point3f& gyro, RobotTimeStamp_t t_ms)
{
  float dt_s = (_lastMeasurement_ms == 0) ? 0.f : ((float)(t_ms - _lastMeasurement_ms)) / 1000;
  _lastMeasurement_ms = t_ms;

  ProcessUpdate(dt_s);

  // this code assumes +z is down, while in the robot frame it is up...
  Point3f rotAcc = Rotation3d(0, Y_AXIS_3D()) * accel;
  rotAcc.MakeUnitLength();

  // normalize accel since we assume a unit gravity vector
  MeasurementUpdate( Concatenate(rotAcc, gyro) );
}

void ImuUKF::ProcessUpdate(float dt_s)
{ 
  // current process model assumes we continue moving at constant velocity
  _Y = GetSigmaPoints(_state, _P + _Q);
  for (auto& y : _Y) {
    y.rotation *= ErrorToQuat(y.velocity * dt_s);
  }
  _state = CalculateMean(_Y);

  // Calculate Process Noise by mean centering Y
  for (int i = 0; i < 2*StateDim; ++i) {
    // something weird here:  this should be Yi * _state.Inv
    // const auto err = QuaternionToError( _Y[i].rotation * _state.rotation.GetInverse() );
    const auto err = QuaternionToError( _state.rotation.GetInverse() * _Y[i].rotation );
    const auto omega = _Y[i].velocity - _state.velocity;
    _W.SetColumn(i, Concatenate(err, omega));
  }

  _P = GetCovariance(_W);
}

void ImuUKF::MeasurementUpdate(const Point<6,float>& measurement)
{
  // Calculate Predicted Measurement Distribution {Zᵢ}
  SmallMatrix<6,12,float> Z;
  const Point3f kGravity(0.f, 0.f, 1.f);
  // const UnitQuaternion kGravity(0.f, 0.f, 0.f, 1.f);
  for (int i = 0; i < 2*StateDim; ++i) {
    // something weird here:  this should be Yi * kGravity
    Z.SetColumn(i, Concatenate(_Y[i].rotation.GetInverse() * kGravity, _Y[i].velocity));
    // Z.SetColumn(i, Concatenate(_Y[i].rotation * kGravity, _Y[i].velocity));
  }

  // mean center {Zᵢ}
  Point<6,float> meanZ;
  for (int i = 0; i < 2*StateDim; ++i) {
    meanZ += Z.GetColumn(i);
  }
  meanZ /= 12;
  
  for (int i = 0; i < 2*StateDim; ++i) { 
    Z.SetColumn(i, Z.GetColumn(i) - meanZ); 
  }

  // get Covariance
  const SmallSquareMatrix<6,float> Pvv = GetCovariance(Z) + _R;
  const SmallSquareMatrix<6,float> Pxz = GetCovariance(_W, Z.GetTranspose());

  // get Kalman gain and update covariance
  const auto K = Pxz * Pvv.CopyInverse();
  _P -= K * Pvv * K.GetTranspose();

  // get measurement residual and update state
  const auto residual = K*(measurement - meanZ);
  _state.rotation *= ErrorToQuat({residual[0], residual[1], residual[2]});
  _state.velocity += {residual[3], residual[4], residual[5]};
}
      
} // namespace Vector
} // namespace Anki
