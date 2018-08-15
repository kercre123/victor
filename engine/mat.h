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
 *              Note that a mat's origin's height is at the driving surface
 *              of the mat so that a robot's height is always near zero when it
 *              is on a mat and does not depend on the thickness of the mat.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef __Products_Cozmo__Mat__
#define __Products_Cozmo__Mat__

#include "engine/actionableObject.h"

#include "engine/viz/vizManager.h"

namespace Anki {
  
  namespace Vector {
    
    class MatPiece : public ActionableObject
    {
    public:
      
      //
      // Inherited Virtual Methods
      //
      virtual ~MatPiece(); 
      
      virtual void Visualize(const ColorRGBA& color) const override;
      virtual void EraseVisualization() const override;
      
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
      
      // TODO: remove this now that the origin is defined to be the top surface
      // Returns top surface height w.r.t. the mat's current pose origin
      virtual f32 GetDrivingSurfaceHeight() const { return 0.f; }
      
      //void SetOrigin(const Pose3d* newOrigin);
      
      // Like GetBoundingQuadXY, but returns quads indicating unsafe regions to
      // drive on or around this mat, such as the regions around a platform
      // (so robot doesn't drive off) or some kind of 3D obstacle built into the
      // mat.
      // Note these quads are _added_ to whatever is in the given vector.
      virtual void GetUnsafeRegions(std::vector<std::pair<Quad2f,ObjectID> >& unsafeRegions, const Pose3d& atPose, const f32 padding_mm) const;
      void GetUnsafeRegions(std::vector<std::pair<Quad2f,ObjectID> >& unsafeRegions, const f32 padding_mm) const; // at current pose
      
      // Since robot can drive on top of a mat, their bounding boxes are allowed
      // to intersect
      virtual bool CanIntersectWithRobot() const override { return true; }
      
    protected:
      
      // Derived classes can instantiate the MatPiece by defining its size.
      // Generic mat pieces are not instantiable.
      MatPiece(ObjectType type, const Point3f& size);

      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      virtual void GeneratePreActionPoses(const PreActionPose::ActionType type,
                                          std::vector<PreActionPose>& preActionPoses) const override {};
      
      // Unsafe regions in the canonical position, given padding
      // By default, there are no "unsafe" regions to avoid. Derived classes
      // can override that.
      virtual void GetCanonicalUnsafeRegions(const f32 padding_mm,
                                             std::vector<Quad3f>& regions) const { regions.clear(); }

      virtual const Point3f& GetSize() const override { return _size; }
      
    private:
      Point3f _size;
      const std::vector<Point3f> _canonicalCorners;
      mutable VizManager::Handle_t _vizHandle;
      
    }; // class MatPiece

    
    inline void MatPiece::GetUnsafeRegions(std::vector<std::pair<Quad2f,ObjectID> >& unsafeRegions, const f32 padding_mm) const {
      GetUnsafeRegions(unsafeRegions, GetPose(), padding_mm);
    }
    
    
       
  } // namespace Vector

} // namespace Anki

#endif // __Products_Cozmo__Mat__
