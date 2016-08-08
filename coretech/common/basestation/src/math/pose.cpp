
#include "anki/common/basestation/math/pose.h"

#include "anki/common/basestation/math/matrix_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/poseOriginList.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/common/shared/utilities_shared.h"

#include <stdexcept>
#include <cmath>

namespace Anki {

  
#pragma mark --- Pose2d Implementations ---
  /*
  template<>
  std::list<Pose2d> PoseBase<Pose2d>::Origins(1); // TODO: Name it "WorldOrigin2D"

  template<class PoseNd>
  const Pose3d* PoseBase<PoseNd>::_sWorld = &PoseBase<PoseNd>::Origins.front();
  */
  
  Pose2d::Pose2d()
  : Pose2d(0, {0.f, 0.f})
  {
    
  }
  
  Pose2d::Pose2d(const Radians &theta, const Point2f &t)
  : _translation(t)
  , _angle(theta)
  , _planeNormal(Z_AXIS_3D())
  {
    
  }
  
  Pose2d::Pose2d(const Radians &theta, const float x, const float y)
  : _translation(x,y)
  , _angle(theta)
  , _planeNormal(Z_AXIS_3D())
  {
    
  }
  
  Pose2d::Pose2d(const Pose3d& pose3d)
  : _translation(pose3d.GetTranslation().x(), pose3d.GetTranslation().y())
  , _angle(pose3d.GetRotationAngle<'Z'>())
  , _planeNormal(Z_AXIS_3D())
  {
    
  }

  void Pose2d::TranslateBy(float dist)
  {
    _translation.x() += dist * cosf(_angle.ToFloat());
    _translation.y() += dist * sinf(_angle.ToFloat());
  }

  void Pose2d::operator*=(const Pose2d &other)
  {
    _angle += other._angle;
    
    _translation += this->GetRotationMatrix() * other._translation;
    
  }
  
  Pose2d Pose2d::operator* (const Pose2d &other) const
  {
    Radians newAngle(_angle + other._angle);
    RotationMatrix2d newRotation(newAngle);
    Point2f newTranslation(GetRotationMatrix() * other._translation);
    newTranslation += _translation;
    
    return Pose2d(newAngle, newTranslation);
  }

  Point2f Pose2d::operator* (const Point2f &point) const
  {
    Point2f pointOut( GetRotationMatrix() * point);
    pointOut += _translation;
    
    return pointOut;
  }

  void Pose2d::PreComposeWith(const Pose2d &other)
  {
    _angle += other._angle;

    _translation = other.GetRotationMatrix() * _translation;
    _translation += other._translation;
  }
  
  Pose2d Pose2d::GetInverse(void) const
  {
    Pose2d returnPose(*this);
    returnPose.Invert();
    return returnPose;
  }
  
  Pose2d& Pose2d::Invert(void)
  {
    _angle *= -1.f;
    RotationMatrix2d R(_angle);

    _translation *= -1.f;
    _translation = R * _translation;
    
    return *this;
  }

  void Pose2d::Print() const
  {
    CoreTechPrint("Pose2d%s%s: Translation=(%f, %f), RotAng=%frad (%.1fdeg), parent 0x%x;%s\n",
                  GetName().empty() ? "" : " ", GetName().c_str(),
                  _translation.x(), _translation.y(),
                  _angle.ToFloat(), _angle.getDegrees(),
                  GetParent(),
                  GetParent() != nullptr ? GetParent()->GetName().c_str() : "NULL"
                  );
  }
    
  
#pragma mark --- Pose3d Implementations ---
  
  /*
  template<>
  std::list<Pose3d> PoseBase<Pose3d>::Origins = {{Pose3d(0, Z_AXIS_3D(), {{0,0,0}}, nullptr, "WorldOrigin3D")}};
  
  template<>
  const Pose3d* PoseBase<Pose3d>::_sWorld = &PoseBase<Pose3d>::Origins.front();
  */
  
  Pose3d::Pose3d()
  : Pose3d(0, Z_AXIS_3D(), {0.f, 0.f, 0.f})
  {
    
  } // Constructor: Pose3d()  
  
  Pose3d::Pose3d(const Rotation3d& R, const Vec3f& T,
                 const Pose3d* parentPose,
                 const std::string& name)
  : PoseBase<Pose3d>(parentPose, name)
  , _rotation(R)
  , _translation(T)
  {
    
  }
  
