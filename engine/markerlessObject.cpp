/**
 * File: MarkerlessObject.cpp
 *
 * Author: Kevin Yoon
 * Date:   (various)
 *
 *
 * Copyright: Anki, Inc. 2014
 **/



#include "engine/markerlessObject.h"

#include "coretech/vision/shared/MarkerCodeDefinitions.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/quad_impl.h"

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
        {ObjectType::ProxObstacle,      {10.f, 10.f, 50.f}},
        {ObjectType::CliffDetection,    {10.f, 150.f, 50.f}},
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
      DEV_ASSERT(nullptr != _vizManager, "VizManager was not set for object we want to visualize");
      Pose3d vizPose = GetPose().GetWithRespectToRoot();
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
    
    Quad2f MarkerlessObject::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {
      const std::vector<Point3f>& canonicalCorners = GetCanonicalCorners();
      
      const Pose3d& atPoseWrtOrigin = atPose.GetWithRespectToRoot();
      const Rotation3d& R = atPoseWrtOrigin.GetRotation();

      Point3f paddedSize(_size);
      paddedSize += 2.f*padding_mm;
      
      std::vector<Point2f> points;
      points.reserve(canonicalCorners.size());
      for(auto corner : canonicalCorners) {
        // Scale canonical point to correct (padded) size
        corner *= paddedSize;
        
        // Rotate to given pose
        corner = R*corner;
        
        // Project onto XY plane, i.e. just drop the Z coordinate
        points.emplace_back(corner.x(), corner.y());
      }
      
      Quad2f boundingQuad = GetBoundingQuad(points);
      
      // Re-center
      Point2f center(atPoseWrtOrigin.GetTranslation().x(), atPoseWrtOrigin.GetTranslation().y());
      boundingQuad += center;
      
      return boundingQuad;
      
    } // GetBoundingBoxXY()
      
  } // namespace Cozmo
  
} // namespace Anki
