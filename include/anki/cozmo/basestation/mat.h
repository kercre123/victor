/**
 * File: mat.h
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Defines a MatPiece object, which is a "mat" that Cozmo drives 
 *              around on with VisionMarkers at known locations for localization.
 *
 *              MatPiece inherits from ActionableObject since mats may have
 *              action poses for "entering" the mat, for example.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef __Products_Cozmo__Mat__
#define __Products_Cozmo__Mat__

#include "actionableObject.h"

#include "vizManager.h"

namespace Anki {
  
  namespace Cozmo {
    
    class MatPiece : public ActionableObject
    {
    public:
      
      // TODO: Use a MatDefinitions file, like with blocks
      class Type : public ObjectType
      {
        Type(const std::string& name) : ObjectType(name) { }
      public:
        // Define new mat piece types here, as static const Type:
        // (Note: don't forget to instantiate each in the .cpp file)
        static const Type LETTERS_4x4;
        static const Type LARGE_PLATFORM;
        static const Type LONG_BRIDGE;
        static const Type SHORT_BRIDGE;
      };
      
      // Constructor, based on Type
      MatPiece(Type type);
      
      //
      // Inherited Virtual Methods
      //
      
      virtual ObjectType GetType() const override { return _type; }
    
      //virtual float GetMinDim() const {return 0;}
      
      virtual MatPiece* CloneType() const;
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const;
      
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
    
      virtual void Visualize(const ColorRGBA& color) override;
      virtual void EraseVisualization() override;
      
      virtual Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm = 0.f) const override;
      
      static ObjectType GetTypeByName(const std::string& name);
      
      virtual Point3f GetSameDistanceTolerance() const override;
      virtual Radians GetSameAngleTolerance() const override;
      
      //
      // MatPiece Methods
      //

      // Return true if given pose shares a common origin and is "on" this
      // MatPiece; i.e., is within its bounding box, within the specified height
      // tolerance from the top surface, and with Z axis aligned to the mat's
      // z axis.
      bool IsPoseOn(const Pose3d& pose, const f32 heightOffset, const f32 heightTol) const;
      
      // Same as above, but also returns the pose w.r.t. the mat.
      bool IsPoseOn(const Pose3d& pose, const f32 heightOffset, const f32 heightTol, Pose3d& poseWrtMat) const;
      
      // Returns top surface height w.r.t. the mat's current pose origin
      f32 GetDrivingSurfaceHeight() const;
      
      //void SetOrigin(const Pose3d* newOrigin);
      
      bool IsMoveable() const { return _isMoveable; }
      
      // Like GetBoundingQuadXY, but returns quads indicating unsafe regions to
      // drive on or around this mat, such as the regions around a platform
      // (so robot doesn't drive off) or some kind of 3D obstacle built into the
      // mat.
      // Note these quads are _added_ to whatever is in the given vector.
      void GetUnsafeRegions(std::vector<Quad2f>& unsafeRegions, const Pose3d& atPose, const f32 padding_mm) const;
      void GetUnsafeRegions(std::vector<Quad2f>& unsafeRegions, const f32 padding_mm) const; // at current pose
      
      
    protected:
      static const std::vector<RotationMatrix3d> _rotationAmbiguities;
      static const s32 NUM_CORNERS = 8;
      static const std::array<Point3f, MatPiece::NUM_CORNERS> _canonicalCorners;
      
      const Type _type;
      
      // x = length, y = width, z = height
      Point3f _size;
      
      bool _isMoveable;
      
      VizManager::Handle_t _vizHandle;
    };
    
    
    inline MatPiece* MatPiece::CloneType() const
    {
      // Call the copy constructor
      return new MatPiece(this->_type);
    }
    
    inline void MatPiece::GetUnsafeRegions(std::vector<Quad2f>& unsafeRegions, const f32 padding_mm) const {
      GetUnsafeRegions(unsafeRegions, GetPose(), padding_mm);
    }
    
    
  } // namespace Cozmo

} // namespace Anki

#endif // __Products_Cozmo__Mat__