  Pose3d::Pose3d(const RotationVector3d &Rvec_in, const Vec3f &T_in, const Pose3d *parentPose, const std::string& name)
  : Pose3d(Rotation3d(Rvec_in), T_in, parentPose, name)
  {

  } // Constructor: Pose3d(Rvec, T)
  
  /* TODO: Add this constructor once we're using covariance
  Pose3d::Pose3d(const RotationVector3d &Rvec_in, const Vec3f &T_in,
             const Matrix<float> &cov_in, const Pose3d *parentPose)
  : Pose3d(Rvec_in, T_in, parentPose)
  {
    covariance = cov_in;
  } // Constructor: Pose3d(Rvec, T, cov)
  */
  
  Pose3d::Pose3d(const RotationMatrix3d &Rmat_in, const Vec3f &T_in, const Pose3d *parentPose, const std::string& name)
  : Pose3d(Rotation3d(Rmat_in), T_in, parentPose, name)
  {

  } // Constructor: Pose3d(Rmat, T)
  
  Pose3d::Pose3d(const Radians &angle, const Vec3f &axis,
                 const Vec3f &T, const Pose3d *parentPose, const std::string& name)
  : Pose3d(Rotation3d(angle, axis), T, parentPose, name)
  {
    
  } // Constructor: Pose3d(angle, axis, T)
  
  Pose3d::Pose3d(const Pose3d &otherPose)
  : Pose3d(otherPose._rotation, otherPose._translation, otherPose.GetParent()) // NOTE: *not* copying name
  {
    
  }
  
  Pose3d::Pose3d(const Pose2d &pose2d)
  : Pose3d(pose2d.GetAngle(), {0.f, 0.f, 1.f},
           {pose2d.GetX(), pose2d.GetY(), 0.f})
  {
    // At this point, we have initialized a 3D pose corresponding
    // just to the 2D pose (i.e. w.r.t. the (0,0,0) origin and
    // with rotation around the Z axis).
    //
    // Next compute the 3D pose of the plane in which the 2D Pose
    // is embedded.  Then compose the two poses to get the final
    // 3D pose.
    
    const Vec3f Zaxis(0.f, 0.f, 1.f);
    
    const float dotProduct = DotProduct(pose2d.GetPlaneNormal(), Zaxis);
    
    CORETECH_ASSERT(std::abs(dotProduct) <= 1.f);
    
    // We will rotate the pose2d's plane normal into the Z axis.
    // The dot product gives us the angle needed to do that, and
    // the cross product gives us the axis around which we will
    // rotate.
    Radians angle3d = std::acos(dotProduct);

    if( ! NEAR_ZERO( angle3d ) ) {
      Vec3f axis3d = CrossProduct(Zaxis, pose2d.GetPlaneNormal());
      Pose3d planePose(angle3d, axis3d, pose2d.GetPlaneOrigin());
      this->PreComposeWith(planePose);
    }
    // else we are already in the correct plane
    
  } // Constructor: Pose3d(Pose2d)

  Pose3d::Pose3d(const PoseStruct3d& poseStruct, const PoseOriginList& originList)
  : Pose3d(Rotation3d(UnitQuaternion<f32>(poseStruct.q0, poseStruct.q1, poseStruct.q2, poseStruct.q3)),
           Vec3f(poseStruct.x, poseStruct.y, poseStruct.z),
           originList.GetOriginByID(poseStruct.originID))
  {
    
  }
  
  PoseStruct3d Pose3d::ToPoseStruct3d(const PoseOriginList& originList) const
  {
    // Must flatten the pose before making it a PoseStruct3d because we are about
    // to lose all the information along the chain from the pose up to its origin
    const Pose3d flattenedPose = GetWithRespectToOrigin();
    
    const Vec3f& T = flattenedPose.GetTranslation();
    const UnitQuaternion<f32>& Q = flattenedPose.GetRotation().GetQuaternion();
    
    PoseStruct3d poseStruct(T.x(), T.y(), T.z(), Q.w(), Q.x(), Q.y(), Q.z(),
                            originList.GetOriginID(&FindOrigin()));
    
    return poseStruct;
  }
  
#pragma mark --- Operator Overloads ---
  // Composition: this = this*other
  void Pose3d::operator*=(const Pose3d &other)
  {
   
    // this.T = this.R*other.T + this.T;
    Vec3f thisTranslation(_translation); // temp storage
    _translation = _rotation * other._translation;
    _translation += thisTranslation;

    // this.R = this.R * other.R
    // Note: must do this _after_ the translation update, since that uses this.R
    _rotation *= other._rotation;

  } // operatore*=(Pose3d)
  
