/**
 * File: mat.cpp
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Implements a MatPiece object, which is a "mat" that Cozmo drives
 *              around on with VisionMarkers at known locations for localization.
 *
 *              MatPiece inherits from ActionableObject since mats may have
 *              action poses for "entering" the mat, for example.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/



#include "engine/mat.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/quad_impl.h"

namespace Anki {
  namespace Vector {
    
   
    const std::vector<Point3f>& MatPiece::GetCanonicalCorners() const
    {
      return _canonicalCorners;
    }
    
    MatPiece::MatPiece(ObjectType type, const Point3f& size)
    : ObservableObject(ObjectFamily::Mat, type)
    , _size(size)
    , _canonicalCorners({
      Point3f(-0.5f*_size.x(), -0.5f*_size.y(), -_size.z()),
      Point3f( 0.5f*_size.x(), -0.5f*_size.y(), -_size.z()),
      Point3f(-0.5f*_size.x(), -0.5f*_size.y(),  0.f),
      Point3f( 0.5f*_size.x(), -0.5f*_size.y(),  0.f),
      Point3f(-0.5f*_size.x(),  0.5f*_size.y(), -_size.z()),
      Point3f( 0.5f*_size.x(),  0.5f*_size.y(), -_size.z()),
      Point3f(-0.5f*_size.x(),  0.5f*_size.y(),  0.f),
      Point3f( 0.5f*_size.x(),  0.5f*_size.y(),  0.f)
    })
    {
      
    }

    MatPiece::~MatPiece()
    {
      EraseVisualization();
    }
    
    Point3f MatPiece::GetSameDistanceTolerance() const
    {
      // "Thin" mats: don't use half the thickness as the height tolerance (too strict)
      Point3f distTol(_size.x()*.5f, _size.y()*.5f, std::max(25.f, _size.z()*.5f));
      return distTol;
    }
    
    
    Radians MatPiece::GetSameAngleTolerance() const {
      return DEG_TO_RAD(45); // TODO: too loose?
    }

    
    void MatPiece::Visualize(const ColorRGBA& color) const
    {
      // VizManager's cuboids are drawn around their center, so adjust the
      // vizPose to account for the fact that MatPieces' origins are on the top surface.
      Pose3d vizPose = Pose3d(RotationMatrix3d(), {0.f, 0.f, -.5f*_size.z()}, GetPose());
      vizPose = vizPose.GetWithRespectToRoot();
      _vizHandle = _vizManager->DrawCuboid(GetID().GetValue(), _size, vizPose, color);
    }
    
    void MatPiece::EraseVisualization() const
    {
      // Erase the main object
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        _vizManager->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
    }
    
    
    bool MatPiece::IsPoseOn(const Anki::Pose3d &pose, const f32 heightOffset, const f32 heightTol) const
    {
      Pose3d poseWrtMat;
      return IsPoseOn(pose, heightOffset, heightTol, poseWrtMat);
    }
    
    
    bool MatPiece::IsPoseOn(const Anki::Pose3d &pose, const f32 heightOffset, const f32 heightTol, Pose3d& poseWrtMat) const
    {
      if(pose.GetWithRespectTo(GetPose(), poseWrtMat) == false) {
        return false;
      }
      
      const Point2f pt(poseWrtMat.GetTranslation().x(), poseWrtMat.GetTranslation().y());
      const bool withinBBox = GetBoundingQuadXY(Pose3d()).Contains(pt);
      
      const bool withinHeight = NEAR(poseWrtMat.GetTranslation().z(), heightOffset, heightTol);
      
      // Make sure the given pose's rotation axis is well aligned with the mat's Z axis
      // TODO: make alignment a parameter?
      // TODO: const bool zAligned     = poseWrtMat.GetRotationAxis().z() >= std::cos(DEG_TO_RAD(25));
      const bool zAligned = true;
      
      return withinBBox && withinHeight && zAligned;
      
    } // IsPoseOn()
        
    
    void MatPiece::GetUnsafeRegions(std::vector<std::pair<Quad2f,ObjectID> >& unsafeRegions, const Pose3d& atPose, const f32 padding_mm) const
    {
      // Put the canonical regions created above at the current pose, and add them
      // to the given vector
      std::vector<Quad3f> regions;
      GetCanonicalUnsafeRegions(padding_mm, regions);
      for(Quad3f const& region : regions) {
        Quad3f regionAtPose;
        atPose.ApplyTo(region, regionAtPose);
        
        // Note we are constructing a 2D quad here from the 3D one and just
        // dropping the z coordinate
        unsafeRegions.emplace_back(regionAtPose, this->GetID());
      }
      
    } // GetUnsafeRegions()
    
    
    
  } // namespace Vector
  
} // namespace Anki
