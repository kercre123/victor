//
//  robotPoseHistory.cpp
//  Products_Cozmo
//
//  Created by Kevin Yoon 2014-05-13
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "robotPoseHistory.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#define DEBUG_ROBOT_POSE_HISTORY 0

namespace Anki {
  namespace Cozmo {

    //////////////////////// RobotPoseStamp /////////////////////////
    
    RobotPoseStamp::RobotPoseStamp()
    {
      SetAll(0, Pose3d(), 0.0f, 0.0f, false);
    }
    

    RobotPoseStamp::RobotPoseStamp(const PoseFrameID_t frameID,
                                   const Pose3d& pose,
                                   const f32 head_angle,
                                   const f32 lift_angle,
                                   const bool isCarryingObject)
    {
      SetAll(frameID, pose, head_angle, lift_angle, isCarryingObject);
    }
    
    void RobotPoseStamp::SetAll(const PoseFrameID_t frameID,
                                 const Pose3d& pose,
                                 const f32 head_angle,
                                 const f32 lift_angle,
                                 const bool isCarryingObject)
    {
      frame_ = frameID;
      pose_ = pose;
      headAngle_ = head_angle;
      liftAngle_ = lift_angle;
      _isCarryingObject = isCarryingObject;
    }
    
    void RobotPoseStamp::Print() const
    {
      printf("Frame %d, headAng %f, carrying %s", frame_, headAngle_, _isCarryingObject?"yes":"no");
      pose_.Print();
    }
    
    /////////////////////// RobotPoseHistory /////////////////////////////
    
    HistPoseKey RobotPoseHistory::currHistPoseKey_ = 0;
    
    RobotPoseHistory::RobotPoseHistory()
    : windowSize_(3000)
    {

    }

    void RobotPoseHistory::Clear()
    {
      poses_.clear();
      visPoses_.clear();
      computedPoses_.clear();
      
      tsByKeyMap_.clear();
      keyByTsMap_.clear();
    }
    
    void RobotPoseHistory::SetTimeWindow(const u32 windowSize_ms)
    {
      windowSize_ = windowSize_ms;
      CullToWindowSize();
    }
    
    
    Result RobotPoseHistory::AddRawOdomPose(const TimeStamp_t t,
                                            const RobotPoseStamp& p)
    {
      if(p.GetPose().GetParent() != nullptr && !p.GetPose().GetParent()->IsOrigin()) {
        PRINT_NAMED_ERROR("RobotPoseHistory.AddRawOdomPose.NonFlattenedPose",
                          "Pose object inside pose stamp should be flattened (%s)",
                          p.GetPose().GetNamedPathToOrigin(false).c_str());
        return RESULT_FAIL;
      }

      TimeStamp_t newestTime = poses_.rbegin()->first;
      if (newestTime > windowSize_ && t < newestTime - windowSize_) {
        PRINT_NAMED_WARNING("RobotPoseHistory.AddRawOdomPose.TimeTooOld", "newestTime %d, oldestAllowedTime %d, t %d", newestTime, newestTime - windowSize_, t);
        return RESULT_FAIL;
      }
      
      std::pair<PoseMapIter_t, bool> res;
      res = poses_.emplace(t, p);

      if (!res.second) {
        PRINT_NAMED_WARNING("RobotPoseHistory.AddRawOdomPose.AddFailed", "Time: %d", t);
        return RESULT_FAIL;
      }

      CullToWindowSize();
      
      return RESULT_OK;
    }

