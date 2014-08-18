/**
 * File: mat.cpp
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Implements a MatPiece object, which is a "mat" that Cozmo drives
 *              around on with VisionMarkers at known locations for localization.
 *
 *              MatPiece inherits from ActionableObject since mats may have
 *              action poses for "entering" the mat, for example.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/



#include "anki/cozmo/basestation/mat.h"

#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"

namespace Anki {
  namespace Cozmo {
    
    // Instantiate static const MatPiece types here:
    const MatPiece::Type MatPiece::Type::LETTERS_4x4("LETTERS_4x4");
    const MatPiece::Type MatPiece::Type::LARGE_PLATFORM("LARGE_PLATFORM");
    const MatPiece::Type MatPiece::Type::LONG_BRIDGE("LONG_BRIDGE");
    const MatPiece::Type MatPiece::Type::SHORT_BRIDGE("SHORT_BRIDGE");
    
    
    // MatPiece has no rotation ambiguities but we still need to define this
    // static const here to instatiate an empty list.
    const std::vector<RotationMatrix3d> MatPiece::_rotationAmbiguities;
    
    const std::array<Point3f, MatPiece::NUM_CORNERS> MatPiece::_canonicalCorners = {{
      Point3f(-0.5f, -0.5f,  0.5f),
      Point3f( 0.5f, -0.5f,  0.5f),
      Point3f(-0.5f, -0.5f, -0.5f),
      Point3f( 0.5f, -0.5f, -0.5f),
      Point3f(-0.5f,  0.5f,  0.5f),
      Point3f( 0.5f,  0.5f,  0.5f),
      Point3f(-0.5f,  0.5f, -0.5f),
      Point3f( 0.5f,  0.5f, -0.5f)
    }};
    
    
    MatPiece::MatPiece(Type type)
    : _type(type)
    {
      
      // TODO: Use a MatTypeLUT and MatDefinitions file, like we do with blocks
      if(Type::LETTERS_4x4 == _type) {
        
        _size = {1000.f, 1000.f, 2.5f};
        _isMoveable = false;
        
        //#include "anki/cozmo/basestation/Mat_AnkiLogoPlus8Bits_8x8.def"
#       include "anki/cozmo/basestation/Mat_Letters_30mm_4x4.def"
     
      }
      else if(Type::LARGE_PLATFORM == _type) {
        
        _size = {240.f, 240.f, 44.f};
        _isMoveable = true;
        
        const f32& length = _size.x();
        const f32& width  = _size.y();
        const f32& height = _size.z();

        const f32 markerSize_sides = 25.f;
        const f32 markerSize_top   = 25.f;
        
        // Front Face
        AddMarker(Vision::MARKER_PLATFORMNORTH, // Vision::MARKER_INVERTED_E,
                  Pose3d(M_PI_2, Z_AXIS_3D, {length*.5f, 0.f, 0.f}),
                  markerSize_sides);
        
        // Back Face
        AddMarker(Vision::MARKER_PLATFORMSOUTH, //Vision::MARKER_INVERTED_RAMPBACK,
                  Pose3d(-M_PI_2, Z_AXIS_3D, {-length*.5f, 0.f, 0.f}),
                  markerSize_sides);

        // Right Face
        AddMarker(Vision::MARKER_PLATFORMEAST, //Vision::MARKER_INVERTED_RAMPRIGHT,
                  Pose3d(M_PI, Z_AXIS_3D, {0, width*.5f, 0}),
                  markerSize_sides);
        
        // Left Face
        AddMarker(Vision::MARKER_PLATFORMWEST, //Vision::MARKER_INVERTED_RAMPLEFT,
                  Pose3d(0.f, Z_AXIS_3D, {0, -width*.5f, 0}),
                  markerSize_sides);
        
        // Top Faces:
        AddMarker(Vision::MARKER_INVERTED_A, //TODO: Vision::MARKER_ANKILOGOWITHBITS001,
                  Pose3d(-M_PI_2, X_AXIS_3D, {-length*.25f, -width*.25f, height*.5f}),
                  markerSize_top);
        AddMarker(Vision::MARKER_INVERTED_B, //TODO: Vision::MARKER_ANKILOGOWITHBITS010,
                  Pose3d(-M_PI_2, X_AXIS_3D, {-length*.25f,  width*.25f, height*.5f}),
                  markerSize_top);
        AddMarker(Vision::MARKER_INVERTED_C, //TODO: Vision::MARKER_ANKILOGOWITHBITS020,
                  Pose3d(-M_PI_2, X_AXIS_3D, { length*.25f, -width*.25f, height*.5f}),
                  markerSize_top);
        AddMarker(Vision::MARKER_INVERTED_D, //TODO: Vision::MARKER_ANKILOGOWITHBITS030, 
                  Pose3d(-M_PI_2, X_AXIS_3D, { length*.25f,  width*.25f, height*.5f}),
                  markerSize_top);
      }
      else if(Type::LONG_BRIDGE == _type || Type::SHORT_BRIDGE == _type) {
        
        _isMoveable = true;
        
        Vision::MarkerType leftMarkerType, rightMarkerType, middleMarkerType;
        f32 markerSize = 0.f;
        f32 length = 0.f;
        
        if(Type::LONG_BRIDGE == _type) {
          length = 300.f;
          markerSize = 25.f;
          
          leftMarkerType   = Vision::MARKER_BRIDGESUNLEFT;
          rightMarkerType  = Vision::MARKER_BRIDGESUNRIGHT;
          middleMarkerType = Vision::MARKER_BRIDGESUNMIDDLE;
        }
        else if(Type::SHORT_BRIDGE == _type) {
          length = 200.f;          
          markerSize = 25.f;
          
          leftMarkerType   = Vision::MARKER_BRIDGEMOONLEFT;
          rightMarkerType  = Vision::MARKER_BRIDGEMOONRIGHT;
          middleMarkerType = Vision::MARKER_BRIDGEMOONMIDDLE;
        }
        else {
          PRINT_NAMED_ERROR("MatPiece.BridgeUnexpectedElse", "Should not get to else in if ladder constructing bridge-type mat.\n");
          return;
        }
        
        _size = {length, 62.f, 1.f};
        
        Pose3d preCrossingPoseLeft(0, Z_AXIS_3D, {-_size.x()*.5f-30.f, 0.f, _size.z()}, &GetPose());
        Pose3d preCrossingPoseRight(M_PI, Z_AXIS_3D, {_size.x()*.5f+30.f, 0.f, _size.z()}, &GetPose());

        //Pose3d leftMarkerPose(-M_PI_2, Z_AXIS_3D, {-_size.x()*.5f+markerSize, 0.f, _size.z()});
        //leftMarkerPose *= Pose3d(-M_PI_2, X_AXIS_3D, {0.f, 0.f, 0.f});
        Pose3d leftMarkerPose(-M_PI_2, X_AXIS_3D, {-_size.x()*.5f+markerSize, 0.f, _size.z()});
        
        //Pose3d rightMarkerPose(M_PI_2, Z_AXIS_3D, { _size.x()*.5f-markerSize, 0.f, _size.z()});
        //rightMarkerPose *= Pose3d(-M_PI_2, X_AXIS_3D, {0.f, 0.f, 0.f});
        Pose3d rightMarkerPose(-M_PI_2, X_AXIS_3D, { _size.x()*.5f-markerSize, 0.f, _size.z()});
        
        const Vision::KnownMarker* leftMarker  = &AddMarker(leftMarkerType,  leftMarkerPose,  markerSize);
        const Vision::KnownMarker* rightMarker = &AddMarker(rightMarkerType, rightMarkerPose, markerSize);
        AddMarker(middleMarkerType, Pose3d(-M_PI_2, X_AXIS_3D, {0.f, 0.f, _size.z()}), markerSize);
        
        CORETECH_ASSERT(leftMarker != nullptr);
        CORETECH_ASSERT(rightMarker != nullptr);
        
        if(preCrossingPoseLeft.GetWithRespectTo(leftMarker->GetPose(), preCrossingPoseLeft) == false) {
          PRINT_NAMED_ERROR("MatPiece.PreCrossingPoseLeftError", "Could not get preCrossingLeftPose w.r.t. left bridge marker.\n");
        }
        AddPreActionPose(PreActionPose::ENTRY, leftMarker, preCrossingPoseLeft);
        
        if(preCrossingPoseRight.GetWithRespectTo(rightMarker->GetPose(), preCrossingPoseRight) == false) {
          PRINT_NAMED_ERROR("MatPiece.PreCrossingPoseRightError", "Could not get preCrossingRightPose w.r.t. right bridge marker.\n");
        }
        AddPreActionPose(PreActionPose::ENTRY, rightMarker, preCrossingPoseRight);

        
      } // bridge types
      else {
        PRINT_NAMED_ERROR("MatPiece.UnrecognizedType",
                          "Trying to instantiate a MatPiece with an unknown Type = %d.\n", int(type));
      }
      
      
    } // MatPiece(type) Constructor
    
    
    Point3f MatPiece::GetSameDistanceTolerance() const
    {
       if(Type::LETTERS_4x4  == GetType() ||
          Type::LONG_BRIDGE  == GetType() ||
          Type::SHORT_BRIDGE == GetType())
       {
         // "Thin" mats: don't use half the thickness as the height tolerance (too strict)
         return Point3f(_size.x()*.5f, _size.y()*.5f, 25.f);
       }
       else if(Type::LARGE_PLATFORM == GetType()) {
         return _size*.5f;
       } else {
         PRINT_NAMED_ERROR("MatPiece.GetSameDistanceTolerance.UnrecognizedType",
                           "Trying to get same-distance tolerance for a MatPiece with an unknown Type = %d.\n", int(GetType()));
         return Point3f();
       }
    }
    
    
    Radians MatPiece::GetSameAngleTolerance() const {
      return DEG_TO_RAD(45); // TODO: too loose?
    }

    
    std::vector<RotationMatrix3d> const& MatPiece::GetRotationAmbiguities() const
    {
      return MatPiece::_rotationAmbiguities;
    }
    
    void MatPiece::Visualize(const ColorRGBA& color)
    {
      Pose3d vizPose = GetPose().GetWithRespectToOrigin();
      _vizHandle = VizManager::getInstance()->DrawCuboid(GetID().GetValue(), _size, vizPose, color);
    }
    
    void MatPiece::EraseVisualization()
    {
      // Erase the main object
      if(_vizHandle != VizManager::INVALID_HANDLE) {
        VizManager::getInstance()->EraseVizObject(_vizHandle);
        _vizHandle = VizManager::INVALID_HANDLE;
      }
    }
    
    Quad2f MatPiece::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {
      Point3f paddedSize(_size);
      paddedSize += 2.f*padding_mm;
     
      return ObservableObject::GetBoundingQuadXY_Helper(atPose, paddedSize, MatPiece::_canonicalCorners);
    }
    
    
    ObjectType MatPiece::GetTypeByName(const std::string& name)
    {
      // TODO: Support other types/names
      if(name == "LETTERS_4x4") {
        return MatPiece::Type::LETTERS_4x4;
      } else if(name == "LARGE_PLATFORM") {
        return MatPiece::Type::LARGE_PLATFORM;
      } else {
        PRINT_NAMED_ERROR("MatPiece.NoTypeForName",
                          "No MatPiece Type registered for name '%s'.\n", name.c_str());
        return ObjectType::GetInvalidType();
      }
    }
    
    void MatPiece::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const {
      corners.resize(NUM_CORNERS);
      for(s32 i=0; i<NUM_CORNERS; ++i) {
        corners[i] = MatPiece::_canonicalCorners[i];
        corners[i] *= _size;
        corners[i] = atPose * corners[i];
      }
    }
    
    
    bool MatPiece::IsPoseOn(const Anki::Pose3d &pose, const f32 heightOffset, const f32 heightTol) const
    {
      Pose3d poseWrtMat;
      return IsPoseOn(pose, heightOffset, heightTol, poseWrtMat);
    }
    
    bool MatPiece::IsPoseOn(const Anki::Pose3d &pose, const f32 heightOffset, const f32 heightTol, Pose3d& poseWrtMat) const
    {
      if(pose.GetWithRespectTo(GetPose(), poseWrtMat) == false) {
        return false;
      }
      
      const Point2f pt(poseWrtMat.GetTranslation().x(), poseWrtMat.GetTranslation().y());
      const bool withinBBox   = GetBoundingQuadXY(Pose3d()).Contains(pt);
      
      const bool withinHeight = NEAR(poseWrtMat.GetTranslation().z(), _size.z()*.5f + heightOffset, heightTol);
      
      // Make sure the given pose's rotation axis is well aligned with the mat's Z axis
      // TODO: make alignment a parameter?
      // TODO: const bool zAligned     = poseWrtMat.GetRotationAxis().z() >= std::cos(DEG_TO_RAD(25));
      const bool zAligned = true;
      
      return withinBBox && withinHeight && zAligned;
      
    } // IsPoseOn()
    
    f32 MatPiece::GetDrivingSurfaceHeight() const
    {
      //Pose3d poseWrtOrigin = GetPose().GetWithRespectToOrigin();
      return _size.z()*.5f;// + poseWrtOrigin.GetTranslation().z();
    } // GetDrivingSurfaceHeight()
    
  } // namespace Cozmo
  
} // namespace Anki
