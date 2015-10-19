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
    
    static const Point3f& GetSizeByType(MarkerlessObject::Type type)
    {
      static const std::map<MarkerlessObject::Type, Point3f> Sizes = {
        {MarkerlessObject::Type::ProxObstacle, {20.f, 40.f, 50.f}},
      };
    
      auto iter = Sizes.find(type);
      if(iter == Sizes.end()) {
        PRINT_NAMED_ERROR("MarkerlessObject.GetSizeByType.UndefinedType",
                          "No size defined for type %s (%d).\n",
                          ObjectTypeToString(type), type);
        static const Point3f DefaultSize(0.f,0.f,0.f);
        return DefaultSize;
      } else {
        return iter->second;
      }
    }
    
    
    MarkerlessObject::MarkerlessObject(Type type)
    : ObservableObject(ObjectFamily::MarkerlessObject, ObjectType::ProxObstacle)
    , _size(GetSizeByType(_type))
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
    
    void MarkerlessObject::Visualize(const ColorRGBA& color)
    {
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      _vizHandle = VizManager::getInstance()->DrawCuboid(GetID().GetValue(), _size, vizPose, color);
    }
    
    void MarkerlessObject::EraseVisualization()
    {
      // Erase the main object
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        VizManager::getInstance()->EraseVizObject(_vizHandle);
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
