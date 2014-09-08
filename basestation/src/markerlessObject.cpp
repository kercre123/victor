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

namespace Anki {
  namespace Cozmo {
    
    // Instantiate static const MarkerlessObject types here:
    // NOTE: Yes, there's only one type (PROX_OBSTACLE) but if we ever decide to have
    //       objects of different shapes, this makes it easier.
    const MarkerlessObject::Type MarkerlessObject::Type::PROX_OBSTACLE("PROX_OBSTACLE");
    
    
    // MarkerlessObject has no rotation ambiguities but we still need to define this
    // static const here to instatiate an empty list.
    const std::vector<RotationMatrix3d> MarkerlessObject::_rotationAmbiguities;
    
    const std::array<Point3f, MarkerlessObject::NUM_CORNERS> MarkerlessObject::_canonicalCorners = {{
      Point3f(-0.5f, -0.5f,  0.5f),
      Point3f( 0.5f, -0.5f,  0.5f),
      Point3f(-0.5f, -0.5f, -0.5f),
      Point3f( 0.5f, -0.5f, -0.5f),
      Point3f(-0.5f,  0.5f,  0.5f),
      Point3f( 0.5f,  0.5f,  0.5f),
      Point3f(-0.5f,  0.5f, -0.5f),
      Point3f( 0.5f,  0.5f, -0.5f)
    }};
    
    
    MarkerlessObject::MarkerlessObject(Type type)
    : _type(type)
    {
      if(Type::PROX_OBSTACLE == _type) {
        
        _size = {10.f, 24.f, 50.f};
        
      }
      else {
        PRINT_NAMED_ERROR("MarkerlessObject.UnrecognizedType",
                          "Trying to instantiate a MarkerlessObject with an unknown Type = %d.\n", int(type));
      }
      
    } // MarkerlessObject(type) Constructor
    
    MarkerlessObject::~MarkerlessObject() {
      EraseVisualization();
    }
    
    Point3f MarkerlessObject::GetSameDistanceTolerance() const
    {
       if(Type::PROX_OBSTACLE == GetType())
       {
         return _size *.5f;
       } else {
         PRINT_NAMED_ERROR("MarkerlessObject.GetSameDistanceTolerance.UnrecognizedType",
                           "Trying to get same-distance tolerance for a MarkerlessObject with an unknown Type = %d.\n", int(GetType()));
         return Point3f();
       }
    }
    
    
    Radians MarkerlessObject::GetSameAngleTolerance() const {
      return DEG_TO_RAD(45); // TODO: too loose?
    }

    
    std::vector<RotationMatrix3d> const& MarkerlessObject::GetRotationAmbiguities() const
    {
      return MarkerlessObject::_rotationAmbiguities;
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
    
    Quad2f MarkerlessObject::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {
      Point3f paddedSize(_size);
      paddedSize += 2.f*padding_mm;
     
      return ObservableObject::GetBoundingQuadXY_Helper(atPose, paddedSize, MarkerlessObject::_canonicalCorners);
    }
    
    
    ObjectType MarkerlessObject::GetTypeByName(const std::string& name)
    {
      if(name == "PROX_OBSTACLE") {
        return MarkerlessObject::Type::PROX_OBSTACLE;
      } else {
        PRINT_NAMED_ERROR("MarkerlessObject.NoTypeForName",
                          "No MarkerlessObject Type registered for name '%s'.\n", name.c_str());
        return ObjectType::GetInvalidType();
      }
    }
    
    void MarkerlessObject::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const {
      corners.resize(NUM_CORNERS);
      for(s32 i=0; i<NUM_CORNERS; ++i) {
        corners[i] = MarkerlessObject::_canonicalCorners[i];
        corners[i] *= _size;
        corners[i] = atPose * corners[i];
      }
    }
    
    void MarkerlessObject::GetSize(f32& x, f32& y, f32& z)
    {
      x = _size.x();
      y = _size.y();
      z = _size.z();
    }
    
    
  } // namespace Cozmo
  
} // namespace Anki
