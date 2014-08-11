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
    , _preCrossingPoseLeft(0, Z_AXIS_3D, {-_size.x()*.5f-30.f, 0.f, _size.z()})
    , _preCrossingPoseRight(M_PI, Z_AXIS_3D, {_size.x()*.5f+30.f, 0.f, _size.z()})
    {

      Vision::MarkerType leftMarker, rightMarker, middleMarker;
      f32 markerSize = 0.f;
      
      if(Type::LONG == _type) {
        _size = {300.f, 62.f, 3.f};

        markerSize = 25.f;
        
        leftMarker   = Vision::MARKER_BRIDGESUNLEFT;
        rightMarker  = Vision::MARKER_BRIDGESUNRIGHT;
        middleMarker = Vision::MARKER_BRIDGESUNMIDDLE;
      }
      else if(Type::SHORT == _type) {
        _size = {200.f, 62.f, 3.f};
        
        markerSize = 25.f;
        
        leftMarker   = Vision::MARKER_BRIDGEMOONLEFT;
        rightMarker  = Vision::MARKER_BRIDGEMOONRIGHT;
        middleMarker = Vision::MARKER_BRIDGEMOONMIDDLE;
      }
      else {
        PRINT_NAMED_ERROR("Bridge.UnknownType", "Tried to construct unknown Bridge type.\n");
        return;
      }

      Pose3d leftMarkerPose(-M_PI_2, Z_AXIS_3D, {-_size.x()*.5f+markerSize, 0.f, _size.z()});
      leftMarkerPose *= Pose3d(M_PI_2, X_AXIS_3D, {0.f, 0.f, 0.f});
      
      Pose3d rightMarkerPose(M_PI_2, Z_AXIS_3D, { _size.x()*.5f-markerSize, 0.f, _size.z()});
      rightMarkerPose *= Pose3d(M_PI_2, X_AXIS_3D, {0.f, 0.f, 0.f});
      
      _leftEndMarker  = &AddMarker(leftMarker,  leftMarkerPose,  markerSize);
      _rightEndMarker = &AddMarker(rightMarker, rightMarkerPose, markerSize);
      AddMarker(middleMarker, Pose3d(M_PI_2, X_AXIS_3D, {0.f, 0.f, _size.z()}), markerSize);
      
      CORETECH_ASSERT(_leftEndMarker != nullptr);
      CORETECH_ASSERT(_rightEndMarker != nullptr);
      
      if(_preCrossingPoseLeft.GetWithRespectTo(_leftEndMarker->GetPose(), _preCrossingPoseLeft) == false) {
        PRINT_NAMED_ERROR("Ramp.PreCrossingPoseLeftError", "Could not get preCrossingLeftPose w.r.t. left bridge marker.\n");
      }
      AddPreActionPose(PreActionPose::ENTRY, _leftEndMarker, _preCrossingPoseLeft);
      
      if(_preCrossingPoseRight.GetWithRespectTo(_rightEndMarker->GetPose(), _preCrossingPoseRight) == false) {
        PRINT_NAMED_ERROR("Ramp.PreCrossingPoseRightError", "Could not get preCrossingRightPose w.r.t. right bridge marker.\n");
      }
      AddPreActionPose(PreActionPose::ENTRY, _rightEndMarker, _preCrossingPoseRight);
      
    } // Bridge Constructor
    
    Bridge::Bridge(const Bridge& otherBridge)
    : Bridge(otherBridge._type)
    {
      SetPose(otherBridge.GetPose());
    }
    
    Bridge::~Bridge()
    {
      EraseVisualization();
    }
    
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
    
    void Bridge::Visualize(const ColorRGBA& color)
    {
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      
      _vizHandle = VizManager::getInstance()->DrawCuboid(GetID().GetValue(), _size, GetPose(), color);
      
    } // Visualize()
    
    
    void Bridge::EraseVisualization()
    {
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        VizManager::getInstance()->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
      
    }
    
    Quad2f Bridge::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {      
      Point3f paddedSize(_size);
      paddedSize += 2.f*padding_mm;
      
      return ObservableObject::GetBoundingQuadXY_Helper(atPose, paddedSize, Bridge::_canonicalCorners);
    }

    Point3f Bridge::GetSameDistanceTolerance() const
    {
      if(Type::LETTERS_4x4 == GetType()) {
        // Thin mat: don't use half the thickness as the height tolerance (too strict)
        return Point3f(_size.x()*.5f, _size.y()*.5f, 15.f);
      }
      else if(Type::LARGE_PLATFORM == GetType()) {
        return _size*.5f;
      } else {
        PRINT_NAMED_ERROR("MatPiece.GetSameDistanceTolerance.UnrecognizedType",
                          "Trying to get same-distance tolerance for a MatPiece with an unknown Type = %d.\n", int(GetType()));
        return Point3f();
      }
    }
    
    Radians Bridge::GetSameAngleTolerance() const {
      return DEG_TO_RAD(45); // TODO: too loose?
    }

    
    
  } // namespace Cozmo
} // namespace Anki
#endif
