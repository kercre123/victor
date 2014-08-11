/**
 * File: bridge.cpp
 *
 * Author: Andrew Stein
 * Date:   8/5/2014
 *
 * Description: Implements a Bridge object, which is a type of DockableObject,
 *              "dockable" in the sense that we use markers to line the robot
 *              up with it, analogous to a ramp.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#if 0
#include "bridge.h"

namespace Anki {
  
  namespace Cozmo {
  
    const Bridge::Type Bridge::Type::LONG("LONG_BRIDGE");
    const Bridge::Type Bridge::Type::SHORT("SHORT_BRIDGE");
    
    const std::array<Point3f, Bridge::NUM_CORNERS> Bridge::_canonicalCorners = {{
      // Bottom corners
      Point3f(-0.5f, -0.5f, 0.f),
      Point3f(-0.5f,  0.5f, 0.f),
      Point3f( 0.5f, -0.5f, 0.f),
      Point3f( 0.5f,  0.5f, 0.f),
      
      // Top corners:
      Point3f(-0.5f, -0.5f, 1.f),
      Point3f(-0.5f,  0.5f, 1.f),
      Point3f( 0.5f, -0.5f, 1.f),
      Point3f( 0.5f,  0.5f, 1.f),
    }};
    
    
    Bridge::Bridge(Type type)
    : _type(type)
    , _leftEndMarker(nullptr)
    , _rightEndMarker(nullptr)
    {
      if(Type::LONG == _type) {
        _size = {300.f, 62.f, 3.f};
        
        const f32 markerSize = 25.f;
        
        _leftEndMarker  = &AddMarker(Vision::MARKER_BRIDGESUNLEFT,   {-_size.x()*.5f+markerSize, 0.f, _size.z()}, markerSize);
        _rightEndMarker = &AddMarker(Vision::MARKER_BRIDGESUNRIGHT,  { _size.x()*.5f-markerSize, 0.f, _size.z()}, markerSize);
        
        AddMarker(Vision::MARKER_BRIDGESUNMIDDLE, {                      0.f, 0.f, _size.z()}, markerSize);
      }
      else if(Type::SHORT == _type) {
        _size = {200.f, 62.f, 3.f};
        
        const f32 markerSize = 25.f;
        
        AddMarker(Vision::MARKER_BRIDGEMOONLEFT,   {-_size.x()*.5f+markerSize, 0.f, _size.z()}, markerSize);
        AddMarker(Vision::MARKER_BRIDGEMOONMIDDLE, {                      0.f, 0.f, _size.z()}, markerSize);
        AddMarker(Vision::MARKER_BRIDGEMOONRIGHT,  { _size.x()*.5f-markerSize, 0.f, _size.z()}, markerSize);
      }
      else {
        PRINT_NAMED_ERROR("Bridge.UnknownType", "Tried to construct unknown Bridge type.\n");
        return;
      }

      CORETECH_ASSERT(_leftEndMarker != nullptr);
      CORETECH_ASSERT(_rightEndMarker != nullptr);
      
    } // Bridge Constructor
    
#if 0
#pragma mark --- Virtual Method Implementations ---
#endif
    
    void Bridge::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const
    {
      // Use parent class helper with this class's canonical corners
      ObservableObject::GetCorners_Helper(atPose, Bridge::_canonicalCorners, corners);
    } // GetCorners()
    
    
    Bridge* Bridge::Clone() const
    {
      // Call the copy constructor
      return new Bridge(*this);
    }
    
    void Bridge::Visualize()
    {
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      
      _vizHandle[0] = VizManager::getInstance()->DrawCuboid(GetID().GetValue(), _size, GetPose(), VIZ_COLOR_DARKGRAY);
      
      Pose3d preCrossingPoses[2];
      GetPreCrossingPoses(preCrossingPoses[0], preCrossingPoses[1]);

      _vizHandle[1] = VizManager::getInstance()->DrawPreDockPose(GetID().GetValue(),   preCrossingPoses[0].GetWithRespectToOrigin(), VIZ_COLOR_PRERAMPPOSE);
      _vizHandle[2] = VizManager::getInstance()->DrawPreDockPose(GetID().GetValue()+1, preCrossingPoses[1].GetWithRespectToOrigin(), VIZ_COLOR_PRERAMPPOSE);
      
    } // Visualize()
    
    
    void Bridge::EraseVisualization()
    {
      for(auto & hangle : _vizHandle) {
        if(hangle != VizManager::INVALID_HANDLE) {
          VizManager::getInstance()->EraseVizObject(hangle);
          hangle = VizManager::INVALID_HANDLE;
        }
      }
    }
    
    Quad2f Bridge::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {      
      Point3f paddedSize(_size);
      paddedSize += 2.f*padding_mm;
      
      return ObservableObject::GetBoundingQuadXY_Helper(atPose, paddedSize, Bridge::_canonicalCorners);
    }

    
    
  } // namespace Cozmo
} // namespace Anki

#endif