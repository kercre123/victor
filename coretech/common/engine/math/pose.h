/**
 * File: pose.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Implements two classes for storing Pose, meaning a rotation 
 *              translation.  
 *
 *                Pose2d stores 2D pose representing rotation
 *                in a plane with a 2D translation vector. Thus it has three
 *                degrees of freedom: x, y, and angle.
 *
 *                Pose3d stores 3D pose representing a 3D rotation and
 *                a 3D translation vector.  Thus it has six degrees of
 *                freedom: three rotation angles and x, y, z translation.
 *               
 *                Each has the ability to represent a kinematic tree using
 *                a "parent" pointer. Thus, a pose "with respect to" another
 *                pose can be requested using the GetWithRespectTo() method.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef _ANKICORETECH_MATH_POSE_H_
#define _ANKICORETECH_MATH_POSE_H_

#include "util/logging/logging.h"

#include "coretech/common/shared/math/matrix.h"
#include "coretech/common/shared/math/point_fwd.h"
#include "coretech/common/engine/math/poseBase.h"
#include "coretech/common/engine/math/quad.h"
#include "coretech/common/shared/math/rotation.h"
#include "coretech/common/engine/math/transform.h"
#include "coretech/common/shared/math/radians.h"

#include "coretech/common/engine/exceptions.h"

#include "clad/types/poseStructs.h"

#include <list>

namespace Anki {
  
  class PoseOriginList;

  // Forward declaration of Pose3d so Pose2d can use it
  class Pose3d;
  
  //
  // Pose2d Class
  //
  // Stores 2d translation and orienation in a plane.
  //
  class Pose2d : public PoseBase<Pose2d, Transform2d>
  {
  public:
   
    // Constructors:
    Pose2d();
    Pose2d(const Transform2d& transform);
    Pose2d(const Radians &angle, const Point2f &translation);
    Pose2d(const Radians &angle, const float x, const float y);
    
    // NOTE: Copy/rvalue construction and assignment inherited from base class.
    //       IDs are *not* copied, but *are* moved. Copied names will have "_COPY" appended.
    
    // Create a 2D pose from a 3D one, by keeping only the translation in the
    // XY plane and the rotation around the Z axis.
    Pose2d(const Pose3d& pose3d);

    // Accessors:
    float          GetX()           const  { return GetTransform().GetX();           }
    float          GetY()           const  { return GetTransform().GetY();           }
    const Radians& GetAngle()       const& { return GetTransform().GetAngle();       }
    const Point2f& GetTranslation() const& { return GetTransform().GetTranslation(); }
    const Point3f& GetPlaneOrigin() const& { return _planeOrigin;                    }
    const Vec3f&   GetPlaneNormal() const& { return _planeNormal;                    }
    Radians        GetAngle()       &&     { return GetTransform().GetAngle();       }
    Point2f        GetTranslation() &&     { return GetTransform().GetTranslation(); }
    Point3f        GetPlaneOrigin() &&     { return _planeOrigin;                    }
    Vec3f          GetPlaneNormal() &&     { return _planeNormal;                    }
    
    void  SetPlaneOrigin(const Point3f &origin) { _planeOrigin = origin; }
    void  SetPlaneNormal(const Vec3f   &normal) { _planeNormal = normal; _planeNormal.MakeUnitLength(); }

    // Convenience wrappers for underlying Transform2d:
    void TranslateForward(float dist)             { GetTransform().TranslateForward(dist); }
    void SetRotation(const Radians& theta)        { GetTransform().SetRotation(theta); }
    RotationMatrix2d GetRotationMatrix()    const { return GetTransform().GetRotationMatrix(); }
    Point2f operator*(const Point2f &point) const { return GetTransform() * point; }
    
    template<typename T>
    void ApplyTo(const Quadrilateral<2,T> &quadIn,
                 Quadrilateral<2,T> &quadOut) const { GetTransform().ApplyTo(quadIn, quadOut); }
    
    // Get this pose with respect to another pose.  Other pose
    // must share the same origin as this pose.  If it does not, false will be
    // returned to indicate failure.  Otherwise, true is returned and the new
    // pose is stored in newPoseWrtOtherPose.
    bool GetWithRespectTo(const Pose2d& otherPose, Pose2d& newPoseWrtOtherPose) const;
    
    // Get pose with respect to its origin
    // NOTE: This returns by value, not reference. Do not store references to its members in line! I.e., never do:
    //   const Foo& = somePose.GetWithRespectToRoot().GetFoo(); // Foo will immediately be invalidated
    // Either store "somePoseWrtRoot" and then get Foo& from it, or make Foo a copy.
    Pose2d GetWithRespectToRoot() const;

    void Print(const std::string& channel = "Pose", const std::string& eventName = "Pose3d.Print") const;
    
  protected:
    
    // Stores the orientation of this 2D plane in 3D space
    Point3f _planeOrigin;
    Vec3f   _planeNormal;
    
    // Construct from the base class
    Pose2d(const PoseBase<Pose2d,Transform2d>& poseBase)
    : PoseBase<Pose2d,Transform2d>(poseBase)
    {
      
    }
    
  }; // class Pose2d
  
  //
  // Pose3d Class
  //
  // Stores a 6DOF (Degree Of Freedom) pose: 3D translation plus
  //   3-axis rotation.
  //
  class Pose3d : public PoseBase<Pose3d,Transform3d>
  {
  public:
    
    // If initWithIdentity=false, this will be a Null pose (with no Impl).
    // In that case, you MUST intialize the transformation (Rotation/Translation) before trying to access them!
    Pose3d(const std::string& name = "");
    
    // Construct from a Transform3d, with or without parent and name
    Pose3d(const Transform3d& transform, const std::string& name = "");
    Pose3d(const Transform3d& transform, const Pose3d& parentPose, const std::string& name = "");
    
    // Construct from generic rotation and translation vector
    Pose3d(const Rotation3d& R, const Vec3f& T, const std::string& name = "");
    Pose3d(const Rotation3d& R, const Vec3f& T,
           const Pose3d& parentPose,
           const std::string& name = "");
    
    // Construct from rotation vector and translation vector
    Pose3d(const RotationVector3d &Rvec, const Vec3f &T, const std::string& name = "");
    Pose3d(const RotationVector3d &Rvec, const Vec3f &T,
           const Pose3d &parentPose,
           const std::string& name = "");
    
    // Construct from rotation matrix and translation vector
    Pose3d(const RotationMatrix3d &Rmat, const Vec3f &T, const std::string& name = "");
    Pose3d(const RotationMatrix3d &Rmat, const Vec3f &T,
           const Pose3d& parentPose,
           const std::string& name = "");
    
    // Construct from an angle, axis, and translation vector
    Pose3d(const Radians &angle, const Vec3f &axis, const Vec3f &translation, const std::string& name = "");
    Pose3d(const Radians &angle, const Vec3f &axis,
           const Vec3f &translation,
           const Pose3d& parentPose,
           const std::string& name = "");
    
    // NOTE: Copy/rvalue construction and assignment inherited from base class.
    //       IDs are *not* copied, but *are* moved. Copied names will have "_COPY" appended.
    
    // Construct a Pose3d from a Pose2d (using the plane information)
    Pose3d(const Pose2d &pose2d);
    
    // To/from CLAD PoseStruct3d
    // NOTE: neither sets/uses originID, so that must be handled manually
    explicit Pose3d(const PoseStruct3d& poseStruct, const PoseOriginList& originList);
    PoseStruct3d ToPoseStruct3d(const PoseOriginList& originList) const;
    
    // Accessors:
    //   Note: we return by value for Rvalues to avoid keeping references to temporary poses' members
    const Rotation3d&       GetRotation()       const& { return GetTransform().GetRotation();    }
    Rotation3d              GetRotation()       &&     { return GetTransform().GetRotation();    }
    const Vec3f&            GetTranslation()    const& { return GetTransform().GetTranslation(); }
    Vec3f                   GetTranslation()    &&     { return GetTransform().GetTranslation(); }
    
    RotationMatrix3d        GetRotationMatrix() const { return GetRotation().GetRotationMatrix(); }
    RotationVector3d        GetRotationVector() const { return GetRotation().GetRotationVector(); }
    Vec3f                   GetRotationAxis()   const { return GetRotation().GetAxis(); }

    // Get the rotation angle, optionally around a specific axis.
    // By default the rotation angle *around the pose's rotation axis* is
    // returned. When 'X', 'Y', or 'Z' are specified (case insensitive), the
    // rotation angle around that axis is returned.
    template<char AXIS = ' '>
    Radians GetRotationAngle() const { return GetTransform().GetRotationAngle<AXIS>(); }
    
    // Convenience wrappers around underlying transform:
    void SetRotation(const Rotation3d       &R)    { GetTransform().SetRotation(R); }
    void SetRotation(const RotationMatrix3d &Rmat) { GetTransform().SetRotation( Rotation3d(Rmat) ); }
    void SetRotation(const RotationVector3d &Rvec) { GetTransform().SetRotation( Rotation3d(Rvec) ); }
    void SetRotation(const Radians angle, const Vec3f &axis) { GetTransform().SetRotation( Rotation3d(angle, axis) ); }
    void SetTranslation(const Vec3f &T) { GetTransform().SetTranslation(T); }
    
    void RotateBy(const Radians& angle)         { GetTransform().RotateBy(angle); }
    void RotateBy(const RotationVector3d& Rvec) { GetTransform().RotateBy(Rvec);  }
    void RotateBy(const RotationMatrix3d& Rmat) { GetTransform().RotateBy(Rmat);  }
    void RotateBy(const Rotation3d& R)          { GetTransform().RotateBy(R);     }
    
    // translate along its current angle, along the +x axis (negative means backwards)
    template<typename T>
    void TranslateForward(T dist) { GetTransform().TranslateForward(dist); }

    // "Apply" Pose to 3D point(s) (i.e. transform that point by this Pose)
    template<typename T>
    Point<3,T> operator*(const Point<3,T> &point) const { return GetTransform() * point; }
    
    template<typename T>
    void ApplyTo(const std::vector<Point<3,T> > &pointsIn,
                 std::vector<Point<3,T> >       &pointsOut) const { GetTransform().ApplyTo(pointsIn, pointsOut); }

    template<typename T, size_t N>
    void ApplyTo(const std::array<Point<3,T>, N> &pointsIn,
                 std::array<Point<3,T>, N>       &pointsOut) const { GetTransform().ApplyTo(pointsIn, pointsOut); }
    
    template<typename T>
    void ApplyTo(const Quadrilateral<3,T> &quadIn,
                 Quadrilateral<3,T>       &quadOut) const { GetTransform().ApplyTo(quadIn, quadOut); }
    
    // Get this pose with respect to another pose.  Other pose
    // must share the same origin as this pose.  If it does not, false will be
    // returned to indicate failure.  Otherwise, true is returned and the new
    // pose is stored in newPoseWrtOtherPose.
    bool GetWithRespectTo(const Pose3d& otherPose,
                          Pose3d& newPoseWrtOtherPose) const;
    
    // Get pose with respect to its origin
    // NOTE: This returns by value, not reference. Do not store references to its members in line! I.e., never do:
    //   const Foo& = somePose.GetWithRespectToRoot().GetFoo(); // Foo will immediately be invalidated
    // Either store "somePoseWrtRoot" and then get Foo& from it, or make Foo a copy.
    Pose3d GetWithRespectToRoot() const;
    
    // Check to see if two poses are the same.  Return true if so.
    // * distThreshold is aligned to "this" pose's orientation, not the parent frame!
    // * If distThreshold is same in all dimensions (e.g. if set from scalar float),
    //   simple Euclidean distance is used.
    // * If requested, Tdiff and angleDiff will contain how different the two
    //   transformations are.
    bool IsSameAs(const Pose3d&  P_other,
                  const Point3f& distThreshold,
                  const Radians& angleThreshold) const;
    
    bool IsSameAs(const Pose3d&  P_other,
                  const Point3f& distThreshold,
                  const Radians& angleThreshold,
                  Vec3f&         Tdiff) const;
    
    bool IsSameAs(const Pose3d&  P_other,
                  const Point3f& distThreshold,
                  const Radians& angleThreshold,
                  Vec3f&         Tdiff,
                  Radians&       angleDiff) const;
    
    // Same as above, but with a list of rotational ambiguities to ignore.
    // True will be returned so long as the difference from P_other is
    // one of the given rotations.
    bool IsSameAs_WithAmbiguity(const Pose3d& P_other,
                                const RotationAmbiguities& R_ambiguities,
                                const Point3f&   distThreshold,
                                const Radians&   angleThreshold) const;
    
    bool IsSameAs_WithAmbiguity(const Pose3d& P_other,
                                const RotationAmbiguities& R_ambiguities,
                                const Point3f&   distThreshold,
                                const Radians&   angleThreshold,
                                Vec3f&           Tdiff,
                                Radians&         angleDiff) const;
    
    // TODO: Provide IsSameAs_WithAmbiguity() wrappers that don't require Tdiff/angleDiff outputs

    void Print(const std::string& channel = "Pose", const std::string& eventName = "Pose3d.Print") const;
    
  protected:
    
    // Construct from the base class
    Pose3d(const PoseBase<Pose3d,Transform3d>& poseBase)
    : PoseBase<Pose3d,Transform3d>(poseBase)
    {
      
    }
    
  }; // class Pose3d
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Additional operations
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Multiple two poses together
  Pose2d operator* (const Pose2d& pose1, const Pose2d& pose2);
  Pose3d operator* (const Pose3d& pose1, const Pose3d& pose2);
  
  // Calculate vector from pose2's translation to pose1's translation (rotations of the input poses are ignored), with
  // the resulting vector expressed in the outputFrame (which can be any pose in the same tree as pose1 and pose2).
  // returns true/false depending on whether poses (and output frame) are comparable (share root)
  // stores result in outVector if the return value is true, untouched if false
  bool ComputeVectorBetween(const Pose3d& pose1, const Pose3d& pose2, const Pose3d& outputFrame, Vec3f& outVector);
  
  // calculate distance between pose's translations (rotation is ignored)
  // returns true/false depending on whether poses are comparable (share root)
  // stores result in outDistance if the return value is true, untouched if false
  bool ComputeDistanceBetween(const Pose3d& pose1, const Pose3d& pose2, f32& outDistance);
  
  // calculate distance squared between pose's translations (rotation is ignored)
  // returns true/false depending on whether poses are comparable (share root)
  // stores result in outDistanceSQ if the return value is true, untouched if false
  bool ComputeDistanceSQBetween(const Pose3d& pose1, const Pose3d& pose2, f32& outDistanceSQ);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Inlined methods:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Pose2d:
  inline bool Pose2d::GetWithRespectTo(const Pose2d& otherPose,
                                       Pose2d& newPoseWrtOtherPose) const {
    return PoseBase<Pose2d,Transform2d>::GetWithRespectTo(*this, otherPose, newPoseWrtOtherPose);
  }
  
  inline Pose2d Pose2d::GetWithRespectToRoot() const {
    Pose2d poseWrtOrigin;
    if(this->IsRoot()) {
      poseWrtOrigin = *this;
    } else if(PoseBase<Pose2d,Transform2d>::GetWithRespectTo(*this, this->FindRoot(), poseWrtOrigin) == false) {
      PRINT_NAMED_ERROR("Pose2d.GetWithRespectToRoot.Failed",
                        "Could not get pose w.r.t. its own root. This should never happen.");
      assert(false);
    }
    return poseWrtOrigin;
  }
  
  // Pose3d:
  inline bool Pose3d::GetWithRespectTo(const Pose3d& otherPose,
                                       Pose3d& newPoseWrtOtherPose) const {
    return PoseBase<Pose3d,Transform3d>::GetWithRespectTo(*this, otherPose, newPoseWrtOtherPose);
  }
  
  inline Pose3d Pose3d::GetWithRespectToRoot() const {
    Pose3d poseWrtOrigin;
    if(this->IsRoot()) {
      poseWrtOrigin = *this;
    } else if(PoseBase<Pose3d,Transform3d>::GetWithRespectTo(*this, this->FindRoot(), poseWrtOrigin) == false) {
      PRINT_NAMED_ERROR("Pose3d.GetWithRespectToRoot.Failed",
                        "Could not get pose w.r.t. its own root. This should never happen.");
      assert(false); // TODO: Do something more elegant
    }
    
    return poseWrtOrigin;
  }

  inline bool Pose3d::IsSameAs(const Pose3d&  P_other,
                               const Point3f& distThreshold,
                               const Radians& angleThreshold) const
  {
    Vec3f Tdiff;
    return IsSameAs(P_other, distThreshold, angleThreshold, Tdiff);
  }
  
  inline bool Pose3d::IsSameAs(const Pose3d&  P_other,
                               const Point3f& distThreshold,
                               const Radians& angleThreshold,
                               Vec3f& Tdiff) const
  {
    Radians angleDiff;
    return IsSameAs(P_other, distThreshold, angleThreshold, Tdiff, angleDiff);
  }
  
  inline bool Pose3d::IsSameAs_WithAmbiguity(const Pose3d& P_other,
                                             const RotationAmbiguities& R_ambiguities,
                                             const Point3f&   distThreshold,
                                             const Radians&   angleThreshold) const
  {
    Vec3f Tdiff;
    Radians angleDiff;
    return IsSameAs_WithAmbiguity(P_other, R_ambiguities, distThreshold,
                                  angleThreshold, Tdiff, angleDiff);
  }
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSE_H_