    Result RobotPoseHistory::AddVisionOnlyPose(const TimeStamp_t t,
                                               const RobotPoseStamp& p)
    {
      if(p.GetPose().GetParent() != nullptr && !p.GetPose().GetParent()->IsOrigin()) {
        PRINT_NAMED_ERROR("RobotPoseHistory.AddVisionOnlyPose.NonFlattenedPose",
                          "Pose object inside pose stamp should be flattened (%s)",
                          p.GetPose().GetNamedPathToOrigin(false).c_str());
        return RESULT_FAIL;
      }

      // Check if the pose's timestamp is too old.
      if (!poses_.empty()) {
        TimeStamp_t newestTime = poses_.rbegin()->first;
        if (newestTime > windowSize_ && t < newestTime - windowSize_) {
          PRINT_NAMED_ERROR("RobotPoseHistory.AddVisionOnlyPose.TooOld",
                            "Pose at t=%d too old to add. Newest time=%d, windowSize=%d",
                            t, newestTime, windowSize_);
          return RESULT_FAIL;
        }
      }
      
      // If visPose entry exist at t, then overwrite it
      PoseMapIter_t it = visPoses_.find(t);
      if (it != visPoses_.end()) {
        it->second.SetAll(p.GetFrameId(), p.GetPose(), p.GetHeadAngle(), p.GetLiftAngle(), p.IsCarryingObject());
      } else {
      
        std::pair<PoseMapIter_t, bool> res;
        res = visPoses_.emplace(t, p);
      
        if (!res.second) {
          PRINT_NAMED_ERROR("RobotPoseHistory.AddVisionOnlyPose.EmplaceFailed",
                            "Emplace of pose with t=%d, frameID=%d failed",
                            t, p.GetFrameId());
          return RESULT_FAIL;
        }
        
        CullToWindowSize();
      }
      
      return RESULT_OK;
    }

    
    Result RobotPoseHistory::GetRawPoseBeforeAndAfter(const TimeStamp_t t,
                                                      TimeStamp_t&    t_before,
                                                      RobotPoseStamp& p_before,
                                                      TimeStamp_t&    t_after,
                                                      RobotPoseStamp& p_after)
    {
      // Get the iterator for time t
      const_PoseMapIter_t it = poses_.lower_bound(t);
      
      if (it == poses_.begin() || it == poses_.end() || t < poses_.begin()->first) {
        return RESULT_FAIL;
      }

      // Get iterator to pose just before t
      const_PoseMapIter_t prev_it = it;
      --prev_it;
      
      t_before = prev_it->first;
      p_before = prev_it->second;
      
      // Get iterator to pose just after t
      const_PoseMapIter_t next_it = it;
      ++next_it;
      
      if (next_it == poses_.end()) {
        return RESULT_FAIL;
      }
      
      t_after = next_it->first;
      p_after = next_it->second;
      
      return RESULT_OK;
    }
    
    // Sets p to the pose nearest the given timestamp t.
    // Interpolates pose if withInterpolation == true.
    // Returns OK if t is between the oldest and most recent timestamps stored.
    Result RobotPoseHistory::GetRawPoseAt(const TimeStamp_t t_request,
                                          TimeStamp_t& t, RobotPoseStamp& p,
                                          bool withInterpolation) const
    {
      // This pose occurs at or immediately after t_request
      const_PoseMapIter_t it = poses_.lower_bound(t_request);
      
      // Check if in range
      if (it == poses_.end() || t_request < poses_.begin()->first) {
        return RESULT_FAIL;
      }
      
      if (t_request == it->first) {
        // If the exact timestamp was found, return the corresponding pose.
        t = it->first;
        p = it->second;
      } else {

        // Get iterator to the pose just before t_request
        const_PoseMapIter_t prev_it = it;
        --prev_it;

        // Get the pose transform between the two poses.
        Pose3d pTransform;
        const bool inSameOrigin = it->second.GetPose().GetWithRespectTo(prev_it->second.GetPose(), pTransform);
        if(!inSameOrigin)
        {
          PRINT_NAMED_INFO("RobotPoseHistory.GetRawPoseAt.MisMatchedOrigins",
                            "Cannot interpolate at t=%u as requested because the two poses don't share the same origin: prev=%s vs next=%s",
                           t_request,
                           prev_it->second.GetPose().FindOrigin().GetName().c_str(),
                           it->second.GetPose().FindOrigin().GetName().c_str());
          
          // they asked us for a t_request that is between two origins. We can't interpolate or decide which origin is
          // "right" for you, so, we are going to fail
          return RESULT_FAIL_ORIGIN_MISMATCH;
        }
        
        if(withInterpolation)
        {
          // Compute scale factor between time to previous pose and time between previous pose and next pose.
          f32 timeScale = (f32)(t_request - prev_it->first) / (it->first - prev_it->first);
          
          // Compute scaled transform
          Vec3f interpTrans(prev_it->second.GetPose().GetTranslation());
          interpTrans += pTransform.GetTranslation() * timeScale;
          
          // NOTE: Assuming there is only z-axis rotation!
          // TODO: Make generic?
          Radians interpRotation = prev_it->second.GetPose().GetRotationAngle<'Z'>() + Radians(pTransform.GetRotationAngle<'Z'>() * timeScale);
          
          // Interp head angle
          f32 interpHeadAngle = prev_it->second.GetHeadAngle() + timeScale * (it->second.GetHeadAngle() - prev_it->second.GetHeadAngle());
        
          // Interp lift angle
          f32 interpLiftAngle = prev_it->second.GetLiftAngle() + timeScale * (it->second.GetLiftAngle() - prev_it->second.GetLiftAngle());
          
          t = t_request;
//          p.SetPose(prev_it->second.GetFrameId(), interpTrans.x(), interpTrans.y(), interpTrans.z(), interpRotation.ToFloat(), interpHeadAngle, interpLiftAngle, prev_it->second.GetPose().GetParent());

          const bool isCloserToItThanFirst = Util::IsFltGE(timeScale, 0.5f);
          const bool interpIsCarryingObject = isCloserToItThanFirst ? it->second.IsCarryingObject() : prev_it->second.IsCarryingObject();

          Pose3d interpPose(interpRotation, Z_AXIS_3D(), interpTrans, prev_it->second.GetPose().GetParent());
          p.SetAll(prev_it->second.GetFrameId(), interpPose, interpHeadAngle, interpLiftAngle, interpIsCarryingObject);
        } else {
          
          // Return the pose closest to the requested time
          if (it->first - t_request < t_request - prev_it->first) {
            t = it->first;
            p = it->second;
          } else {
            t = prev_it->first;
            p = prev_it->second;
          }
        }
      }
      
      return RESULT_OK;
    }

