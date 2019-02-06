
#include "coretech/common/engine/math/pose.h"

#include "coretech/common/shared/math/matrix_impl.h"
#include "coretech/common/engine/math/poseBase_impl.h"
#include "coretech/common/engine/math/poseOriginList.h"
#include "coretech/common/engine/math/quad_impl.h"

#include "coretech/common/shared/utilities_shared.h"

#include "util/math/math.h"

#include <stdexcept>
#include <cmath>

namespace Anki {
  
#pragma mark -
#pragma mark Pose2d Implementations
  
  // Explicit instantiation of 2d and 3d poses
  template class PoseBase<Pose2d,Transform2d>;
  template class PoseBase<Pose3d,Transform3d>;
  
  Pose2d::Pose2d()
  : Pose2d(0, {0.f, 0.f})
  {
    
  }
  
  Pose2d::Pose2d(const Transform2d& transform)
  : PoseBase<Pose2d,Transform2d>(transform)
  {
    
  }
  
  Pose2d::Pose2d(const Radians &theta, const Point2f &t)
  : PoseBase<Pose2d,Transform2d>(Transform2d(theta,t))
  , _planeNormal(Z_AXIS_3D())
  {
    
  }
  
  Pose2d::Pose2d(const Radians &theta, const float x, const float y)
  : PoseBase<Pose2d,Transform2d>(Transform2d(theta,{x,y}))
  , _planeNormal(Z_AXIS_3D())
  {
    
  }
  
  Pose2d::Pose2d(const Pose3d& pose3d)
  : PoseBase<Pose2d,Transform2d>(Transform2d(pose3d.GetRotationAngle<'Z'>(),
                                             {pose3d.GetTranslation().x(), pose3d.GetTranslation().y()}))
  , _planeNormal(Z_AXIS_3D())
  {
    
  }
  
  void Pose2d::Print(const std::string& channel, const std::string& eventName) const
  {
    PRINT_CH_INFO(channel.c_str(), eventName.c_str(),
                  "Pose2d%s%s: Translation=%s, RotAng=%frad (%.1fdeg), %s",
                  GetName().empty() ? "" : " ", GetName().c_str(),
                  GetTranslation().ToString().c_str(),
                  GetAngle().ToFloat(), GetAngle().getDegrees(),
                  GetParentString().c_str());
  }
    
  Pose2d operator* (const Pose2d& pose1, const Pose2d& pose2)
  {
    Pose2d newPose(pose1);
    newPose *= pose2;
    return newPose;
  }
  
#pragma mark -
#pragma mark Pose3d Implementations
  
  Pose3d::Pose3d(const std::string& name)
  : Pose3d(0, Z_AXIS_3D(), {0.f, 0.f, 0.f}, name)
  {
    
  } // Constructor: Pose3d()
  
  Pose3d::Pose3d(const Transform3d& transform, const std::string& name)
  : PoseBase<Pose3d,Transform3d>(transform, name)
  {
    
  }
  
  Pose3d::Pose3d(const Transform3d& transform, const Pose3d& parentPose, const std::string& name)
  : PoseBase<Pose3d,Transform3d>(transform, parentPose, name)
  {
    
  }
  
  Pose3d::Pose3d(const Rotation3d& R, const Vec3f& T,
                 const std::string& name)
  : PoseBase<Pose3d,Transform3d>(Transform3d(R,T), name)
  {
    
  }
  
  Pose3d::Pose3d(const Rotation3d& R, const Vec3f& T, const Pose3d& parentPose, const std::string& name)
  : PoseBase<Pose3d,Transform3d>(Transform3d(R,T), parentPose, name)
  {
    
  }
  
  Pose3d::Pose3d(const RotationVector3d& Rvec, const Vec3f& T, const std::string& name)
  : Pose3d(Rotation3d(Rvec), T, name)
  {
    
  }
  
  Pose3d::Pose3d(const RotationVector3d& Rvec, const Vec3f& T, const Pose3d& parentPose, const std::string& name)
  : Pose3d(Rotation3d(Rvec), T, parentPose, name)
  {

  }
  
  Pose3d::Pose3d(const RotationMatrix3d& Rmat, const Vec3f& T, const std::string& name)
  : Pose3d(Rotation3d(Rmat), T, name)
  {
    
  }
  
  Pose3d::Pose3d(const RotationMatrix3d& Rmat, const Vec3f& T, const Pose3d& parentPose, const std::string& name)
  : Pose3d(Rotation3d(Rmat), T, parentPose, name)
  {

  }
  
