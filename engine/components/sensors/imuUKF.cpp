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


// #include <cmath>

#define StateDim 6
#define StateDimSq 36

namespace Anki {
namespace Vector {

namespace {
  // calculates the decomposition of a positive definite n x n matrix A s.t. A = L' L    
  template<MatDimType N>
  SmallSquareMatrix<N,float> Cholesky(const SmallSquareMatrix<N,float>& A) {
    SmallSquareMatrix<N,float> L;

    for (size_t row = 0; row < N; ++row) {
      for (size_t col = 0; col <= row; ++col) {
          float s = 0;
          for (size_t k = 0; k < col; ++k) {
              s += L(col, k) * L(row, k);
          }
          L(row, col) = (row == col) ? sqrt(A(row, row) - s) : (1.0 / L(col, col) * (A(row, col) - s));
      }
    }
    return L;
  }
  
  
  // transforms rotation error vector into a quaternion
  UnitQuaternion ErrorToQuat(const ErrorVector& v, float dt = 1.f) {
    // TODO: double check the math here vs. Paper
    float alpha = sqrt(v.LengthSq());
    if ( NEAR_ZERO(alpha) ) {
      return UnitQuaternion();
    } else {
      UnitQuaternion retv;
      float scale = sin(alpha*dt / 2) / alpha;
      retv.w() = cos(alpha*dt / 2);
      retv.x() = v.x()*scale;
      retv.y() = v.y()*scale;
      retv.z() = v.z()*scale;

      retv.Normalize();
      return retv;
    }
  };
  
  // transforms a quaternion into a rotation error vector
  ErrorVector QuaternionToError(const UnitQuaternion& q) {

    float sinAlpha = sqrt(q.x()*q.x() + q.y()*q.y() + q.z()*q.z());

    if ( NEAR_ZERO(sinAlpha) ) {
      return {0.f, 0.f, 0.f};
    } else {
      // Numerically more stable than acos
      float alpha = atan2(sinAlpha, q.w()) * 2 / sinAlpha;

      return {q.x() * alpha, q.y() * alpha, q.z() * alpha};
    }
  };

  template<MatDimType N>
  SmallMatrix<7,N, float> ToMatrix(const std::array<ImuUKF::State,N>& states) {
    SmallMatrix<7,N,float> retv;
    for (int i = 0; i < N; ++i) {
      retv(0,i) = states[i].rotation.w();
      retv(1,i) = states[i].rotation.x();
      retv(2,i) = states[i].rotation.y();
      retv(3,i) = states[i].rotation.z();
      retv(4,i) = states[i].velocity.x();
      retv(5,i) = states[i].velocity.y();
      retv(6,i) = states[i].velocity.z();
    }
    return retv;
  }
    
  template<MatDimType N>
  SmallMatrix<6,N,float> ToMatrix(const std::array<ImuUKF::Noise,N>& states) {
    SmallMatrix<6,N,float> retv;
    for (int i = 0; i < N; ++i) {
      retv(0,i) = states[i].rotationErr.x();
      retv(1,i) = states[i].rotationErr.y();
      retv(2,i) = states[i].rotationErr.z();
      retv(3,i) = states[i].velocityErr.x();
      retv(4,i) = states[i].velocityErr.y();
      retv(5,i) = states[i].velocityErr.z();
    }
    return retv;
  }
  template<MatDimType M, MatDimType N>
  SmallSquareMatrix<M,float> GetCovariance(const SmallMatrix<M,N,float>& A,
                                       const SmallMatrix<N,M,float>& B) {
    return (A*B) * (1.f/((float)N));
  }