    Result RobotPoseHistory::GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p)
    {
      PoseMapIter_t it = visPoses_.find(t_request);
      if (it != visPoses_.end()) {
        *p = &(it->second);
        return RESULT_OK;
      }
      return RESULT_FAIL;
    }
    
    Result RobotPoseHistory::ComputePoseAt(const TimeStamp_t t_request,
                                           TimeStamp_t& t, RobotPoseStamp& p,
                                           bool withInterpolation) const
    {
      // If the vision-based version of the pose exists, return it.
      const_PoseMapIter_t it = visPoses_.find(t_request);
      if (it != visPoses_.end()) {
        t = t_request;
        p = it->second;
        return RESULT_OK;
      }
      
      // Get the raw pose at the requested timestamp
      RobotPoseStamp p1;
      const Result getRawResult = GetRawPoseAt(t_request, t, p1, withInterpolation);
      if (RESULT_OK != getRawResult) {
        return getRawResult;
      }
      
      // Now get the previous vision-based pose
      const_PoseMapIter_t git = visPoses_.lower_bound(t);
      
      // If there are no vision-based poses then return the raw pose that we just got
      if (git == visPoses_.end()) {
        if (visPoses_.empty()) {
          p = p1;
          return RESULT_OK;
        } else {
          --git;
        }
      } else if (git->first != t) {
        // As long as the vision-based pose is not from time t,
        // decrement the pointer to get the previous vision-based
        --git;
      }
      
      // Check frame ID
      // If the vision pose frame id <= requested frame id
      // then just return the raw pose of the requested frame id since it
      // is already based on the vision-based pose.
      if (git->second.GetFrameId() <= p1.GetFrameId()) {
        //printf("FRAME %d <= %d\n", git->second.GetFrameId(), p1.GetFrameId());
        p = p1;
        return RESULT_OK;
      }
      
      #if(DEBUG_ROBOT_POSE_HISTORY)
      static bool printDbg = false;
      if(printDbg) {
        printf("gt: %d\n", git->first);
        git->second.GetPose().Print();
      }
      #endif
      
      // git now points to the latest vision-based pose that exists before time t.
      // Now get the pose in poses_ that immediately follows the vision-based pose's time.
      const_PoseMapIter_t p0_it = poses_.lower_bound(git->first);

      #if(DEBUG_ROBOT_POSE_HISTORY)
      if (printDbg) {
        printf("p0_it: t: %d  frame: %d\n", p0_it->first, p0_it->second.GetFrameId());
        p0_it->second.GetPose().Print();
      
        printf("p1: t: %d  frame: %d\n", t, p1.GetFrameId());
        p1.GetPose().Print();
      }
      #endif
     
      // Compute the total transformation taking us from the vision-only pose at time
      // corresponding to p0, forward to p1. We will be applying this transformation
      // to whatever is stored in the vision-only pose below.
      Pose3d pTransform;
      if(p0_it->second.GetFrameId() == p1.GetFrameId())
      {
        // Special case: no intermediate frames to chain through. The total transformation
        // is just going from p0 to p1.
        Pose3d p1_wrt_p0_parent;
        const bool inSameOrigin = p1.GetPose().GetWithRespectTo(*p0_it->second.GetPose().GetParent(), pTransform);
        ASSERT_NAMED(inSameOrigin, "RobotPoseHistory.ComputePoseAt.FailedGetWRT1");
        pTransform *= p0_it->second.GetPose().GetInverse();
      }
      else
      {
        const_PoseMapIter_t pMid0 = p0_it;
        const_PoseMapIter_t pMid1 = p0_it;
        for (pMid1 = p0_it; pMid1 != poses_.end(); ++pMid1)
        {
          // Bump pMid1 forward until it hits the next frame ID
          if (pMid1->second.GetFrameId() > pMid0->second.GetFrameId())
          {
            // pMid1 is now pointing to the first pose in the next frame after pMid0.
            // Point pMid1 to the last pose of the same frame as pMid0. Compute
            // the transform for this frame (from pMid0 to pMid1) and
            // fold it into the running total stored in pTransform.
            
            // We expect the beginning (pMid0) and end (pMid1) of this part of history
            // to have the same frame ID and origin.
            --pMid1; // (temporarily) move back to last pose in same frame as pMid0
            ASSERT_NAMED(pMid0->second.GetFrameId() == pMid1->second.GetFrameId(),
                         "RobotPoseHistory.ComputePoseAt.MismatchedIntermediateFrameIDs");
            ASSERT_NAMED(&pMid0->second.GetPose().FindOrigin() == &pMid1->second.GetPose().FindOrigin(),
                         "RobotPoseHistory.ComputePoseAt.MismatchedIntermediateOrigins");

            // Get pMid0 and pMid1 w.r.t. the same parent and store in the intermediate
            // transformation pMidTransform, which is going to hold the transformation
            // from pMid0 to pMid1
            Pose3d pMidTransform;
            const bool inSameOrigin = pMid1->second.GetPose().GetWithRespectTo(*pMid0->second.GetPose().GetParent(), pMidTransform);
            ASSERT_NAMED(inSameOrigin, "RobotPoseHistory.ComputePoseAt.FailedGetWRT1");
            
            // pMidTransform = pMid1 * pMid0^(-1)
            pMidTransform *= pMid0->second.GetPose().GetInverse();
            
            // Fold the transformation from pMid0 to pMid1 into the total transformation thus far
            //  pTranform = pMidTransofrm * pTransform
            pTransform.PreComposeWith(pMidTransform);
            
            // Move both pointers to start of next pose frame to begin process again
            ++pMid1;
            pMid0 = pMid1;
          }
       
          if (pMid1->second.GetFrameId() == p1.GetFrameId())
          {
            // Reached p1, so we're done
            break;
          }
        }
      }

      #if(DEBUG_ROBOT_POSE_HISTORY)
      if (printDbg) {
        printf("pTrans: %d\n", t);
        pTransform.Print();
      }
      #endif
      
      // NOTE: We are about to return p, which is a transformed version of the vision-only
      // pose in "git", so it should still be relative to whatever "git" was relative to.
      pTransform *= git->second.GetPose(); // Apply pTransform to git and store in pTransform
      pTransform.SetParent(git->second.GetPose().GetParent()); // Keep git's parent
      p.SetAll(git->second.GetFrameId(), pTransform, p1.GetHeadAngle(), p1.GetLiftAngle(), p1.IsCarryingObject());
      
      return RESULT_OK;
    }
    
