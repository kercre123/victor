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

#include "anki/common/basestation/math/matrix.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/poseBase.h"
#include "anki/common/basestation/math/quad.h"
#include "anki/common/basestation/math/rotation.h"
#include "anki/common/shared/radians.h"

#include "anki/common/basestation/exceptions.h"

#include <list>

namespace Anki {

  // Forward declarations of types used below:
  //  typedef Point3f Vec3f;
  //template<typename T> class Matrix;
  

  // Forward declaration of Pose3d so Pose2d can use it
  class Pose3d;
  
  //
  // Pose2d Class
  //
  // Stores 2d translation and orienation in a plane.
  //
  class Pose2d : public PoseBase<Pose2d>
  {
  public:
   
    // Constructors:
    Pose2d(); 
    Pose2d(const Radians &angle, const Point2f &translation);
    Pose2d(const Radians &angle, const float x, const float y);
    
    // Create a 2D pose from a 3D one, by keeping only the translation in the
    // XY plane and the rotation around the Z axis.
    Pose2d(const Pose3d& pose3d);

    /* TODO: Add constructor to take in covariance
    Pose2d(const float x, const float y, const Radians angle,
           const Matrix<float> &cov);
     */
    
    //bool IsOrigin() const { return parent == nullptr; }
    //Pose2d* GetOrigin() const;
    
    // Accessors:
    float   GetX()     const;
    float   GetY()     const;
    Radians GetAngle() const;
    
    const Point2f& GetTranslation() const;
    const Point3f& GetPlaneOrigin() const;
    const Vec3f&   GetPlaneNormal() const;

    void  SetPlaneOrigin(const Point3f &origin);
    void  SetPlaneNormal(const Vec3f   &normal);
    
    // Note that this Rotation Matrix is not a member but is computed
    // on-the-fly from the Pose's angle.
    RotationMatrix2d GetRotationMatrix() const;
    
    //void SetParent(const Pose2d* otherPose);
    //const Pose2d* GetParent() const;
    
    // Composition with another Pose2d:
    void   operator*=(const Pose2d &other); // in place
    Pose2d operator* (const Pose2d &other) const;
    void PreComposeWith(const Pose2d &other);
    
    // "Apply" Pose to a 2D point (i.e. transform that point by this Pose)
    Point2f operator*(const Point2f &point) const;
    
    Pose2d  GetInverse(void) const; // return new Pose
    Pose2d& Invert(void); // in place
    
    // Get this pose with respect to another pose.  Other pose
    // must share the same origin as this pose.  If it does not, false will be
    // returned to indicate failure.  Otherwise, true is returned and the new
    // pose is stored in newPoseWrtOtherPose.
    bool GetWithRespectTo(const Pose2d& otherPose,
                          Pose2d& newPoseWrtOtherPose) const;
    
    // Get pose with respect to its origin
    Pose2d GetWithRespectToOrigin() const;

    const Pose2d& FindOrigin() const;
    
  protected:
    Point2f _translation;
    Radians _angle;
    
    // Stores the orientation of this 2D plane in 3D space
    Point3f _planeOrigin;
    Vec3f   _planeNormal;
    
    /* TODO: Add and use 3DOF covariance
    Matrix<float> covariance;
     */
    
  }; // class Pose2d
  
  
  // TODO: Add thin wrapper or typedef for "Transform2d/3d" objects?
  
  
  //
  // Pose3d Class
  //
  // Stores a 6DOF (Degree Of Freedom) pose: 3D translation plus
  //   3-axis rotation.
  //
  class Pose3d : public PoseBase<Pose3d>
  {
  public:
    
    // Default pose: no rotation, no translation, world as parent
    Pose3d();
    
    // Construct from generic rotation and translation vector
    Pose3d(const Rotation3d& R, const Vec3f& T,
           const Pose3d* parentPose = nullptr,
           const std::string& name = "");
    
    // Construct from rotation vector and translation vector
    Pose3d(const RotationVector3d &Rvec, const Vec3f &T,
           const Pose3d *parentPose = nullptr,
           const std::string& name = "");
    
    /* TODO: Add constructor that takes in covariance
    Pose3d(const RotationVector3d &Rvec, const Vec3f &T, const Matrix<float> &cov,
           const Pose3d *parentPose = Pose3d::World);
     */
    
    // Construct from rotation matrix and translation vector
    // TODO: do we want a version that takes in covariance too?
    Pose3d(const RotationMatrix3d &Rmat, const Vec3f &T,
           const Pose3d *parentPose = nullptr,
           const std::string& name = "");
    