  // Composition: new = this * other
  Pose3d Pose3d::operator*(const Pose3d &other) const
  {
    Vec3f newTranslation = _rotation * other._translation;
    newTranslation += _translation;
    
    Rotation3d newRotation(_rotation);
    newRotation *= other._rotation;
    
    return Pose3d(newRotation, newTranslation);
  } // operator*(Pose3d)
  
  
  bool Pose3d::operator==(const Pose3d &other) const
  {
    return (_rotation == other._rotation &&
            _translation == other._translation);
  }
  
  // Composition: this = other * this
  void Pose3d::PreComposeWith(const Pose3d &other)
  {
    _rotation.PreMultiplyBy(other._rotation);
    _translation = other._rotation * _translation;
    _translation += other._translation;
  }
  
  
#pragma mark --- Member Methods ---
  Pose3d Pose3d::GetInverse(void) const
  {
    Pose3d returnPose(this->GetRotation(), this->GetTranslation());
    returnPose.Invert();
    return returnPose;
  }
  
  Pose3d& Pose3d::Invert(void)
  {
    _rotation.Invert();
    _translation *= -1.f;
    _translation = _rotation * _translation;
    SetParent(nullptr);
    
    return *this;
  }
  
  void Pose3d::RotateBy(const Rotation3d& R)
  {
    _translation = R * _translation;
    _rotation *= R;
  }
  
  void Pose3d::RotateBy(const Radians& angleIn)
  {
    // Keep same rotation axis, but add the incoming angle
    Rotation3d Rnew(angleIn, _rotation.GetAxis());
    _translation = Rnew * _translation;
    Rnew *= _rotation;
    SetRotation(Rnew);
  }
  
  void Pose3d::RotateBy(const RotationVector3d& Rvec)
  {
    Rotation3d Rnew(Rvec);
    _translation = Rnew * _translation;
    Rnew *= _rotation;
    SetRotation(Rnew);
  }
  
  void Pose3d::RotateBy(const RotationMatrix3d& Rmat)
  {
    Rotation3d R(Rmat);
    _translation = R * _translation;
    _rotation.PreMultiplyBy(R);
  }


  
  /*
  unsigned int Pose2d::GetTreeDepth(void) const
  {
    return getTreeDepthHelper<Pose2d>(this);
  } // GetTreeDepth()

  unsigned int Pose3d::GetTreeDepth(void) const
  {
    return getTreeDepthHelper<Pose3d>(this);
  } // GetTreeDepth()
  */
  
  
  
  /*
  Pose2d Pose2d::GetWithRespectTo(const Anki::Pose2d *otherPose) const
  {
    return GetWithRespectToHelper<Pose2d>(this, otherPose);
  }
  
  Pose3d Pose3d::GetWithRespectTo(const Anki::Pose3d *otherPose) const
  {
    return GetWithRespectToHelper<Pose3d>(this, otherPose);
  }
   */
  
