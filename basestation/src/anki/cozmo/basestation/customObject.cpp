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

    CustomObject::CustomObject(Type type, const f32 depth_mm, const f32 width_mm, const f32 height_mm)
    : ObservableObject(ObjectFamily::CustomObject, ObjectType::CustomObstacle)
    , _size(Point3f(depth_mm, width_mm, height_mm))
    , _vizHandle(VizManager::INVALID_HANDLE)
    {
      _depth_mm = depth_mm;
      _width_mm = width_mm;
      _height_mm = height_mm;

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
    
    CustomObject::~CustomObject() {
      EraseVisualization();
    }
    
    Point3f CustomObject::GetSameDistanceTolerance() const
    {
      return _size*.5f;
    }
    
    void CustomObject::Visualize(const ColorRGBA& color) const
    {
      ASSERT_NAMED(nullptr != _vizManager, "VizManager was not set for object we want to visualize");
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