    Result RobotPoseHistory::ComputeAndInsertPoseAt(const TimeStamp_t t_request,
                                                    TimeStamp_t& t, RobotPoseStamp** p,
                                                    HistPoseKey* key,
                                                    bool withInterpolation)
    {
      RobotPoseStamp ps;
      //printf("COMPUTE+INSERT\n");
      const Result computeResult = ComputePoseAt(t_request, t, ps, withInterpolation);
      if (RESULT_OK != computeResult) {
        *p = nullptr;
        return computeResult;
      }
      
      // If computedPose entry exist at t, then overwrite it
      PoseMapIter_t it = computedPoses_.find(t);
      if (it != computedPoses_.end()) {
        it->second.SetAll(ps.GetFrameId(), ps.GetPose(), ps.GetHeadAngle(), ps.GetLiftAngle(), ps.IsCarryingObject());
        *p = &(it->second);
        
        if (key) {
          *key = keyByTsMap_[t];
        }
      } else {
        
        std::pair<PoseMapIter_t, bool> res;
        res = computedPoses_.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(t),
                                     std::forward_as_tuple(ps.GetFrameId(), ps.GetPose(), ps.GetHeadAngle(), ps.GetLiftAngle(), ps.IsCarryingObject()));
        
        if (!res.second) {
          return RESULT_FAIL;
        }
        
        *p = &(res.first->second);
        
        
        // Create key associated with computed pose
        ++currHistPoseKey_;
        tsByKeyMap_.emplace(std::piecewise_construct,
                            std::forward_as_tuple(currHistPoseKey_),
                            std::forward_as_tuple(t));
        keyByTsMap_.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(t),
                                  std::forward_as_tuple(currHistPoseKey_));
        