  bool Pose3d::IsSameAs(const Pose3d&  P_other,
                        const Point3f& distThreshold,
                        const Radians& angleThreshold,
                        Vec3f& T_diff,
                        Radians& angleDiff) const
  {
    
    const std::vector<RotationMatrix3d> R_ambiguities;
    
    return IsSameAs_WithAmbiguity(P_other,
                                  R_ambiguities,
                                  distThreshold,
                                  angleThreshold,
                                  false, // doesn't really matter with no ambiguities
                                  T_diff,
                                  angleDiff);
    
  } // IsSameAs()
  
  
  bool Pose3d::IsSameAs_WithAmbiguity(const Pose3d& P_other_in,
                                      const std::vector<RotationMatrix3d>& R_ambiguities,
                                      const Point3f&   distThreshold,
                                      const Radians&   angleThreshold,
                                      const bool       useAbsRotation,
                                      Vec3f& Tdiff,
                                      Radians& angleDiff) const
  {
    ASSERT_NAMED(distThreshold >= 0.f,
                 "Pose3d.IsSameAs_WithAmbiguity.NegativeDistThreshold");
    
    ASSERT_NAMED(angleThreshold.ToFloat() >= 0.f,
                 "Pose3d.IsSameAs_WithAmbiguity.NegativeAngleThreshold");
    
    Pose3d P_other;
    
    if(&P_other_in == this->GetParent())
    {
      // Other pose is this pose's parent, leave otherPose as default
      // identity transformation and hook up parent connection.
      P_other.SetParent(&P_other_in);
    }
    
    else if(this->IsOrigin())
    {
      // This pose is an origin, GetParent() will be null, so we can't
      // dereference it below to make them have the same parent.  So try
      // to get other pose w.r.t. this origin.  If the other pose is the
      // same as an origin pose (which itself is an identity transformation)
      // then the remaining transformation should be the identity.
      if(P_other_in.GetWithRespectTo(*this, P_other) == false) {
        PRINT_NAMED_WARNING("Pose3d.IsSameAs.PosesHaveDifferentOrigins",
                            "Could not get other pose w.r.t. this pose. Returning not same.");
        return false;
      }
    }
    
    // Otherwise, attempt to make the two poses have the same parent so they
    // are comparable
    else if(P_other_in.GetWithRespectTo(*this->GetParent(), P_other) == false)
    {
      // If this fails, the poses must not have the same origin, so we cannot
      // determine sameness.
      
      // No need to warn: this can easily happen after the robot gets kidnapped
      //  PRINT_NAMED_WARNING("Pose3d.IsSameAs.ObjectsHaveDifferentOrigins",
      //                      "Could not get other object w.r.t. this object's parent. Returning isSame == false.\n");
      
      return false;
    }

    // P_this represents the canonical/reference pose after some arbitrary
    // transformation, T:
    //    P_this = T * P_ref
    //
    // If P_other is another ambigously-rotated version of this canonical pose
    // that has undergone the same transformation, T, then it is:
    //    P_other = T * [R_amb | 0] * P_ref
    //
    // So we compute P_diff = inv(P_this) * P_other.  If the above is true, then
    // P_diff is equivalent to:
    //    P_diff = inv(P_ref) * inv(T) * T * P_amb * P_ref
    //           = inv(P_ref) * [R_amb | 0] * P_ref
    //
    // Without loss of generality, we can assume P_ref is the identity
    // transformation (or, equivalently, that the input poses -- this and other
    // have been pre-adjusted by inv(P_ref) before calling this function). In
    // that case, P_diff = [R_amb | 0].  
    //
    
    /*
    P_diff = this->GetInverse();
    P_diff *= P_other;
    
    // First, check to see if the translational difference between the two
    // poses is small enough to call them a match
    if(P_diff.GetTranslation().GetAbs() < distThreshold) {
      
      // Next check to see if the rotational difference is small
      RotationVector3d Rvec(P_diff.GetRotationMatrix());
      if(Rvec.GetAngle() < angleThreshold) {
        // Rotation is same, without even considering the ambiguities
        isSame = true;
      }
      else {
        // Need to consider ambiguities...
        
        RotationMatrix3d R(P_diff.GetRotationMatrix());
        
        if(useAbsRotation) {
          // The ambiguities are assumed to be defined up various sign flips
          R.Abs();
        }
        
        // Check to see if the rotational part of the pose difference is
        // similar enough to one of the rotational ambiguities
        for(auto R_ambiguity : R_ambiguities) {
          if(R.GetAngleDiffFrom(R_ambiguity) < angleThreshold) {
            isSame = true;
            break;
          }
        }
      }
    } // if translation component is small enough
    */
    
    // Simpler (?) version:
    // Just directly compare the translations, followed by comparing the
    // rotation matrices.
    // Why is this better?!
    
    
    ASSERT_NAMED(P_other.GetParent() == this->GetParent() ||
                 (this->IsOrigin() && P_other.GetParent() == this),
                 "Pose3d.IsSameAs.BadParents");
    
    Tdiff = P_other.GetTranslation() - this->GetTranslation();
    
    // NOTE: using > instead of >= so threshold of zero will still return true for identical poses
    if(Tdiff.GetAbs().AnyGT(distThreshold))
    {
      return false;
    }
    
    if(angleThreshold >= M_PI)
    {
      // No need to check the rotation (can't be more than 180 degrees0
      return true;
    }
    
    // Next check to see if the rotational difference is small
    angleDiff = this->GetRotation().GetAngleDiffFrom(P_other.GetRotation());
    
    // NOTE: using <= instead of < so threshold of zero still will still return true for identical poses
    if(angleDiff <= angleThreshold)
    {
      // Rotation is same, without even considering the ambiguities
      return true;
    }
    
    if(!R_ambiguities.empty())
    {
      // Need to consider ambiguities...
      Rotation3d Rdiff(this->GetRotation());
      Rdiff.Invert(); // Invert
      Rdiff *= P_other.GetRotation();
      
      // TODO: Does this directly with quaternions instead of converting to RotationMatrix
      RotationMatrix3d RdiffMat( Rdiff.GetRotationMatrix() );
      
      if(useAbsRotation) {
        // The ambiguities are assumed to be defined up various sign flips
        RdiffMat.Abs();
      }
      
      // Check to see if the rotational part of the pose difference is
      // similar enough to one of the rotational ambiguities
      for(const auto& R_ambiguity : R_ambiguities) {
        if(RdiffMat.GetAngleDiffFrom(R_ambiguity) < angleThreshold) {
          return true;
        }
      }
    } // if(!R_ambiguities.empty())
    
    // If we get here, we didn't pass some check above, so poses are not the same
    return false;

  } // IsSameAs_WithAmbiguity()