  Pose3d::Pose3d(const Radians& angle, const Vec3f& axis, const Vec3f& T, const std::string& name)
  : Pose3d(Rotation3d(angle, axis), T, name)
  {
    
  }

  Pose3d::Pose3d(const Radians& angle, const Vec3f& axis, const Vec3f& T, const Pose3d& parentPose, const std::string& name)
  : Pose3d(Rotation3d(angle, axis), T, parentPose, name)
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

    if( ! Util::IsFltNear(angle3d.ToFloat(), 0.f) ) {
      Vec3f axis3d = CrossProduct(Zaxis, pose2d.GetPlaneNormal());
      Pose3d planePose(angle3d, axis3d, pose2d.GetPlaneOrigin());
      this->PreComposeWith(planePose);
    }
    // else we are already in the correct plane
    
  } // Constructor: Pose3d(Pose2d)

  Pose3d::Pose3d(const PoseStruct3d& poseStruct, const PoseOriginList& originList)
  : Pose3d(Rotation3d(UnitQuaternion(poseStruct.q0, poseStruct.q1, poseStruct.q2, poseStruct.q3)),
           Vec3f(poseStruct.x, poseStruct.y, poseStruct.z),
           originList.GetOriginByID(poseStruct.originID))
  {
    
  }
  
  PoseStruct3d Pose3d::ToPoseStruct3d(const PoseOriginList& originList) const
  {
    // Must flatten the pose before making it a PoseStruct3d because we are about
    // to lose all the information along the chain from the pose up to its origin
    const Pose3d flattenedPose = GetWithRespectToRoot();
    
    const PoseOriginID_t originID = flattenedPose.GetRootID();
    ANKI_VERIFY(originList.ContainsOriginID(originID),
                "Pose3d.ToPoseStruct3d.UnknownOrigin", "ID:%d", originID);
    
    const Vec3f& T = flattenedPose.GetTranslation();
    const UnitQuaternion& Q = flattenedPose.GetRotation().GetQuaternion();
    
    ANKI_VERIFY(IsRoot() || flattenedPose.GetParent().GetID() == originID,
                "Pose3d.ToPoseStruct3d.BadParent", "ParentID:%d OriginID:%d",
                flattenedPose.GetParent().GetID(), originID);
    
    PoseStruct3d poseStruct(T.x(), T.y(), T.z(), Q.w(), Q.x(), Q.y(), Q.z(), originID);
    
    return poseStruct;
  }
  
  bool Pose3d::IsSameAs(const Pose3d&  P_other,
                        const Point3f& distThreshold,
                        const Radians& angleThreshold,
                        Vec3f& T_diff,
                        Radians& angleDiff) const
  {
    
    static const RotationAmbiguities kNoAmbiguities;
    
    return IsSameAs_WithAmbiguity(P_other,
                                  kNoAmbiguities,
                                  distThreshold,
                                  angleThreshold,
                                  T_diff,
                                  angleDiff);
    
  } // IsSameAs()
  
  
  bool Pose3d::IsSameAs_WithAmbiguity(const Pose3d& P_other_in,
                                      const RotationAmbiguities& R_ambiguities,
                                      const Point3f&   distThreshold,
                                      const Radians&   angleThreshold,
                                      Vec3f& Tdiff,
                                      Radians& angleDiff) const
  {
    DEV_ASSERT(distThreshold.AllGTE( 0.f ), "Pose3d.IsSameAs_WithAmbiguity.NegativeDistThreshold");
    
    DEV_ASSERT(angleThreshold.ToFloat() >= 0.f, "Pose3d.IsSameAs_WithAmbiguity.NegativeAngleThreshold");
    
    if(&P_other_in == this){
      return true;
    }

    Pose3d P_other;

    if(this->IsChildOf(P_other_in))
    {
      // Other pose is this pose's parent, leave otherPose as default
      // identity transformation and hook up parent connection.
      P_other.SetParent(P_other_in);
    }
    
    else if(this->IsRoot())
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
    else if(P_other_in.GetWithRespectTo(this->GetParent(), P_other) == false)
    {
      // If this fails, the poses must not have the same origin, so we cannot
      // determine sameness.
      
      // No need to warn: this can easily happen after the robot gets kidnapped
      //  PRINT_NAMED_WARNING("Pose3d.IsSameAs.ObjectsHaveDifferentOrigins",
      //                      "Could not get other object w.r.t. this object's parent. Returning isSame == false.");
      
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
     // P_diff is the transformation which takes us from P1 to P2.
     // If this transformation is "small" (in translation and rotation), then
     // P1 and P2 are the same.
     //   P2 = P_diff * P1
     //   P_diff = P2 * inv(P1)
     Pose3d P_diff(P_other);
     P_diff *= this->GetInverse();
    
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
    
    // Problem with the above, seemingly-simple check: rotation affects the translation
    // comparison. So if it is large, it will make the translations appear different
    // _even if we have a large angle threshold_. Instead, we choose to just
    // directly compare the translations, followed by comparing the rotations.
    
    DEV_ASSERT(this->HasSameParentAs(P_other) ||
               (this->IsRoot() && this->IsParentOf(P_other)),
               "Pose3d.IsSameAs.BadParents");
    
    Tdiff = P_other.GetTranslation() - this->GetTranslation();
    
    if(Util::IsFltNear(distThreshold.x(), distThreshold.y()) &&
       Util::IsFltNear(distThreshold.x(), distThreshold.z()))
    {
      // Use simple euclidean distance
      if(Tdiff.LengthSq() > distThreshold.x()*distThreshold.x())
      {
        return false;
      }
    }
    else
    {
      // Compare "unrotated" Tdiff to distThreshold which is specified in the
      // rotated frame of "this" pose
      // NOTE: using > instead of >= so threshold of zero will still return true for identical poses
      const Point3f Tdiff_unrotated = this->GetRotation().GetInverse()*Tdiff;
      if(Tdiff_unrotated.GetAbs().AnyGT(distThreshold))
      {
        return false;
      }
    }
    
    if(angleThreshold >= M_PI)
    {
      // No need to check the rotation (can't be more than 180 degrees)
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
    
    if(R_ambiguities.HasAmbiguities())
    {
      // Need to consider ambiguities. Compute the rotation difference between the two poses' rotations.
      // See if that difference is one of the ambiguities we should ignore.
      Rotation3d Rdiff(this->GetRotation());
      Rdiff.Invert(); // Invert
      Rdiff *= P_other.GetRotation();
      
      return R_ambiguities.IsRotationSame(Rdiff, angleThreshold);
    }
    
    // If we get here, we didn't pass some check above, so poses are not the same
    return false;

  } // IsSameAs_WithAmbiguity()

  void Pose3d::Print(const std::string& channel, const std::string& eventName) const
  {
    PRINT_CH_INFO(channel.c_str(), eventName.c_str(),
                  "Pose%s%s: Translation=%s, RotVec=(%f, %f, %f), RotAng=%frad (%.1fdeg), %s",
                  GetName().empty() ? "" : " ", GetName().c_str(),
                  GetTranslation().ToString().c_str(),
                  GetRotationAxis().x(), GetRotationAxis().y(), GetRotationAxis().z(),
                  GetRotationAngle().ToFloat(), GetRotationAngle().getDegrees(),
                  GetParentString().c_str());
  }
  
#pragma mark -
#pragma mark Global Functions

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool ComputeVectorBetween(const Pose3d& pose1, const Pose3d& pose2, const Pose3d& outputFrame, Vec3f& outVector)
  {
    Pose3d pose1wrtOutputFrame;
    Pose3d pose2wrtOutputFrame;
    
    if (!pose1.GetWithRespectTo(outputFrame, pose1wrtOutputFrame) ||
        !pose2.GetWithRespectTo(outputFrame, pose2wrtOutputFrame)) {
      return false;
    }

    outVector = (pose1wrtOutputFrame.GetTranslation() - pose2wrtOutputFrame.GetTranslation());
    return true;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool ComputeDistanceBetween(const Pose3d& pose1, const Pose3d& pose2, f32& outDistance)
  {
    Pose3d pose1Wrt2;
    if (!pose1.GetWithRespectTo(pose2, pose1Wrt2)) {
      return false;
    }
    
    outDistance = pose1Wrt2.GetTranslation().Length();
    return true;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool ComputeDistanceSQBetween(const Pose3d& pose1, const Pose3d& pose2, f32& outDistanceSQ)
  {
    Pose3d pose1Wrt2;
    if (!pose1.GetWithRespectTo(pose2, pose1Wrt2)) {
      return false;
    }
    
    outDistanceSQ = pose1Wrt2.GetTranslation().LengthSq();
    return true;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Pose3d operator* (const Pose3d& pose1, const Pose3d& pose2)
  {
    Pose3d newPose(pose1);
    newPose *= pose2;
    return newPose;
  }
  
} // namespace Anki
