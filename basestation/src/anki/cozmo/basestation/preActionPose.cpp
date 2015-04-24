/**
 * File: preActionPose.cpp
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Implements a "Pre-Action" pose, which is used by ActionableObjects
 *              to define a position to be in before acting on the object with a
 *              given type of ActionType.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/preActionPose.h"

#include "anki/common/basestation/colorRGBA.h"
#include "anki/vision/basestation/visionMarker.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
  
  namespace Cozmo {
    
    const ColorRGBA& PreActionPose::GetVisualizeColor(ActionType type)
    {
      static const std::map<ActionType, ColorRGBA> ColorLUT = {
        {DOCKING,      ColorRGBA(0.0f,0.0f,1.0f,0.5f)},
        {PLACEMENT,    ColorRGBA(0.0f,0.8f,0.2f,0.5f)},
        {ENTRY,        ColorRGBA(1.f,0.f,0.f,0.5f)}
      };
      
      static const ColorRGBA Default(1.0f,0.0f,0.0f,0.5f);
      
      auto iter = ColorLUT.find(type);
      if(iter == ColorLUT.end()) {
        PRINT_NAMED_WARNING("PreActionPose.GetVisualizationColor.ColorNotDefined",
                            "Color not defined for ActionType=%d. Returning default color.\n", type);
        return Default;
      } else {
        return iter->second;
      }
    } // GetVisualizeColor()
    
    void PreActionPose::SetHeightTolerance()
    {
      const Point3f T = _poseWrtMarkerParent.GetTranslation().GetAbs();
      const f32 distance = std::max(T.x(), std::max(T.y(), T.z()));
      _heightTolerance = distance * std::tan(ANGLE_TOLERANCE);
    }
    
    PreActionPose::PreActionPose(ActionType type,
                                 const Vision::KnownMarker* marker,
                                 const f32 distance)
    : PreActionPose(type, marker, Y_AXIS_3D() * -distance)
    {
      
    } // PreActionPose Constructor
    
    
    PreActionPose::PreActionPose(ActionType type,
                                 const Vision::KnownMarker* marker,
                                 const Vec3f& offset)
    : _type(type)
    , _marker(marker)
    , _poseWrtMarkerParent(M_PI_2, Z_AXIS_3D(), offset, &marker->GetPose()) // init w.r.t. marker
    {
      // Now make pose w.r.t. marker parent
      if(_poseWrtMarkerParent.GetWithRespectTo(*_marker->GetPose().GetParent(), _poseWrtMarkerParent) == false) {
        PRINT_NAMED_ERROR("PreActionPose.GetPoseWrtMarkerParentFailed",
                          "Could not get the pre-action pose w.r.t. the marker's parent.\n");
      }
      _poseWrtMarkerParent.SetName("PreActionPose");
      
      SetHeightTolerance();
      
    } // PreActionPose Constructor
    
    
    PreActionPose::PreActionPose(ActionType type,
                                 const Vision::KnownMarker* marker,
                                 const Pose3d& poseWrtMarker)
    : _type(type)
    , _marker(marker)
    {
      if(poseWrtMarker.GetParent() != &marker->GetPose()) {
        PRINT_NAMED_ERROR("PreActionPose.PoseWrtMarkerParentInvalid",
                          "Given pose w.r.t. marker should have the marker as its parent pose.\n");
      }
      if(poseWrtMarker.GetWithRespectTo(*_marker->GetPose().GetParent(), _poseWrtMarkerParent) == false) {
        PRINT_NAMED_ERROR("PreActionPose.GetPoseWrtMarkerParentFailed",
                          "Could not get the pre-action pose w.r.t. the marker's parent.\n");
      }
      _poseWrtMarkerParent.SetName("PreActionPose");
      
      SetHeightTolerance();
      
    } // PreActionPose Constructor
    
    
    PreActionPose::PreActionPose(const PreActionPose& canonicalPose,
                                 const Pose3d& markerParentPose)
    : _type(canonicalPose.GetActionType())
    , _marker(canonicalPose.GetMarker())
    , _poseWrtMarkerParent(markerParentPose*canonicalPose._poseWrtMarkerParent)
    {
      _poseWrtMarkerParent.SetParent(markerParentPose.GetParent());
      _poseWrtMarkerParent.SetName("PreActionPose");
      
      SetHeightTolerance();
    }
    
    
  } // namespace Cozmo
} // namespace Anki
