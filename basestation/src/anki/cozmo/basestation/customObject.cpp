/**
 * File: CustomObject.cpp
 *
 * Author: Alec Solder
 * Date:   06/20/16
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/customObject.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

namespace Anki {
  namespace Cozmo {
    
    const std::vector<Point3f>& CustomObject::GetCanonicalCorners() const
    {
      return _canonicalCorners;
    }

    CustomObject::CustomObject(ObjectType type, const f32 xSize_mm, const f32 ySize_mm, const f32 zSize_mm,
                               const f32 markerWidth_mm, const f32 markerHeight_mm)
    : ObservableObject(ObjectFamily::CustomObject, type)
    , _size(Point3f(xSize_mm, ySize_mm, zSize_mm))
    ,  _markerWidth_mm(markerWidth_mm), _markerHeight_mm(markerHeight_mm)
    , _vizHandle(VizManager::INVALID_HANDLE)
    {
      DefineFacesByType(type);
      
      _canonicalCorners = {{
        Point3f(-0.5f*_size.x(), -0.5f*_size.y(),  0.5f*_size.z()),
        Point3f( 0.5f*_size.x(), -0.5f*_size.y(),  0.5f*_size.z()),
        Point3f(-0.5f*_size.x(), -0.5f*_size.y(), -0.5f*_size.z()),
        Point3f( 0.5f*_size.x(), -0.5f*_size.y(), -0.5f*_size.z()),
        Point3f(-0.5f*_size.x(),  0.5f*_size.y(),  0.5f*_size.z()),
        Point3f( 0.5f*_size.x(),  0.5f*_size.y(),  0.5f*_size.z()),
        Point3f(-0.5f*_size.x(),  0.5f*_size.y(), -0.5f*_size.z()),
        Point3f( 0.5f*_size.x(),  0.5f*_size.y(), -0.5f*_size.z())
      }};

    } // CustomObject(type) Constructor
    
    void CustomObject::DefineFacesByType(ObjectType type)
    {
      // TODO replace this with actual object definitions, currently we don't have enough markers
      switch (type) {
        case ObjectType::Custom_STAR5_Box:
          AddFace(FRONT_FACE,  Vision::MARKER_STAR5);
          AddFace(BACK_FACE,   Vision::MARKER_ARROW);
          break;
        case ObjectType::Custom_STAR5_Cube:
          AddFace(FRONT_FACE,  Vision::MARKER_STAR5);
          AddFace(LEFT_FACE,   Vision::MARKER_STAR5);
          AddFace(BACK_FACE,   Vision::MARKER_STAR5);
          AddFace(RIGHT_FACE,  Vision::MARKER_STAR5);
          AddFace(TOP_FACE,    Vision::MARKER_STAR5);
          AddFace(BOTTOM_FACE, Vision::MARKER_STAR5);
          break;
        case ObjectType::Custom_ARROW_Box:
          AddFace(FRONT_FACE,  Vision::MARKER_ARROW);
          AddFace(BACK_FACE,   Vision::MARKER_STAR5);
          break;
        case ObjectType::Custom_ARROW_Cube:
          AddFace(FRONT_FACE,  Vision::MARKER_ARROW);
          AddFace(LEFT_FACE,   Vision::MARKER_ARROW);
          AddFace(BACK_FACE,   Vision::MARKER_ARROW);
          AddFace(RIGHT_FACE,  Vision::MARKER_ARROW);
          AddFace(TOP_FACE,    Vision::MARKER_ARROW);
          AddFace(BOTTOM_FACE, Vision::MARKER_ARROW);
          break;
        default:
          break;
      }
    }

    void CustomObject::AddFace(const FaceName whichFace, const Vision::MarkerType code)
    {
      Pose3d facePose;
      switch(whichFace)
      {
        case FRONT_FACE:
          facePose = Pose3d(-M_PI_2_F, Z_AXIS_3D(), {-_size.x() * 0.5f, 0.f, 0.f},  &GetPose());
          break;
          
        case LEFT_FACE:
          facePose = Pose3d(M_PI,    Z_AXIS_3D(), {0.f, _size.y() * 0.5f, 0.f},   &GetPose());
          break;
          
        case BACK_FACE:
          facePose = Pose3d(M_PI_2,  Z_AXIS_3D(), {_size.x() * 0.5f, 0.f, 0.f},   &GetPose());
          break;
          
        case RIGHT_FACE:
          facePose = Pose3d(0.0f,       Z_AXIS_3D(), {0.f, -_size.y() * 0.5f, 0.f},  &GetPose());
          break;
          
        case TOP_FACE:
          // Rotate -90deg around X, then -90 around Z
          facePose = Pose3d(2.09439510f, {-0.57735027f, 0.57735027f, -0.57735027f}, {0.f, 0.f, _size.z() * 0.5f},  &GetPose());
          break;
          
        case BOTTOM_FACE:
          // Rotate +90deg around X, then -90 around Z
          facePose = Pose3d(2.09439510f, {0.57735027f, -0.57735027f, -0.57735027f}, {0.f, 0.f, -_size.z() * 0.5f}, &GetPose());
          break;
          
        default:
          PRINT_NAMED_WARNING("CustomObject.AddFace", "Attempting to add unknown block face.");
      }
      _markersByFace[whichFace] = &AddMarker(code, facePose, Point2f(_markerWidth_mm, _markerHeight_mm));
    }
    
    CustomObject::~CustomObject() {
      EraseVisualization();
    }
    
    Point3f CustomObject::GetSameDistanceTolerance() const
    {
      return _size * kSameDistToleranceFraction;
    }
    
    void CustomObject::Visualize(const ColorRGBA& color) const
    {
      DEV_ASSERT(nullptr != _vizManager, "VizManager was not set for object we want to visualize");
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      _vizHandle = _vizManager->DrawCuboid(GetID().GetValue(), _size, vizPose, color);
    }
    
    void CustomObject::EraseVisualization() const
    {
      // Erase the main object
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        _vizManager->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
    }
    
    
    void CustomObject::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const
    {
      corners = GetCanonicalCorners();
      for(auto & corner : corners) {
        corner *= _size;
        corner = atPose * corner;
      }
    }
    
  } // namespace Cozmo
  
} // namespace Anki