        if (key) {
          *key = currHistPoseKey_;
        }

      }
      
      return RESULT_OK;
    }

    Result RobotPoseHistory::GetComputedPoseAt(const TimeStamp_t t_request,
                                               const RobotPoseStamp ** p,
                                               HistPoseKey* key) const
    {
      const_PoseMapIter_t it = computedPoses_.find(t_request);
      if (it != computedPoses_.end()) {
        *p = &(it->second);
        
        // Get key for the computed pose
        if (key){
          const_KeyByTimestampMapIter_t kIt = keyByTsMap_.find(it->first);
          if (kIt == keyByTsMap_.end()) {
            PRINT_NAMED_WARNING("RobotPoseHistory.GetComputedPoseAt.KeyNotFound","");
            return RESULT_FAIL;
          }
          *key = kIt->second;
        }
        
        return RESULT_OK;
      }
      
      // TODO: Compute the pose if it doesn't exist already?
      // ...
      
      return RESULT_FAIL;
    }


    // TODO:(bn) is there a way to avoid this duplicated code here? Not eager to use templates...
    Result RobotPoseHistory::GetComputedPoseAt(const TimeStamp_t t_request,
                                               RobotPoseStamp ** p,
                                               HistPoseKey* key)
    {
      return GetComputedPoseAt(t_request, const_cast<const RobotPoseStamp **>(p), key);
    }
    
    Result RobotPoseHistory::GetLatestVisionOnlyPose(TimeStamp_t& t, RobotPoseStamp& p) const
    {
      if (!visPoses_.empty()) {
        t = visPoses_.rbegin()->first;
        p = visPoses_.rbegin()->second;
        return RESULT_OK;
      }
      
      return RESULT_FAIL;
    }
    
    Result RobotPoseHistory::GetLastPoseWithFrameID(const PoseFrameID_t frameID, RobotPoseStamp& p) const
    {
      // Start from end and work backward until we find a pose stamp with the
      // specified ID. Fail if we get back to the beginning without finding it.
      if(poses_.empty()) {
        PRINT_NAMED_ERROR("RobotPoseHistory.GetLastPoseWithFrameID.EmptyHistory",
                          "Looking for last pose with frame ID=%d, but pose history is empty.", frameID);
        return RESULT_FAIL;
      }
      
      // First look through "raw" poses for the frame ID. We don't need to look
      // any further once the frameID drops below the one we are looking for,
      // because they are ordered
      bool found = false;
      auto poseIter = poses_.rend();
      for(poseIter = poses_.rbegin();
          poseIter != poses_.rend() && poseIter->second.GetFrameId() >= frameID; ++poseIter )
      {
        if(poseIter->second.GetFrameId() == frameID) {
          found = true; // break out of the loop without incrementing poseIter
          break;
        }
      }
      
      // NOTE: this second loop over vision poses will only occur if found is still false,
      // meaning we didn't find a pose already in the first loop.
      if(!found) {
        for(poseIter = visPoses_.rbegin();
            poseIter != visPoses_.rend() && poseIter->second.GetFrameId() >= frameID; ++poseIter)
        {
          if(poseIter->second.GetFrameId() == frameID) {
            found = true;
            break;
          }
        }
      }
      
      if(found) {
        // Success!
          assert(poseIter != poses_.rend());
        p = poseIter->second;
        return RESULT_OK;
        
      } else {
        PRINT_NAMED_ERROR("RobotPoseHistory.GetLastPoseWithFrameID.FrameIdNotFound",
                          "Could not find frame ID=%d in pose history. "
                          "(First frameID in pose history is %d, last is %d. "
                          "First frameID in vis pose history is %d, last is %d.", frameID,
                          poses_.begin()->second.GetFrameId(),
                          poses_.rbegin()->second.GetFrameId(),
                          visPoses_.begin()->second.GetFrameId(),
                          visPoses_.rbegin()->second.GetFrameId());
        return RESULT_FAIL;

      }
      
    } // GetLastPoseWithFrameID()
    
    void RobotPoseHistory::CullToWindowSize()
    {
      if (poses_.size() > 1) {
        
        // Get the most recent timestamp
        TimeStamp_t mostRecentTime = poses_.rbegin()->first;
        
        // If most recent time is less than window size, we're done.
        if (mostRecentTime < windowSize_) {
          return;
        }
        
        // Get pointer to the oldest timestamp that may remain in the map
        TimeStamp_t oldestAllowedTime = mostRecentTime - windowSize_;
        const_PoseMapIter_t it = poses_.upper_bound(oldestAllowedTime);
        const_PoseMapIter_t git = visPoses_.upper_bound(oldestAllowedTime);
        const_PoseMapIter_t cit = computedPoses_.upper_bound(oldestAllowedTime);
        
        // Delete everything before the oldest allowed timestamp
        if (oldestAllowedTime > poses_.begin()->first) {
          poses_.erase(poses_.begin(), it);
        }
        if (oldestAllowedTime > visPoses_.begin()->first) {
          visPoses_.erase(visPoses_.begin(), git);
        }
        if (oldestAllowedTime > computedPoses_.begin()->first) {

          // For all computedPoses up until cit, remove their associated keys.
          for(PoseMapIter_t delIt = computedPoses_.begin(); delIt != cit; ++delIt) {
            KeyByTimestampMapIter_t kbtIt = keyByTsMap_.find(delIt->first);
            if (kbtIt == keyByTsMap_.end()) {
              PRINT_NAMED_WARNING("RobotPoseHistory.CullToWindowSize.KeyNotFound", "");
              break;
            }
            tsByKeyMap_.erase(kbtIt->second);
            keyByTsMap_.erase(kbtIt);
          }

          computedPoses_.erase(computedPoses_.begin(), cit);
        }
      }
    }
    
    bool RobotPoseHistory::IsValidPoseKey(const HistPoseKey key) const
    {
      return (tsByKeyMap_.find(key) != tsByKeyMap_.end());
    }
    
    TimeStamp_t RobotPoseHistory::GetOldestTimeStamp() const
    {
      return (poses_.empty() ? 0 : poses_.begin()->first);
    }
    
    TimeStamp_t RobotPoseHistory::GetNewestTimeStamp() const
    {
      return (poses_.empty() ? 0 : poses_.rbegin()->first);
    }

    TimeStamp_t RobotPoseHistory::GetOldestVisionOnlyTimeStamp() const
    {
      return (visPoses_.empty() ? 0 : visPoses_.begin()->first);
    }

    TimeStamp_t RobotPoseHistory::GetNewestVisionOnlyTimeStamp() const
    {
      return (visPoses_.empty() ? 0 : visPoses_.rbegin()->first);
    }

    void RobotPoseHistory::Print() const
    {
      // Create merged map of all poses
      std::multimap<TimeStamp_t, std::pair<std::string, const_PoseMapIter_t> > mergedPoses;
      std::multimap<TimeStamp_t, std::pair<std::string, const_PoseMapIter_t> >::iterator mergedIt;
      const_PoseMapIter_t pit;
      
      for(pit = poses_.begin(); pit != poses_.end(); ++pit) {
        mergedPoses.emplace(std::piecewise_construct,
                            std::forward_as_tuple(pit->first),
                            std::forward_as_tuple("  ", pit));
      }

      for(pit = visPoses_.begin(); pit != visPoses_.end(); ++pit) {
        mergedPoses.emplace(std::piecewise_construct,
                            std::forward_as_tuple(pit->first),
                            std::forward_as_tuple("v ", pit));
      }

      for(pit = computedPoses_.begin(); pit != computedPoses_.end(); ++pit) {
        mergedPoses.emplace(std::piecewise_construct,
                            std::forward_as_tuple(pit->first),
                            std::forward_as_tuple("c ", pit));
      }
      
      
      printf("\nRobotPoseHistory\n");
      printf("================\n");
      for(mergedIt = mergedPoses.begin(); mergedIt != mergedPoses.end(); ++mergedIt) {
        printf("%s%d: ", mergedIt->second.first.c_str(), mergedIt->first);
        mergedIt->second.second->second.Print();
      }
    }
    
  } // namespace Cozmo
} // namespace Anki