    // Construct from an angle, axis, and translation vector
    Pose3d(const Radians &angle, const Vec3f &axis,
           const Vec3f &translation,
           const Pose3d *parentPose = nullptr,
           const std::string& name = "");
    
    // Construct a Pose3d from a Pose2d (using the plane information)
    Pose3d(const Pose2d &pose2d);
    
    // TODO: Copy constructor?
    Pose3d(const Pose3d &otherPose);
    
    //bool IsOrigin() const { return parent == nullptr; }

    // Accessors:
    const Rotation3d&       GetRotation()       const;
    const Vec3f&            GetTranslation()    const;
    
    RotationMatrix3d        GetRotationMatrix() const;
    RotationVector3d        GetRotationVector() const;
    Vec3f                   GetRotationAxis()   const;

    // Get the rotation angle, optionally around a specific axis.
    // By default the rotation angle *around the pose's rotation axis* is
    // returned. When 'X', 'Y', or 'Z' are specified (case insensitive), the
    // rotation angle around that axis is returned.
    template<char AXIS = ' '>
    Radians GetRotationAngle() const;
    
    void SetRotation(const Rotation3d       &R);
    void SetRotation(const RotationMatrix3d &Rmat);
    void SetRotation(const RotationVector3d &Rvec);
    void SetRotation(const Radians angle, const Vec3f &axis);
    void SetTranslation(const Vec3f &T);
    
    //void SetParent(const Pose3d* parent);
    //const Pose3d* GetParent() const { return Pose<3>::GetParent();
    
    // Composition with another Pose
    void   operator*=(const Pose3d &other); // in place
    Pose3d operator*(const Pose3d &other) const;
    bool operator==(const Pose3d &other) const;
    void PreComposeWith(const Pose3d &other);
    
    void RotateBy(const Radians& angle); // around existing axis
    void RotateBy(const RotationVector3d& Rvec);
    void RotateBy(const RotationMatrix3d& Rmat);
    
    // "Apply" Pose to 3D point(s) (i.e. transform that point by this Pose)
    template<typename T>
    Point<3,T> operator*(const Point<3,T> &point) const;
    
    template<typename T>
    void ApplyTo(const std::vector<Point<3,T> > &pointsIn,
                 std::vector<Point<3,T> >       &pointsOut) const;

    template<typename T, size_t N>
    void ApplyTo(const std::array<Point<3,T>, N> &pointsIn,
                 std::array<Point<3,T>, N>       &pointsOut) const;
    
    template<typename T>
    void ApplyTo(const Quadrilateral<3,T> &quadIn,
                 Quadrilateral<3,T>       &quadOut) const;
    
    Pose3d  GetInverse(void) const;
    Pose3d& Invert(void); // in place?
    
    // Get this pose with respect to another pose.  Other pose
    // must share the same origin as this pose.  If it does not, false will be
    // returned to indicate failure.  Otherwise, true is returned and the new
    // pose is stored in newPoseWrtOtherPose.
    bool GetWithRespectTo(const Pose3d& otherPose,
                          Pose3d& newPoseWrtOtherPose) const;
    
    // Get pose with respect to its origin
    Pose3d GetWithRespectToOrigin() const;
    
    const Pose3d& FindOrigin() const;

    // Check to see if two poses are the same.  Return true if so.
    // If requested, Tdiff and angleDiff will contain how different the two
    // transformations are.
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
                                const std::vector<RotationMatrix3d>& R_ambiguities,
                                const Point3f&   distThreshold,
                                const Radians&   angleThreshold,
                                const bool       useAbsRotation) const;
    
    bool IsSameAs_WithAmbiguity(const Pose3d& P_other,
                                const std::vector<RotationMatrix3d>& R_ambiguities,
                                const Point3f&   distThreshold,
                                const Radians&   angleThreshold,
                                const bool       useAbsRotation,
                                Vec3f&           Tdiff,
                                Radians&         angleDiff) const;
    
    // TODO: Provide IsSameAs_WithAmbiguity() wrappers that don't require Tdiff/angleDiff outputs

    void Print() const;
    
    std::string GetNamedPathToOrigin(bool showTranslations)   const;
    void        PrintNamedPathToOrigin(bool showTranslations) const;
    
  protected:
    
