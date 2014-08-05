/**
 * File: dockableObject.cpp
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Defines a "Dockable" Object, which is a subclass of an
 *              ObservableObject that can also be docked with.
 *              It extends the (coretech) ObservableObject to have a notion of
 *              docking for Cozmo's purposes, by offering, for example, the
 *              ability to request pre-dock poses.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "dockableObject.h"

namespace Anki {
  namespace Cozmo {
    
    DockableObject::DockableObject(ObjectType type)
    : Vision::ObservableObject(type)
    , _isBeingCarried(false)
    {
      
    }
    
    bool DockableObject::GetPreDockPose(const Point3f& canonicalPoint,
                                        const float distance_mm,
                                        Pose3d& preDockPose) const
    {
      bool dockingPointFound = false;
      
      // Compute this point's position at this distance according to this
      // block's current pose
      Point3f dockingPt(canonicalPoint);  // start with canonical point
      dockingPt *= distance_mm;           // scale to specified distance
      dockingPt =  GetPose() * dockingPt; // transform to block's pose
      
      //
      // Check if it's vertically oriented
      //
      const float DOT_TOLERANCE   = .35f;
      
      // Get vector, v, from center of block to this point
      Point3f v(dockingPt);
      v -= GetPose().GetTranslation();
      
      // Dot product of this vector with the z axis should be near zero
      // TODO: make dot product tolerance in terms of an angle?
      if( NEAR(DotProduct(Z_AXIS_3D, v), 0.f,  distance_mm * DOT_TOLERANCE) ) {
        
        /*
         // Rotation of block around v should be a multiple of 90 degrees
         const float ANGLE_TOLERANCE = DEG_TO_RAD(35);
         const float angX = ABS(blockPose.GetRotationAngle<'X'>().ToFloat());
         const float angY = ABS(blockPose.GetRotationAngle<'Y'>().ToFloat());
         
         const bool angX_mult90 = (NEAR(angX, 0.f,             ANGLE_TOLERANCE) ||
         NEAR(angX, DEG_TO_RAD(90),  ANGLE_TOLERANCE) ||
         NEAR(angX, DEG_TO_RAD(180), ANGLE_TOLERANCE) ||
         NEAR(angX, DEG_TO_RAD(360), ANGLE_TOLERANCE));
         const bool angY_mult90 = (NEAR(angY, 0.f,             ANGLE_TOLERANCE) ||
         NEAR(angY, DEG_TO_RAD(90),  ANGLE_TOLERANCE) ||
         NEAR(angY, DEG_TO_RAD(180), ANGLE_TOLERANCE) ||
         NEAR(angY, DEG_TO_RAD(360), ANGLE_TOLERANCE));
         
         if(angX_mult90 && angY_mult90)
         {
         dockingPt.z() = 0.f;  // Project to floor plane
         preDockPose.SetTranslation(dockingPt);
         preDockPose.SetRotation(atan2f(-v.y(), -v.x()), Z_AXIS_3D);
         dockingPointFound = true;
         }
         */
        
        // TODO: The commented out logic above appears not to be accounting
        // for rotation ambiguity of blocks. Just accept this is a valid dock pose for now.
        dockingPt.z() = 0.f;  // Project to floor plane
        preDockPose.SetTranslation(dockingPt);
        preDockPose.SetRotation(atan2f(-v.y(), -v.x()), Z_AXIS_3D);
        preDockPose.SetParent(GetPose().GetParent());
        dockingPointFound = true;
        
      }
      
      return dockingPointFound;
    }  // GetPreDockPose()
    
    
    void DockableObject::Visualize(const VIZ_COLOR_ID color, const f32 preDockPoseDistance)
    {
      // Draw the main object:
      Visualize(color);
      
      // Draw the pre-dock poses
      if(preDockPoseDistance > 0.f) {
        u32 poseID = 0;
        std::vector<DockableObject::PoseMarkerPair_t> poses;
        GetPreDockPoses(preDockPoseDistance, poses);
        for(auto pose : poses) {
          _vizPreDockPoseHandles.emplace_back(VizManager::getInstance()->DrawPreDockPose(poseID,
                                                                                         pose.first.GetWithRespectToOrigin(),
                                                                                         VIZ_COLOR_PREDOCKPOSE));
          ++poseID;
        }
      }
    }
    
    void DockableObject::EraseVisualization()
    {
      for(auto & preDockPoseHandle : _vizPreDockPoseHandles) {
        if(preDockPoseHandle != VizManager::INVALID_HANDLE) {
          VizManager::getInstance()->EraseVizObject(preDockPoseHandle);
        }
      }
      _vizPreDockPoseHandles.clear();
    }
    
    
  } // namespace Cozmo
} // namespace Anki