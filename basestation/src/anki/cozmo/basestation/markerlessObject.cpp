/**
 * File: MarkerlessObject.cpp
 *
 * Author: Kevin Yoon
 * Date:   (various)
 *
 *
 * Copyright: Anki, Inc. 2014
 **/



#include "anki/cozmo/basestation/markerlessObject.h"

#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

namespace Anki {
  namespace Cozmo {
    
    const std::vector<Point3f>& MarkerlessObject::GetCanonicalCorners() const
    {
      static const std::vector<Point3f> CanonicalCorners = {{
        Point3f(-0.5f, -0.5f,  0.5f),
        Point3f( 0.5f, -0.5f,  0.5f),
        Point3f(-0.5f, -0.5f, -0.5f),
        Point3f( 0.5f, -0.5f, -0.5f),
        Point3f(-0.5f,  0.5f,  0.5f),
        Point3f( 0.5f,  0.5f,  0.5f),
        Point3f(-0.5f,  0.5f, -0.5f),
        Point3f( 0.5f,  0.5f, -0.5f)
      }};
      
      return CanonicalCorners;
    }
    
    const Point3f& MarkerlessObject::GetSizeByType(ObjectType type)
    {
      static const std::map<ObjectType, Point3f> Sizes = {
        {ObjectType::ProxObstacle,      {20.f, 40.f, 50.f}},
        {ObjectType::CliffDetection,    {20.f, 40.f, 50.f}}, // TODO: Update size?
        {ObjectType::CollisionObstacle, {20.f, ROBOT_BOUNDING_Y, ROBOT_BOUNDING_Z}},
      };
    
      auto iter = Sizes.find(type);
      if(iter == Sizes.end()) {
        PRINT_NAMED_ERROR("MarkerlessObject.GetSizeByType.UndefinedType",
                          "No size defined for type %s (%d).",
                          ObjectTypeToString(type), type);
        static const Point3f DefaultSize(10.f,10.f,10.f);
        return DefaultSize;
      } else {
        return iter->second;
      }
    }
    
    
    MarkerlessObject::MarkerlessObject(ObjectType type)
    : ObservableObject(ObjectFamily::MarkerlessObject, type)
    , _size(GetSizeByType(_type))
    , _vizHandle(VizManager::INVALID_HANDLE)
    {
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

    } // MarkerlessObject(type) Constructor
    
    MarkerlessObject::~MarkerlessObject() {
      EraseVisualization();
    }
    
    Point3f MarkerlessObject::GetSameDistanceTolerance() const
    {
      return _size*.5f;
    }
    
    void MarkerlessObject::Visualize(const ColorRGBA& color) const
    {
      ASSERT_NAMED(nullptr != _vizManager, "VizManager was not set for object we want to visualize");
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      _vizHandle = _vizManager->DrawCuboid(GetID().GetValue(), _size, vizPose, color);
    }
    
    void MarkerlessObject::EraseVisualization() const
    {
      // Erase the main object
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        _vizManager->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
    }
    
    
    void MarkerlessObject::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const
    {
      corners = GetCanonicalCorners();
      for(auto & corner : corners) {
        corner *= _size;
        corner = atPose * corner;
      }
    }
    
  } // namespace Cozmo
  
} // namespace Anki