  std::string Pose3d::GetNamedPathToOrigin(bool showTranslations) const
  {
    return PoseBase<Pose3d>::GetNamedPathToOrigin(*this, showTranslations);
  }
  
  void Pose3d::PrintNamedPathToOrigin(bool showTranslations) const
  {
    PoseBase<Pose3d>::PrintNamedPathToOrigin(*this, showTranslations);
  }
  
  void Pose3d::Print() const
  {
    PRINT_NAMED_INFO("Pose3d.Print", "Pose%s%s: Translation=(%f, %f %f), RotVec=(%f, %f, %f), RotAng=%frad (%.1fdeg), parent 0x%p;%s\n",
                  GetName().empty() ? "" : " ", GetName().c_str(),
                  _translation.x(), _translation.y(), _translation.z(),
                  GetRotationAxis().x(), GetRotationAxis().y(), GetRotationAxis().z(),
                  GetRotationAngle().ToFloat(), GetRotationAngle().getDegrees(),
                  GetParent(),
                  GetParent() != nullptr ? GetParent()->GetName().c_str() : "NULL"
                  );
  }
  
  
#pragma mark --- Global Functions ---
  
  Vec3f ComputeVectorBetween(const Pose3d& pose1, const Pose3d& pose2)
  {
    // Make sure the two poses share a common parent:
    
    if(pose1.GetParent() != pose2.GetParent()) {
      Pose3d pose1wrt2;
      if(false == pose1.GetWithRespectTo(pose2, pose1wrt2)) {
        PRINT_NAMED_ERROR("ComputeVectorBetween.NoCommonParent",
                          "Could not get pose1 w.r.t. pose2.");
        return Vec3f(0.f,0.f,0.f);
      }
      return pose1wrt2.GetTranslation();
    }
    
    // Compute distance between the two poses' translation vectors
    // TODO: take rotation into account?
    Vec3f distVec(pose1.GetTranslation());
    distVec -= pose2.GetTranslation();
    return distVec;
  }

  f32 ComputeEuclidianDistanceBetween(const Pose3d& pose1, const Pose3d& pose2)
  {
    // Make sure the two poses share a common parent:
    
    if(pose1.GetParent() != pose2.GetParent()) {
      Pose3d pose1wrt2;
      if(false == pose1.GetWithRespectTo(pose2, pose1wrt2)) {
        PRINT_NAMED_ERROR("ComputeVectorBetween.NoCommonParent",
                          "Could not get pose1 w.r.t. pose2.");
        return Vec3f(0.f,0.f,0.f).Length();
      }
      return pose1wrt2.GetTranslation().Length();
    }
    
    
    //Compute the euclidian distance
    double xDiff =  pow(pose1.GetTranslation().x() - pose2.GetTranslation().x(), 2);
    double yDiff =  pow(pose1.GetTranslation().y() - pose2.GetTranslation().y(), 2);
    double zDiff =  pow(pose1.GetTranslation().z() - pose2.GetTranslation().z(), 2);
    
    return sqrt(xDiff + yDiff + zDiff);

  }
  
} // namespace Anki