    //RotationVector3d  rotationVector;
    //RotationMatrix3d  _rotationMatrix;
    Rotation3d        _rotation;
    Vec3f             _translation;
    
    /* TODO: Add and use 6DOF covariance.
    // 6x6 covariance matrix (upper-left 3x3 block corresponds to rotation
    // vector elements, lower right 3x3 block corresponds to translation)
    Matrix<float> covariance;
     */
    
  }; // class Pose3d
  
  // Compute distance between the two poses' translation vectors
  Vec3f ComputeVectorBetween(const Pose3d& pose1, const Pose3d& pose2);
  inline f32 ComputeDistanceBetween(const Pose3d& pose1, const Pose3d& pose2) {
    return ComputeVectorBetween(pose1, pose2).Length();
  }
  
  
  //
  // Inline accessors:
  //
  
  // Pose2d
  
  inline float Pose2d::GetX(void) const
  { return _translation.x(); }
  
  inline float Pose2d::GetY(void) const
  { return _translation.y(); }
  
  inline Radians Pose2d::GetAngle(void) const
  { return _angle; }
  
  inline RotationMatrix2d Pose2d::GetRotationMatrix(void) const
  { return RotationMatrix2d(_angle); }
  
  inline const Point2f& Pose2d::GetTranslation(void) const
  { return _translation; }
  
  inline void Pose2d::SetPlaneNormal(const Vec3f &normal)
  {
    _planeNormal = normal;
    _planeNormal.MakeUnitLength();
  }
  
  inline const Vec3f& Pose2d::GetPlaneNormal() const
  { return _planeNormal; }
  
  inline const Point3f& Pose2d::GetPlaneOrigin() const
  { return _planeOrigin; }
  
  inline bool Pose2d::GetWithRespectTo(const Pose2d& otherPose,
                                       Pose2d& newPoseWrtOtherPose) const {
    return PoseBase<Pose2d>::GetWithRespectTo(*this, otherPose, newPoseWrtOtherPose);
  }
  
  inline Pose2d Pose2d::GetWithRespectToOrigin() const {
    Pose2d poseWrtOrigin;
    if(this->IsOrigin()) {
      poseWrtOrigin = *this;
    } else if(PoseBase<Pose2d>::GetWithRespectTo(*this, this->FindOrigin(), poseWrtOrigin) == false) {
      PRINT_NAMED_ERROR("Pose2d::GetWithRespectToOriginFailed",
                        "Could not get pose w.r.t. its own origin. This should never happen.\n");
      assert(false);
    }
    return poseWrtOrigin;
  }

  
  inline const Pose2d& Pose2d::FindOrigin() const {
    return PoseBase<Pose2d>::FindOrigin(*this);
  }
  
  // Pose3d
  inline const Rotation3d& Pose3d::GetRotation() const
  { return _rotation; }
  
  inline RotationMatrix3d Pose3d::GetRotationMatrix() const
  { return _rotation.GetRotationMatrix(); }
  
  inline RotationVector3d Pose3d::GetRotationVector() const
  { return _rotation.GetRotationVector(); }
  
  inline const Vec3f& Pose3d::GetTranslation() const
  { return _translation; }
  
  inline Vec3f Pose3d::GetRotationAxis() const
  { return GetRotationVector().GetAxis(); }
  
  /*
  template<char AXIS>
  inline Radians Pose3d::GetRotationAngle() const
  {
    CORETECH_THROW("Invalid template parameter for GetRotationAngle(). "
                   "Expecting 'X' / 'x', 'Y' / 'y', or 'Z' / 'z'.");
    return 0.f;
  }
   */
  
  template<>
  inline Radians Pose3d::GetRotationAngle<' '>() const
  {
    // return the inherient axis of the rotation
    return GetRotation().GetAngle();
  }

  template<>
  inline Radians Pose3d::GetRotationAngle<'X'>() const
  {
    return GetRotation().GetAngleAroundXaxis();
  }
  
  template<>
  inline Radians Pose3d::GetRotationAngle<'Y'>() const
  {
    return GetRotation().GetAngleAroundYaxis();
  }
  
  template<>
  inline Radians Pose3d::GetRotationAngle<'Z'>() const
  {
    return GetRotation().GetAngleAroundZaxis();
  }
  
  template<>
  inline Radians Pose3d::GetRotationAngle<'x'>() const
  {
    return GetRotation().GetAngleAroundXaxis();
  }
  
  template<>
  inline Radians Pose3d::GetRotationAngle<'y'>() const
  {
    return GetRotation().GetAngleAroundYaxis();
  }
  
  template<>
  inline Radians Pose3d::GetRotationAngle<'z'>() const
  {
    return GetRotation().GetAngleAroundZaxis();
  }
  
  inline void Pose3d::SetRotation(const Rotation3d& R)
  {
    _rotation = R;
  }
  
  inline void Pose3d::SetRotation(const RotationMatrix3d &Rmat)
  {
    _rotation = Rotation3d(Rmat);
  }
  
  inline void Pose3d::SetRotation(const RotationVector3d &Rvec)
  {
    _rotation = Rotation3d(Rvec);
  }
  
  inline void Pose3d::SetRotation(const Radians angle, const Vec3f &axis)
  {
    _rotation = Rotation3d(angle, axis);
  }
  
  inline void Pose3d::SetTranslation(const Vec3f &T)
  {
    _translation = T;
  }
  
  inline bool Pose3d::GetWithRespectTo(const Pose3d& otherPose,
                                       Pose3d& newPoseWrtOtherPose) const {
    return PoseBase<Pose3d>::GetWithRespectTo(*this, otherPose, newPoseWrtOtherPose);
  }
  
  inline Pose3d Pose3d::GetWithRespectToOrigin() const {
    Pose3d poseWrtOrigin;
    if(this->IsOrigin()) {
      poseWrtOrigin = *this;
    } else if(PoseBase<Pose3d>::GetWithRespectTo(*this, this->FindOrigin(), poseWrtOrigin) == false) {
      PRINT_NAMED_ERROR("Pose3d::GetWithRespectToOriginFailed",
                        "Could not get pose w.r.t. its own origin. This should never happen.\n");
      assert(false); // TODO: Do something more elegant
    }
    
    return poseWrtOrigin;
  }
  
  inline const Pose3d& Pose3d::FindOrigin() const {
    return PoseBase<Pose3d>::FindOrigin(*this);
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
                                             const std::vector<RotationMatrix3d>& R_ambiguities,
                                             const Point3f&   distThreshold,
                                             const Radians&   angleThreshold,
                                             const bool       useAbsRotation) const
  {
    Vec3f Tdiff;
    Radians angleDiff;
    return IsSameAs_WithAmbiguity(P_other, R_ambiguities, distThreshold,
                                  angleThreshold, useAbsRotation, Tdiff, angleDiff);
  }
  
  template<typename T>
  Point<3,T> Pose3d::operator*(const Point<3,T> &pointIn) const
  {
    Point3f pointOut( _rotation * pointIn );
    pointOut += _translation;
    
    return pointOut;
  }
  
  template<typename T>
  void Pose3d::ApplyTo(const Quadrilateral<3,T> &quadIn,
                       Quadrilateral<3,T>       &quadOut) const
  {
    using namespace Quad;
    quadOut[TopLeft]     = (*this) * quadIn[TopLeft];
    quadOut[TopRight]    = (*this) * quadIn[TopRight];
    quadOut[BottomLeft]  = (*this) * quadIn[BottomLeft];
    quadOut[BottomRight] = (*this) * quadIn[BottomRight];
  }
  
  template<typename T>
  void Pose3d::ApplyTo(const std::vector<Point<3,T> > &pointsIn,
                       std::vector<Point<3,T> >       &pointsOut) const
  {
    const size_t numPoints = pointsIn.size();
    
    if(pointsOut.size() == numPoints)
    {
      // The output vector already has the right number of points
      // in it.  No need to construct a new vector full of (0,0,0)
      // points with resize; just replace what's there.
      for(size_t i=0; i<numPoints; ++i)
      {
        pointsOut[i] = (*this) * pointsIn[i];
      }
      
    } else {
      // Clear the output vector, and use push_back to add newly-
      // constructed points. Again, this avoids first creating a
      // bunch of (0,0,0) points via resize and then immediately
      // overwriting them.
      pointsOut.clear();
      
      for(const Point3f& x : pointsIn)
      {
        pointsOut.emplace_back( (*this) * x );
      }
    }
  } // ApplyTo()
  
  template<typename T, size_t N>
  void Pose3d::ApplyTo(const std::array<Point<3,T>, N> &pointsIn,
                       std::array<Point<3,T>, N>       &pointsOut) const
  {
    for(size_t i=0; i<N; ++i)
    {
      pointsOut[i] = (*this) * pointsIn[i];
    }
  } // ApplyTo()
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSE_H_