  // NOTE: this may need to be negative Z?
  const Anki::UnitQuaternion kGravity(0.f, 0.f, 0.f, 1.f);
}


Point<6,float> ImuUKF::Noise::ToPoint() const { 
  Point<6,float> retv;
  retv[0] = rotationErr.x();
  retv[1] = rotationErr.y();
  retv[2] = rotationErr.z();
  retv[3] = velocityErr.x();
  retv[4] = velocityErr.y();
  retv[5] = velocityErr.z(); 
  return retv;
}

ImuUKF::ImuUKF()
: _state{}
, _lastMeasurement_ms(0)
{
  for (int i = 0; i < StateDim; ++i) {
    _P(i,i) = .01f;
    _Q(i,i) = (i < 3) ? 100.f : .1f;
    _R(i,i) = (i < 3) ? .5f : .01f;
  }
}


void ImuUKF::Update(const Point3f& accelerometer, const Point3f& gyro, RobotTimeStamp_t t_ms)
{
  // for simplicity
  const Noise measurement = {accelerometer, gyro};
  float dt_s = _lastMeasurement_ms == 0 ? .01f : ((float)(t_ms - _lastMeasurement_ms)) / 1000;

  ResetSigmaPoints();
  ProcessUpdate(dt_s);     // TODO: real data is in milliseconds...
  MeasurementUpdate(measurement.ToPoint());

  _lastMeasurement_ms = t_ms;
}

void ImuUKF::ResetSigmaPoints()
{
  // calculate the "SquareRoot Matrix" of covariance and process noise
  auto withNoise = _P;
  withNoise += _Q;
  auto S = Cholesky(withNoise);

  // calculate the set {𝑊ᵢ}
  // this is a zero-mean distribution of 2n 6D points with covariance Pk + Q
  SmallMatrix<6,12,float> W;
  const float sqrtN = sqrt(StateDim);

  for (int i = 0; i < StateDim; ++i) {
    for (int j = 0; j < StateDim; ++j) {
      W(i, j) = sqrtN * S(i,j);
      W(i, j + StateDim) = -sqrtN * S(i,j);
    }
  }

  // apply the set {𝑊ᵢ} to the current state estimate, generating the set {𝑋ᵢ}
  // of 2n State vectors
  const UnitQuaternion& q = _state.rotation;
  const AngularVelocity& omega = _state.velocity;

  for (int i = 0; i < 2*StateDim; ++i) {
    const auto Wi = W.GetColumn(i);
    _X[i].rotation = q * ErrorToQuat({Wi[0], Wi[1], Wi[2]}); 
    _X[i].velocity = omega + AngularVelocity{Wi[3], Wi[4], Wi[5]};
  }

}

void ImuUKF::ProcessUpdate(float dt_s)
{ 
  // current process model assumes we continue moving at constant velocity
  // so update the rotation according to angular rate, and leave velocity constant
  for (int i = 0; i < 2*StateDim; ++i) {
    const UnitQuaternion&      q = _X[i].rotation;
    const AngularVelocity& omega = _X[i].velocity;

    UnitQuaternion delta = ErrorToQuat(static_cast<ErrorVector>(omega), dt_s);
    _Y[i].rotation = q * delta;
    _Y[i].velocity = omega;
  }

  _state = CalculateMean(_Y);
  CalculateProcessNoise(_state);

  auto Wmat = ToMatrix(_W);
  SmallMatrix<12,6,float> Wt;
  Wmat.GetTranspose(Wt);
  _P = GetCovariance(Wmat, Wt );
}

void ImuUKF::MeasurementUpdate(const Point<6,float>& measurement)
{
  CalculateMeasurmentNoise();
  auto meanZ = GetMeanCenteredMeasurement();

  // mean center {Zᵢ}
  for (int i = 0; i < 2*StateDim; ++i) {
    _Z[i].rotationErr -= meanZ.rotationErr;
    _Z[i].velocityErr -= meanZ.velocityErr;
  }

  // get Covariance
  auto Zmat = ToMatrix(_Z);
  auto Wmat = ToMatrix(_W);
  SmallMatrix<12,6,float> Zt;
  Zmat.GetTranspose(Zt);
  auto Pvv = GetCovariance(Zmat, Zt);
  Pvv += _R;
  auto Pxz = GetCovariance(Wmat, Zt);

  // get Kalman gain
  SmallSquareMatrix<6,float> PvvInv, Kt;
  Pvv.GetInverse(PvvInv);
  SmallSquareMatrix<6,float> K = Pxz * PvvInv;

  // update covariance
  K.GetTranspose(Kt);
  _P += ((K * Pvv * Kt) * -1.f);

  // get innovation (measurement residual) and update state
  auto  innovation = K*(measurement - meanZ.ToPoint());

  _state.rotation *= ErrorToQuat({innovation[0], innovation[1], innovation[2]});
  _state.velocity += AngularVelocity{innovation[3], innovation[4], innovation[5]};

  static int i = 0;
  LOG_WARNING("UKF","%d) q:%s  w:%s", i++, _state.rotation.ToString().c_str(), _state.velocity.ToString().c_str());
}

ImuUKF::State ImuUKF::CalculateMean(const std::array<State,12>& Y_) {
  UnitQuaternion rotation = Y_[0].rotation;

  int i = 0;
  ErrorVector mean{0.f, 0.f, 0.f};
  do {
    mean = {0.f, 0.f, 0.f};
    for (int i = 0; i < 2*StateDim; ++i) {
      ErrorVector errVec = QuaternionToError(Y_[i].rotation * rotation.GetConj());
      mean += errVec;
    }
    mean /= 2*StateDim;

    rotation = ErrorToQuat(mean) * rotation;
  } while( FLT_GT(mean.LengthSq(), .0000001f) && ++i < 20);

  // mean velocity is just the barycentric mean
  AngularVelocity meanVel;
  for (int i = 0; i < 2*StateDim; ++i) {
    meanVel += Y_[i].velocity;
  }
  meanVel /= 2*StateDim;


  return {rotation, meanVel};
}

void ImuUKF::CalculateProcessNoise(const State& mean)
{
  // calculate mean centered Y and push to W_
  for (int i = 0; i < 2*StateDim; ++i) {
    // TODO: double check rotation... this seems to be backwards between paper and example impl
    _W[i].rotationErr = QuaternionToError(mean.rotation.GetConj() * _Y[i].rotation);
    _W[i].velocityErr = _Y[i].velocity - mean.velocity;
  }
}

void ImuUKF::CalculateMeasurmentNoise()
{
  // calculate mean centered Y and push to W_
  for (int i = 0; i < 2*StateDim; ++i) {
    UnitQuaternion rotatedG = _Y[i].rotation.GetConj() * kGravity * _Y[i].rotation;
    _Z[i].rotationErr = QuaternionToError(rotatedG);
    _Z[i].velocityErr = _Y[i].velocity;
  }
}

ImuUKF::Noise ImuUKF::GetMeanCenteredMeasurement()
{
  Noise meanZ;
  for (int i = 0; i < 2*StateDim; ++i) {
    meanZ.rotationErr += _Z[i].rotationErr;
    meanZ.velocityErr += _Z[i].velocityErr;
  }

  meanZ.rotationErr /= (2*StateDim);
  meanZ.velocityErr /= (2*StateDim);

  return meanZ;
}

      
} // namespace Vector
} // namespace Anki
