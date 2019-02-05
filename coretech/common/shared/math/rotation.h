/**
 * File: rotation.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 8/23/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Implements objects for storing rotation operations, in two and
 *              three dimensions.
 *
 *              RotationMatrix2d and RotationMatrix3d are subclasses of
 *              SmallSquareMatrix for storing 2x2 and 3x3 rotation matrices, 
 *              respectively.
 *
 *              RotationVector3d stores an angle+axis rotation vector.
 *
 *              Rodrigues functions are provided to convert between 
 *              RotationMatrix3d and RotationVector3d containers.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef __CoreTech_Math__rotation__
#define __CoreTech_Math__rotation__

#include "coretech/common/shared/math/radians.h"
#include "coretech/common/shared/math/matrix.h"
#include "coretech/common/shared/math/point.h"

namespace Anki {
  
  template<MatDimType DIM>
  class RotationMatrixBase : public SmallSquareMatrix<DIM, float>
  {
  public:
    
    // Matrix multiplication operations just call base class functions but then
    // also make sure things stay normalized
    RotationMatrixBase<DIM>  operator* (const RotationMatrixBase<DIM>& R_other) const;
    RotationMatrixBase<DIM>& operator*=(const RotationMatrixBase<DIM>& R_other);
    RotationMatrixBase<DIM>& PreMultiplyBy(const RotationMatrixBase<DIM>& R_other);
    
    // Note that inverting a rotation is equivalent to transposing it
    RotationMatrixBase<DIM>& Invert(void);
    void                     GetInverse(RotationMatrixBase<DIM>& outInverted) const;

    
    bool IsValid(const float tolerance = 1e-6f) const;
    
  protected:
    
    constexpr static const float OrthogonalityToleranceLow  = 1e-6f;
    constexpr static const float OrthogonalityToleranceHigh = 1e-2f;
    
    // NOTE: RotationMatrixBase is not directly instantiable
    RotationMatrixBase(); // init to identity matrix
    RotationMatrixBase(const SmallSquareMatrix<DIM, float> &matrix);
    RotationMatrixBase(std::initializer_list<float> initValues);
    
    // Keep this an orthogonal matrix.  Throw exception if things get too
    // far from orthogonal, i.e. if any of the rows' norms are more than
    // OrthogonalityTolerance from 1.f.
    void Renormalize();
    
  }; // class RotationMatrixBase
  
  
  class RotationMatrix2d : public RotationMatrixBase<2>
  {
  public:
    RotationMatrix2d();
    RotationMatrix2d(const Radians angle);
    RotationMatrix2d(const Matrix_2x2f &matrix2x2);
    RotationMatrix2d(std::initializer_list<float> initVals);
    
  }; // class RotationMatrix2d
  
  // Forward declaratin:
  class RotationMatrix3d;
  
  
  class RotationVector3d
  {
  public:
    RotationVector3d(); // no rotation around z axis
    RotationVector3d(const Radians &angle, const Vec3f &axis);
    RotationVector3d(const Vec3f &rvec);
    RotationVector3d(const RotationMatrix3d &rmat);
    
    bool operator==(const RotationVector3d &other) const;
    
    // Accessors for angle and axis.  Note that it is more efficient
    // to request both simultaneously if you need both, because the
    // angle must be computed to get the axis anyway.
    Radians       GetAngle() const;
    const Vec3f&  GetAxis()  const;
    void          GetAngleAndAxis(Radians &angle, Vec3f &axis) const;
    
  private:
    Radians _angle;
    Vec3f   _axis; // unit vector
    
  }; // class RotationVector3d
  
  // A class for working with UnitQuaternions.
  // Type T must be float or double.
  template<typename T>
  class UnitQuaternion_ : public Point<4,T>
  {
  public:
    UnitQuaternion_();
    UnitQuaternion_(const UnitQuaternion_& other);
    UnitQuaternion_(const T w, const T x, const T y, const T z); // will normalize the inputs
    UnitQuaternion_(const Point<4,T>& vals);
    
    // Named accessors for the four elements of the quatnerion (w,x,y,z)
    inline T w() const { return this->operator[](0); }
    inline T x() const { return this->operator[](1); }
    inline T y() const { return this->operator[](2); }
    inline T z() const { return this->operator[](3); }
    
    // Note: if you use these accessors to set an individual element of the
    // quaternion, it is YOUR responsibility to renormalize!
    inline T& w() { return this->operator[](0); }
    inline T& x() { return this->operator[](1); }
    inline T& y() { return this->operator[](2); }
    inline T& z() { return this->operator[](3); }
    
    // Cast operator
    // NOTE: This instead of overloading assignment operator to avoid doing
    // a bunch of implicit casts unknowingly. Maybe do the same for Point class
    // and classes derived therefrom?
    template<typename T_other>
    UnitQuaternion_<T>& SetCast(const UnitQuaternion_<T_other> &other);
    
    bool operator==(const UnitQuaternion_<T>& other) const;
    
    // Quaternion multiplication
    UnitQuaternion_<T>  operator* (const UnitQuaternion_<T>& other) const;
    UnitQuaternion_<T>& operator*=(const UnitQuaternion_<T>& other);
    
    // Rotation of a point/vector
    Point3<T> operator*(const Point3<T>& p) const;
    
    // Normalize to unit length
    UnitQuaternion_<T>& Normalize();
    
    // Conjugate
    UnitQuaternion_<T>& Conj(); // in place
    UnitQuaternion_<T>  GetConj() const;
    
  }; // class UnitQuaternion
  
  // The default UnitQuaternion is double to minimize effects of floating-point error
  using UnitQuaternion = UnitQuaternion_<double>;
  
  // A general storage class for storing and converting between different Rotation
  // formats. Internally, uses a UnitQuaternion, but can be constructed from and
  // converted to RotationMatrices and RotationVectors.
  class Rotation3d
  {
  public:

    Rotation3d(const Radians& angle, const Vec3f& axis);
    Rotation3d(const RotationVector3d& Rvec);
    Rotation3d(const RotationMatrix3d& Rmat);
    Rotation3d(const UnitQuaternion& q);
    
    bool operator==(const Rotation3d& other) const;
    
    // Compose rotations
    Rotation3d& operator*=(const Rotation3d& other);
    Rotation3d  operator* (const Rotation3d& other) const;
    Rotation3d& PreMultiplyBy(const Rotation3d& other);
    
    // Rotate a point/vector
    Point3<float> operator*(const Point3<float>& p) const;    
    
    const UnitQuaternion& GetQuaternion()     const { return _q; }
    const RotationMatrix3d       GetRotationMatrix() const;
    const RotationVector3d       GetRotationVector() const;
    
    const Radians GetAngle() const;
    const Vec3f   GetAxis()  const;
    
    Radians GetAngleAroundAxis(AxisName axis) const;
    Radians GetAngleAroundXaxis() const;
    Radians GetAngleAroundYaxis() const;
    Radians GetAngleAroundZaxis() const;
    
    Radians GetAngleDiffFrom(const Rotation3d& otherRotation) const;
    
    Rotation3d& Invert();
    Rotation3d  GetInverse() const;
    
  private:
    UnitQuaternion _q;
    
    // Get a specific entry in the corresponding rotation matrix
    // i and j must be one of [0,1,2]; otherwise compilation will fail.
    template<s32 i, s32 j>
    f32 GetRmatEntry() const;
    
  }; // Rotation3d
  
  bool IsNearlyEqual(const Rotation3d& R1, const Rotation3d& R2, const f32 tolerance);
  
  
  class RotationMatrix3d : public RotationMatrixBase<3>
  {
  public:
    RotationMatrix3d(); // 3x3 identity matrix (no rotation)
    RotationMatrix3d(const RotationVector3d &rotationVector);
    RotationMatrix3d(const Matrix_3x3f &matrix3x3);
    RotationMatrix3d(std::initializer_list<float> initVals);
    RotationMatrix3d(const Radians angle, const Vec3f &axis);
    
    // Construct from Euler angles
    RotationMatrix3d(const Radians& angleX, const Radians& angleY, const Radians& angleZ);
    
    // Return total angular rotation from the identity (no rotation)
    Radians GetAngle() const;
    
    // Return angular rotation difference from another rotation matrix
    Radians GetAngleDiffFrom(const RotationMatrix3d &other) const;
    
    // Return the Euler angles, under the convention that the
    // rotation matrix is the composition of three ordered rotations around
    // each of the axes, namely:  R = Rz * Ry * Rx. Returns true if the
    // matrix is in Gimbal Lock (in which case angle_y will arbitrarily be set
    // to zero).
    // Note that any many situations, there is more than one solution. We often
    //   don't care, but if you need both (e.g. for unit tests), pass non-NULL
    //   pointers for the second set of angles. If any is requested, all should be.
    bool GetEulerAngles(Radians& angle_x, Radians& angle_y, Radians& angle_z,
                        Radians* angle_x2 = nullptr,
                        Radians* angle_y2 = nullptr,
                        Radians* angle_z2 = nullptr) const;
    
    Radians GetAngleAroundXaxis() const;
    Radians GetAngleAroundYaxis() const;
    Radians GetAngleAroundZaxis() const;
    
    // Templated wrapper for calling one of the above GetAngleAround methods.
    // AXIS can by 'X', 'Y', or 'Z'
    template<char AXIS>
    Radians GetAngleAroundAxis() const;
    
    // Get the angular difference between the given parent axis and its rotated version.
    // AXIS can by 'X', 'Y', or 'Z'
    // If outWhichAxis is not null, it will be set to GetRotatedParentAxis<AXIS>() (see below)
    template<char AXIS>
    Radians GetAngularDeviationFromParentAxis(AxisName* outWhichAxis = nullptr) const;

    // Get the angle of rotation _around_ the given axis in the _parent_ frame.
    // AXIS can be 'X', 'Y', or 'Z'
    template<char AXIS>
    Radians GetAngleAroundParentAxis() const;

    // Returns the axis that is most aligned with the specified axis of the parent pose.
    template<char AXIS>
    AxisName GetRotatedParentAxis(f32* maxVal = nullptr) const;
    
  }; // class RotationMatrix3d
  
  // Rodrigues' formula for converting between angle+axis representation and 3x3
  // matrix representation.
  void Rodrigues(const RotationVector3d &Rvec_in,
                       RotationMatrix3d &Rmat_out);
  
  void Rodrigues(const RotationMatrix3d &Rmat_in,
                       RotationVector3d &Rvec_out);

  
  // Stores a vector of rotations to consider as ambiguous and thus the same.
  // useAbsoluteValue=true is a shortcut to reduce the number of rotations stored by ignoring sign when comparing.
  class RotationAmbiguities
  {
  public:
    
    // Default constructor: no ambiguities
    RotationAmbiguities();
    
    RotationAmbiguities(bool useAbsoluteValue, std::vector<RotationMatrix3d>&& rotations);
    
    // Returns true if the given rotation is within threshold of any ambiguous rotation stored here
    bool IsRotationSame(const Rotation3d& R, const Radians& angleThreshold) const;
    
    bool HasAmbiguities() const { return !_rotations.empty(); }
    
  private:
    
    std::vector<RotationMatrix3d> _rotations;
    bool _useAbsoluteValue;
    
  }; // class RotationAmbiguities
  
#pragma mark --- Inlined / Templated Implementations ---
  
  inline Radians RotationVector3d::GetAngle(void) const {
    return _angle;
  }
  
  inline const Vec3f& RotationVector3d::GetAxis(void) const {
    if (_angle == 0) {
      // A zero degree rotation means the axis of rotation will be a null vector
      static const Vec3f AXIS_FOR_ZERO_ANGLE(Z_AXIS_3D());
      return AXIS_FOR_ZERO_ANGLE;
    }
    return _axis;
  }
  
  inline void RotationVector3d::GetAngleAndAxis(Radians &angle,
                                                Vec3f   &axis) const
  {
    axis = _axis;
    angle = _angle;
  }
  
  template<char parentAxis>
  AxisName RotationMatrix3d::GetRotatedParentAxis(f32 *maxVal) const
  {
    // Figure out which axis in the rotated frame corresponds to the given
    // axis in the parent frame
    const Point3f row = GetRow(AxisToIndex<parentAxis>());
    
    // assume X axis to start
    f32 max_val = row.x();
    AxisName rotatedAxis = max_val > 0 ? AxisName::X_POS : AxisName::X_NEG;
    
    if(std::abs(row.y()) > std::abs(max_val)) {
      max_val = row.y();
      rotatedAxis = max_val > 0 ? AxisName::Y_POS : AxisName::Y_NEG;
    }
    if(std::abs(row.z()) > std::abs(max_val)) {
      max_val = row.z();
      rotatedAxis = max_val > 0 ? AxisName::Z_POS : AxisName::Z_NEG;
    }

    if (maxVal != nullptr) {
      *maxVal = max_val;
    }
    
    return rotatedAxis;
  }

  template<char parentAxis>
  inline Radians RotationMatrix3d::GetAngularDeviationFromParentAxis(AxisName* outWhichAxis) const
  {
    f32 maxVal;
    AxisName axisName = GetRotatedParentAxis<parentAxis>(&maxVal);
    if(nullptr != outWhichAxis)
    {
      *outWhichAxis = axisName;
    }
    
    return std::acos(std::abs(maxVal));
  }
  
  template<char parentAxis>
  inline Radians RotationMatrix3d::GetAngleAroundParentAxis() const
  {
    AxisName axis = GetRotatedParentAxis<parentAxis>();
    
    switch(axis)
    {
      case AxisName::X_POS:
        return GetAngleAroundXaxis();
      case AxisName::X_NEG:
        return -GetAngleAroundXaxis();
      case AxisName::Y_POS:
        return GetAngleAroundYaxis();
      case AxisName::Y_NEG:
        return -GetAngleAroundYaxis();
      case AxisName::Z_POS:
        return GetAngleAroundZaxis();
      case AxisName::Z_NEG:
        return -GetAngleAroundZaxis();
    }
  }
  
} // namespace Anki

#endif /* defined(__CoreTech_Math__rotation__) */
