/**
 * File: actionableObject.cpp
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Defines an "Actionable" Object, which is a subclass of an
 *              ObservableObject that can also be interacted with or acted upon.
 *              It extends the (coretech) ObservableObject to have a notion of
 *              docking and entry points, for example, useful for Cozmo's.
 *              These are represented by different types of "pre-action" poses.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/actionableObject.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"

#include "anki/cozmo/shared/cozmoEngineConfig.h"

namespace Anki {
  namespace Cozmo {
        
    ActionableObject::ActionableObject(ObjectFamily family, ObjectType type)
    : ObservableObject(family,type)
    , _isBeingCarried(false)
    , _isSelected(false)
    {
      
    }
    
    
    void ActionableObject::AddPreActionPose(PreActionPose::ActionType type, const Vision::KnownMarker *marker,
                                            const f32 distance)
    {
      _preActionPoses.emplace_back(type, marker, distance);
    } // AddPreActionPose()
    
    void ActionableObject::AddPreActionPose(PreActionPose::ActionType type, const Vision::KnownMarker *marker,
                                            const Vec3f& offset)
    {
      _preActionPoses.emplace_back(type, marker, offset);
    } // AddPreActionPose()
    
    void ActionableObject::AddPreActionPose(PreActionPose::ActionType type,
                                            const Vision::KnownMarker* marker,
                                            const Pose3d& poseWrtMarker)
    {
      _preActionPoses.emplace_back(type, marker, poseWrtMarker);
    }
    
    
    bool ActionableObject::IsPreActionPoseValid(const PreActionPose& preActionPose,
                                                const Pose3d* reachableFromPose,
                                                const std::vector<std::pair<Quad2f,ObjectID> >& obstacles) const
    {
      const Pose3d checkPose = preActionPose.GetPose().GetWithRespectToOrigin();
      
      bool isValid = true;
      
      // Overloaded validity checks may use reachableFromPose, but for now, we're
      // going to disable the default check based on height
      /* TODO: Consider re-enabling default reachability check
      if(reachableFromPose != nullptr) {
        // Pose should be at ground height or it's not reachable. Let the threshold
        // vary with the distance from the pre-action pose to the object.
        isValid = NEAR(checkPose.GetTranslation().z(),
                       reachableFromPose->GetWithRespectToOrigin().GetTranslation().z(),
                       preActionPose.GetHeightTolerance());
      }
       */
      
      if(isValid) {
        // Allow any rotation around Z, but none around X/Y, in order to keep
        // vertically-oriented poses
        const f32 vertAlignThresh = 1.f - std::cos(PreActionPose::ANGLE_TOLERANCE); // TODO: tighten?
        isValid = NEAR(checkPose.GetRotationMatrix()(2,2), 1.f, vertAlignThresh);
      }
      
      if(isValid && !obstacles.empty()) {
        // Cheap hack for now (until we use the planner to do this check for us):
        //   Walk a straight line from this preActionPose to the parent object
        //   and check for intersections with the obstacle list. Actually, we will
        //   walk three lines (center, left, and right) so the caller doesn't
        //   have to do some kind of oriented padding of the obstacles to deal
        //   with objects close to the action-object unnecessarily blocking
        //   paths on other sides of the object unnecessarily.
        
        //   (Assumes obstacles are w.r.t. origin...)
        Point2f xyStart(preActionPose.GetPose().GetWithRespectToOrigin().GetTranslation());
        const Point2f xyEnd(preActionPose.GetMarker()->GetPose().GetWithRespectToOrigin().GetTranslation());
        
        const f32 stepSize = 10.f; // 1cm
        Vec2f   stepVec(xyEnd);
        stepVec -= xyStart;
        f32 lineLength = stepVec.MakeUnitLength();
        Vec2f offsetVec(stepVec.y(), -stepVec.x());
        offsetVec *= 0.5f*ROBOT_BOUNDING_Y;
        
        // Pull back xyStart to the rear of the robot's bounding box when the robot is at the preaction pose.
        xyStart -= (stepVec * (ROBOT_BOUNDING_X - ROBOT_BOUNDING_X_FRONT));
        lineLength += (ROBOT_BOUNDING_X - ROBOT_BOUNDING_X_FRONT);
        
        const s32 numSteps = std::floor(lineLength / stepSize);
        
        stepVec *= stepSize;
        
        
        bool pathClear = true;
        Point2f currentPoint(xyStart);
        Point2f currentPointL(xyStart + offsetVec);
        Point2f currentPointR(xyStart - offsetVec);
        for(s32 i=0; i<numSteps && pathClear; ++i) {
          // Check whether the current point along the line is inside any obstacles
          // (excluding this ActionableObject as an obstacle)
          
          // DEBUG VIZ
          //          VizManager::getInstance()->DrawGenericQuad(i+1000+(0xffff & (long)preActionPose.GetMarker()),
          //                                                     Quad2f(currentPoint+Point2f(-1.f,-1.f),
          //                                                            currentPoint+Point2f(-1.f, 1.f),
          //                                                            currentPoint+Point2f( 1.f,-1.f),
          //                                                            currentPoint+Point2f( 1.f, 1.f)),
          //                                                     1.f, NamedColors::BLUE);
          //          VizManager::getInstance()->DrawGenericQuad(i+2000+(0xffff & (long)preActionPose.GetMarker()),
          //                                                     Quad2f(currentPointL+Point2f(-1.f,-1.f),
          //                                                            currentPointL+Point2f(-1.f, 1.f),
          //                                                            currentPointL+Point2f( 1.f,-1.f),
          //                                                            currentPointL+Point2f( 1.f, 1.f)),
          //                                                     1.f, NamedColors::RED);
          //          VizManager::getInstance()->DrawGenericQuad(i+3000+(0xffff & (long)preActionPose.GetMarker()),
          //                                                     Quad2f(currentPointR+Point2f(-1.f,-1.f),
          //                                                            currentPointR+Point2f(-1.f, 1.f),
          //                                                            currentPointR+Point2f( 1.f,-1.f),
          //                                                            currentPointR+Point2f( 1.f, 1.f)),
          //                                                     1.f, NamedColors::GREEN);
          
          // Technically, this quad is already in the list of obstacles, so we could
          // find it rather than recomputing it...
          const Quad2f boundingQuad = GetBoundingQuadXY();
          
          for(auto & obstacle : obstacles) {
            
            // DEBUG VIZ
            //            VizManager::getInstance()->DrawGenericQuad(obstacle.second.GetValue(), obstacle.first, 1.f, NamedColors::ORANGE);
            
            // Make sure this obstacle is not from this object (the one we are trying to interact with).
            if(obstacle.second != this->GetID()) {
              // Also make sure the obstacle is not part of a stack this one belongs
              // to, by seeing if its centroid is contained within this object's
              // bounding quad
              if(boundingQuad.Contains(obstacle.first.ComputeCentroid()) == false) {
                if(obstacle.first.Contains(currentPoint)  ||
                   obstacle.first.Contains(currentPointR) ||
                   obstacle.first.Contains(currentPointL)) {
                  pathClear = false;
                  break;
                }
              }
            }
          }
          // Take a step along the line
          assert( ((currentPoint +stepVec)-xyEnd).Length() < (currentPoint -xyEnd).Length());
          assert( ((currentPointL+stepVec)-xyEnd).Length() < (currentPointL-xyEnd).Length());
          assert( ((currentPointR+stepVec)-xyEnd).Length() < (currentPointR-xyEnd).Length());
          currentPoint  += stepVec;
          currentPointR += stepVec;
          currentPointL += stepVec;
        }

        if(pathClear == false) {
          isValid = false;
        }
      
      }
      
      return isValid;
      
    } // IsPreActionPoseValid()
    
    
    void ActionableObject::GetCurrentPreActionPoses(std::vector<PreActionPose>& preActionPoses,
                                                    const std::set<PreActionPose::ActionType>& withAction,
                                                    const std::set<Vision::Marker::Code>& withCode,
                                                    const std::vector<std::pair<Quad2f,ObjectID> >& obstacles,
                                                    const Pose3d* reachableFromPose,
                                                    const f32 offset_mm)
    {
      const Pose3d& relToObjectPose = GetPose();
      
      for(auto & preActionPose : _preActionPoses)
      {
        if((withCode.empty()   || withCode.count(preActionPose.GetMarker()->GetCode()) > 0) &&
           (withAction.empty() || withAction.count(preActionPose.GetActionType()) > 0))
        {
          // offset_mm is scaled by some amount because otherwise it might too far to see the marker
          // it's docking to.
          PreActionPose currentPose(preActionPose, relToObjectPose, PREACTION_POSE_OFFSET_SCALAR * offset_mm);
          
          if(IsPreActionPoseValid(currentPose, reachableFromPose, obstacles)) {
            preActionPoses.emplace_back(currentPose);
          }
        } // if preActionPose has correct code/action
      } // for each preActionPose
      
    } // GetCurrentPreActionPoses()
    
    
    void ActionableObject::Visualize()
    {
      if(IsSelected()) {
        static const ColorRGBA SELECTED_COLOR(0.f,1.f,0.f,1.f);
        Visualize(SELECTED_COLOR);
        //VisualizePreActionPoses();
      } else {
        Visualize(GetColor());
      }
    }
    
    void ActionableObject::VisualizePreActionPoses(const std::vector<std::pair<Quad2f,ObjectID> >& obstacles,
                                                   const Pose3d* reachableFrom)
    {
      // Draw the main object:
      //Visualize();
      ActionableObject::EraseVisualization();
      
      // Draw the pre-action poses, using a different color for each type of action
      u32 poseID = 0;
      std::vector<PreActionPose> poses;
      
      for(PreActionPose::ActionType actionType : {PreActionPose::DOCKING, PreActionPose::ENTRY})
      {
        GetCurrentPreActionPoses(poses, {actionType}, std::set<Vision::Marker::Code>(), obstacles, reachableFrom);
        for(auto & pose : poses) {
          // TODO: In computing poseID to pass to DrawPreDockPose, multiply object ID by the max number of
          //       preaction poses we expect to visualize per object. Currently, hardcoded to 48 (4 dock and
          //       4 roll per side). We probably won't have more than this.
          _vizPreActionPoseHandles.emplace_back(VizManager::getInstance()->DrawPreDockPose(poseID + GetID().GetValue()*48,
                                                                                           pose.GetPose().GetWithRespectToOrigin(),
                                                                                           PreActionPose::GetVisualizeColor(actionType)));
          ++poseID;
        }
        
        poses.clear();
      } // for each actionType
      
    } // VisualizeWithPreActionPoses()
    
    
    void ActionableObject::EraseVisualization()
    {
      for(auto & preActionPoseHandle : _vizPreActionPoseHandles) {
        if(preActionPoseHandle != VizManager::INVALID_HANDLE) {
          VizManager::getInstance()->EraseVizObject(preActionPoseHandle);
        }
      }
      _vizPreActionPoseHandles.clear();
    }
    
    
  } // namespace Cozmo
} // namespace Anki
