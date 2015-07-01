/**
 * File: observedFaceObject.cpp
 *
 * Author: Andrew Stein
 * Date:   6/30/2015
 *
 * Description: A subclass of the generic ObservableObject for holding faces.
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/vision/basestation/observedFaceObject.h"

namespace Anki {
namespace Vision {

  const ObservedFaceObject::Type ObservedFaceObject::Type::UNKNOWN_FACE("UNKNOWN_FACE");
  
  
  const std::vector<Point3f>& ObservedFaceObject::GetCanonicalCorners() const
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
  
  
  // https://upload.wikimedia.org/wikipedia/commons/6/61/HeadAnthropometry.JPG
  static const Point3f& GetSizeByType(ObservedFaceObject::Type type)
  {
    // x is face width, y is face height, z is depth of head, in mm
    static const std::map<ObservedFaceObject::Type, Point3f> Sizes = {
      {ObservedFaceObject::Type::UNKNOWN_FACE, {134.f, 118.f, 195.f}},
    };
    
    auto iter = Sizes.find(type);
    if(iter == Sizes.end()) {
      PRINT_NAMED_ERROR("MarkerlessObject.GetSizeByType.UndefinedType",
                        "No size defined for type %s (%d).\n",
                        type.GetName().c_str(), type.GetValue());
      static const Point3f DefaultSize(0.f,0.f,0.f);
      return DefaultSize;
    } else {
      return iter->second;
    }
  }
  
  
  ObservedFaceObject::ObservedFaceObject(Type type)
  : _type(type)
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
  
  ObservedFaceObject::~ObservedFaceObject() {
    EraseVisualization();
  }
  
  
  void ObservedFaceObject::Visualize(const ColorRGBA& color)
  {
    // TODO: How to visualize without a (cozmo-specific) VizManager?
    Pose3d vizPose = GetPose().GetWithRespectToOrigin();
    //_vizHandle = VizManager::getInstance()->DrawCuboid(GetID().GetValue(), _size, vizPose, color);
  }
  
  void ObservedFaceObject::EraseVisualization()
  {
    // TODO: How to visualize without a (cozmo-specific) VizManager?
    /*
    // Erase the main object
    if(_vizHandle != VizManager::INVALID_HANDLE) {
      VizManager::getInstance()->EraseVizObject(_vizHandle);
      _vizHandle = VizManager::INVALID_HANDLE;
    }
     */
  }
  
  
} // namespace Vision
} // namespace Anki