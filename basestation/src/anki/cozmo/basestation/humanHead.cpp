/**
 * File: humanHead.cpp
 *
 * Author: Andrew Stein
 * Date:   6/30/2015
 *
 * Description: A subclass of the generic ObservableObject for holding a human
 *   head with a face on the front treated as a "Marker"
 *
 *   For head/face dimensions, reference is:
 *     https://upload.wikimedia.org/wikipedia/commons/6/61/HeadAnthropometry.JPG
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/humanHead.h"

#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
namespace Cozmo {

  
  
  const std::vector<Point3f>& HumanHead::GetCanonicalCorners() const
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
  
  
  static const Point3f& GetSizeByType(HumanHead::Type type)
  {
    // TODO: Add different head sizes by gender / age?
    
    // x is head width, y is head depth, z is head height, in mm
    // this is to match Marker coordinates, where the marker is in the X-Z plane,
    // so we want the face to also be in the X-Z plane
    static const std::map<HumanHead::Type, Point3f> Sizes = {
      {HumanHead::Type::HumanFace_Unknown, {148.f, 225.f, 195.f}},
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
  
  
  HumanHead::HumanHead(Type type)
  : ObservableObject(ObjectFamily::HumanHead,type)
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
    
    // Add the face as a "marker" at a known location on the head:
    const Pose3d facePose(0, Z_AXIS_3D(), {0,0.5f*_size.y(),0.f});
    AddMarker(Vision::Marker::FACE_CODE, facePose, _size.x()); // Using entire head width for face size
    
  } // MarkerlessObject(type) Constructor
  
  HumanHead::~HumanHead() {
    EraseVisualization();
  }
  
  
  void HumanHead::Visualize(const ColorRGBA& color)
  {
    // TODO: How to visualize without a (cozmo-specific) VizManager?
    Pose3d vizPose = GetPose().GetWithRespectToOrigin();
    _vizHandle = VizManager::getInstance()->DrawHumanHead(GetID().GetValue(), _size, vizPose, color);
  }
  
  void HumanHead::EraseVisualization()
  {
    // Erase the main object
    if(_vizHandle != VizManager::INVALID_HANDLE) {
      VizManager::getInstance()->EraseVizObject(_vizHandle);
      _vizHandle = VizManager::INVALID_HANDLE;
    }
  }
  
  
} // namespace Cozmo
} // namespace Anki