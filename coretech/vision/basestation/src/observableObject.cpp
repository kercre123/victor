/**
 * File: observableObject.cpp
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Implements an abstract ObservableObject class, which is an
 *              general 3D object, with type, ID, and pose, and a set of Markers
 *              on it at known locations.  Thus, it is "observable" by virtue of
 *              having these markers, and its 3D / 6DoF pose can be estimated by
 *              matching up ObservedMarkers with the KnownMarkers it possesses.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/


#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/observableObject.h"

#include "anki/common/basestation/exceptions.h"

#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

namespace Anki {
  namespace Vision {
    
#pragma mark --- ObservableObject Implementations ---
    
    //ObjectID ObservableObject::ObjectCounter = 0;
    const std::vector<KnownMarker*> ObservableObject::sEmptyMarkerVector(0);
    
    const std::vector<RotationMatrix3d> ObservableObject::sRotationAmbiguities; // default is empty
    
    ObservableObject::ObservableObject(ObjectType objType)
    : type_(objType), lastObservedTime_(0), wasObserved_(false)
    {
      //ID_ = ObservableObject::ObjectCounter++;
    }
    
    bool ObservableObject::IsVisibleFrom(const Camera &camera,
                                         const f32 maxFaceNormalAngle,
                                         const f32 minMarkerImageSize,
                                         const bool requireSomethingBehind) const
    {
      // Return true if any of this object's markers are visible from the
      // given camera
      for(auto & marker : markers_) {
        if(marker.IsVisibleFrom(camera, maxFaceNormalAngle, minMarkerImageSize, requireSomethingBehind)) {
          return true;
        }
      }
      
      return false;
      
    } // ObservableObject::IsObservableBy()
    
    
    Vision::KnownMarker const& ObservableObject::AddMarker(const Marker::Code&  withCode,
                                                           const Pose3d&        atPose,
                                                           const f32            size_mm)
    {
      // Copy the pose and set this object's pose as its parent
      Pose3d poseCopy(atPose);
      poseCopy.SetParent(&pose_);
      
      // Construct a marker at that pose and store it keyed by its code
      markers_.emplace_back(withCode, poseCopy, size_mm);
      markersWithCode_[withCode].push_back(&markers_.back());
      
      return markers_.back();
    } // ObservableObject::AddMarker()
    
    std::vector<KnownMarker*> const& ObservableObject::GetMarkersWithCode(const Marker::Code& withCode) const
    {
      auto returnVec = markersWithCode_.find(withCode);
      if(returnVec != markersWithCode_.end()) {
        return returnVec->second;
      }
      else {
        return ObservableObject::sEmptyMarkerVector;
      }
    } // ObservableObject::GetMarkersWithCode()
    
    bool ObservableObject::IsSameAs(const ObservableObject&  otherObject,
                                    const float              distThreshold,
                                    const Radians            angleThreshold,
                                    Pose3d&                  P_diff) const
    {
      // The two objects can't be the same if they aren't the same type!
      bool isSame = this->type_ == otherObject.type_;
      
      Pose3d otherPose;
      if(&otherObject.GetPose() == this->pose_.GetParent()) {
        otherPose.SetParent(&otherObject.GetPose());
      } else if(otherObject.GetPose().GetWithRespectTo(*this->pose_.GetParent(), otherPose) == false) {
        PRINT_NAMED_WARNING("ObservableObject.IsSameAs.ObjectsHaveDifferentOrigins",
                            "Could not get other object w.r.t. this object's parent. Returning isSame == false.\n");
        isSame = false;
      }
      
      if(isSame) {
        
        CORETECH_ASSERT(otherPose.GetParent() == pose_.GetParent());
        
        if(this->GetRotationAmbiguities().empty()) {
          isSame = this->pose_.IsSameAs(otherPose, distThreshold, angleThreshold, P_diff);
        }
        else {
          isSame = this->pose_.IsSameAs_WithAmbiguity(otherPose,
                                                      this->GetRotationAmbiguities(),
                                                      distThreshold, angleThreshold,
                                                      true, P_diff);
        } // if/else there are ambiguities
      }
      
      return isSame;
      
    } // ObservableObject::IsSameAs()

    void ObservableObject::ComputePossiblePoses(const ObservedMarker*     obsMarker,
                                                std::vector<PoseMatchPair>& possiblePoses) const
    {
      auto matchingMarkers = markersWithCode_.find(obsMarker->GetCode());
      
      if(matchingMarkers != markersWithCode_.end()) {
        
        for(auto matchingMarker : matchingMarkers->second) {
          // Note that the known marker's pose, and thus its 3d corners, are
          // defined in the coordinate frame of parent object, so computing its
          // pose here _is_ the pose of the parent object.
          Pose3d markerPoseWrtCamera( matchingMarker->EstimateObservedPose(*obsMarker) );
          
          // Store the pose in the camera's coordinate frame, along with the pair
          // of markers that generated it
          possiblePoses.emplace_back(markerPoseWrtCamera,
                                     MarkerMatch(obsMarker, matchingMarker));
        }
      }
      
    } // ComputePossiblePoses()

    
    void ObservableObject::GetCorners(std::vector<Point3f>& corners) const
    {
      this->GetCorners(pose_, corners);
    }
    
    
    void ObservableObject::GetObservedMarkers(std::vector<const KnownMarker*>& observedMarkers,
                                              const TimeStamp_t sinceTime) const
    {
      if(sinceTime > 0) {
        for(auto const& marker : this->markers_)
        {
          if(marker.GetLastObservedTime() >= sinceTime) {
            observedMarkers.push_back(&marker);
          }
        }
      }
    } // GetObservedMarkers()
    
    
    Result ObservableObject::UpdateMarkerObservationTimes(const ObservableObject& otherObject)
    {
      if(otherObject.GetType() != this->GetType()) {
        PRINT_NAMED_ERROR("ObservableObject.UpdateMarkerObservationTimes.TypeMismatch",
                          "Tried to update the marker observations between dissimilar object types.\n");
        return RESULT_FAIL;
      }

      std::list<KnownMarker> const& otherMarkers = otherObject.GetMarkers();

      // If these objects are the same type they have to have the same number of
      // markers, by definition.
      CORETECH_ASSERT(otherMarkers.size() == markers_.size());
      
      std::list<KnownMarker>::const_iterator otherMarkerIter = otherMarkers.begin();
      std::list<KnownMarker>::iterator markerIter = markers_.begin();
      
      for(;otherMarkerIter != otherMarkers.end() && markerIter != markers_.end();
          ++otherMarkerIter, ++markerIter)
      {
        markerIter->SetLastObservedTime(std::max(markerIter->GetLastObservedTime(),
                                                 otherMarkerIter->GetLastObservedTime()));
      }
      
      return RESULT_OK;
    }
    
    
    void ObservableObject::SetMarkersAsObserved(const Marker::Code& withCode,
                                                const TimeStamp_t   atTime)
    {
      auto markers = markersWithCode_.find(withCode);
      if(markers != markersWithCode_.end()) {
        for(auto marker : markers->second) {
          //marker->SetWasObserved(true);
          marker->SetLastObservedTime(atTime);
        }
      }
      else {
        // TODO: Issue warning?
      }
      
    } // SetMarkersAsObserved()
    
    
    Quad2f ObservableObject::GetBoundingQuadXY(const f32 padding_mm) const
    {
      return GetBoundingQuadXY(pose_, padding_mm);
    }
    
#pragma mark --- ObservableObjectLibrary Implementations ---
    
    const std::set<const ObservableObject*> ObservableObjectLibrary::sEmptyObjectVector;
    
    const ObservableObject* ObservableObjectLibrary::GetObjectWithType(const ObjectType type) const
    {
      auto obj = knownObjects_.find(type);
      if(obj != knownObjects_.end()) {
        return obj->second;
      }
      else {
        return 0;
      }
    }
    
    
    std::set<const ObservableObject*> const& ObservableObjectLibrary::GetObjectsWithCode(const Marker::Code& code) const
    {
      auto temp = objectsWithCode_.find(code);
      if(temp != objectsWithCode_.end()) {
        return temp->second;
      }
      else {
        return ObservableObjectLibrary::sEmptyObjectVector;
      }
    }
    
    std::set<const ObservableObject*> const& ObservableObjectLibrary::GetObjectsWithMarker(const Marker& marker) const
    {
      return GetObjectsWithCode(marker.GetCode());
    }
    
    void ObservableObjectLibrary::AddObject(const ObservableObject* object)
    {
      // TODO: Warn/error if we are overwriting an existing object with this type?
      knownObjects_[object->GetType()] = object;
      for(auto marker : object->GetMarkers()) {
        objectsWithCode_[marker.GetCode()].insert(object);
      }
    }
    
    
    // Input:   list of observed markers
    // Outputs: the objects seen and the unused markers
    void ObservableObjectLibrary::CreateObjectsFromMarkers(const std::list<ObservedMarker*>& markers,
                                                           std::vector<ObservableObject*>& objectsSeen,
                                                           const CameraID_t seenOnlyBy) const
    {
      // Group the markers by object type
      std::map<ObjectType, std::vector<const ObservedMarker*>> markersWithObjectType;
      
      for(auto marker : markers) {
        
        marker->MarkUsed(false);
        
        // If seenOnlyBy was specified, make sure this marker was seen by that
        // camera
        if(seenOnlyBy == ANY_CAMERA || marker->GetSeenBy().GetID() == seenOnlyBy)
        {
          // Find all objects which use this marker...
          std::set<const ObservableObject*> const& objectsWithMarker = GetObjectsWithMarker(*marker);
          
          // ...if there are any, add this marker to the list of observed markers
          // that corresponds to this object type.
          if(!objectsWithMarker.empty()) {
            if(objectsWithMarker.size() > 1) {
              CORETECH_THROW("Having multiple objects in the library with the "
                             "same marker is not yet supported.");
              
              /*
               for(auto object : *objectsWithMarker) {
               markersWithObjectID[object->GetID()].push_back(&marker);
               }
               */
            }
            markersWithObjectType[(*objectsWithMarker.begin())->GetType()].push_back(marker);
            marker->MarkUsed(true);
          } // IF objectsWithMarker != NULL
        } // IF seenOnlyBy
        
      } // For each marker we saw
      
      // Now go through each object type we saw a marker for, and use the
      // corresponding observed markers to create a list of possible poses
      // for that object. Then cluster the possible poses into one or more
      // object instances of that type, returned in the "objectsSeen" vector.
      for(auto & objTypeMarkersPair : markersWithObjectType)
      {
        // A place to store the possible poses together with the observed/known
        // markers that implied them
        std::vector<PoseMatchPair> possiblePoses;
        
        const Vision::ObservableObject* libObject = GetObjectWithType(objTypeMarkersPair.first);
        
        // Get timestamp of the observed markers so that I can set the
        // lastSeenTime of the observable object.
        auto obsMarker = objTypeMarkersPair.second.begin();
        const TimeStamp_t observedTime = (*obsMarker)->GetTimeStamp();

        // Check that all remaining markers also have the same timestamp (which
        // they now should, since we are processing grouped by timestamp)
        while(++obsMarker != objTypeMarkersPair.second.end()) {
          CORETECH_ASSERT(observedTime == (*obsMarker)->GetTimeStamp());
        }
        
        for(auto obsMarker : objTypeMarkersPair.second)
        {         
          // For each observed marker, we add to the list of possible poses
          // (each paired with the observed/known marker match from which the
          // pose was computed).
          // (This is all so we can recompute object pose later from clusters of
          // markers, without having to reassociate observed and known markers.)
          // reassociate them.)
          // Note that each observed marker can generate multiple poses because
          // an object may have the same known marker on it several times.

          libObject->ComputePossiblePoses(obsMarker, possiblePoses);
          
        } // FOR each observed marker
        
        // Now we have a list of poses, each paired with the matched marker pair
        // that generated it.  We will now cluster those individual poses which
        // are the "same" according to the object's matching function (which will
        // internally take symmetry ambiguities into account during matching
        // and adjust the known markers' poses accordingly)
        /*
        if(seenOnlyBy == NULL) {
          // First put them all in a common World coordinate frame if multiple
          // observers are possible.  Otherwise they'll be clustered in the
          // coordinate frame of the (single) observer.
          for(auto & poseMatch : possiblePoses) {
            poseMatch.first = poseMatch.first.GetWithRespectTo(Pose3d::World);
          }
        }
         */
        
        // TODO: make the distance/angle thresholds parameters or else object-type-specific
        std::vector<PoseCluster> poseClusters;
        ClusterObjectPoses(possiblePoses, libObject,
                           5.f, 5.f*M_PI/180.f, poseClusters);
        
        // Recompute the pose for any cluster which had multiple matches in it,
        // using all of its matches simultaneously, and then add it as a new
        // object seen
        for(auto & poseCluster : poseClusters) {
          CORETECH_ASSERT(poseCluster.GetSize() > 0);
          
          // NOTE: this does nothing for singleton clusters
          poseCluster.RecomputePose();
          
          // Add to list of objects seen -- using pose in World frame
          objectsSeen.push_back(libObject->Clone());
          
          // Compute pose wrt camera, or world if no camera specified
          //if (seenOnlyBy == ANY_CAMERA) {
            Pose3d newPose = poseCluster.GetPose().GetWithRespectToOrigin();
          
            //if(poseCluster.GetPose().GetWithRespectTo(poseCluster.GetPose().FindOrigin(), newPose) == true) {
              objectsSeen.back()->SetPose(newPose);
            //} else {
            //  PRINT_NAMED_ERROR("ObservableObjectLibrary.CreateObjectsFromMarkers.CouldNotFindWorldOrigin",
            //                    "Could not get the pose cluster w.r.t. the world pose.\n");
              // TODO: this may indicate that we should be getting the pose w.r.t. the origin
              // of the camera that saw this object.
              // We are probably not handling the case that two robot's saw the same thing
              // but are not both localized to the same world frame yet.
            //}
          //} else {
          //  objectsSeen.back()->SetPose(poseCluster.GetPose());
          //}
          
          // Set the markers in the object corresponding to those from the pose
          // cluster from which it was computed as "observed"
          for(auto & match : poseCluster.GetMatches()) {
            const KnownMarker& marker = match.second;
            objectsSeen.back()->SetMarkersAsObserved(marker.GetCode(),
                                                     observedTime);
          }
          
          objectsSeen.back()->SetLastObservedTime(observedTime);
          
        } // FOR each pose cluster
        
        /*
        if(seenOnlyBy != NULL) {
          // If there were (or could have been) multiple observers, we will put
          // all poses in a common World coordinate frame *after* pose clustering,
          // since that process takes the markers' observers into account.
          for(auto & poseMatch : possiblePoses) {
            poseMatch.first = poseMatch.first.GetWithRespectTo(Pose3d::World);
          }
        }
         */
        
      } // FOR each objectType
      
    } // CreateObjectsFromMarkers()
    
    
    ObservableObjectLibrary::PoseCluster::PoseCluster(const PoseMatchPair& match)
    : pose_(match.first)
    {
      const MarkerMatch& markerMatch = match.second;
      const ObservedMarker* obsMarker = markerMatch.first;
      const KnownMarker* knownMarker  = markerMatch.second;
      matches_.emplace_back(obsMarker, *knownMarker);
      
      // Mark the new KnownMarker object in this match as observed
      //matches_.back().second.SetWasObserved(true);
      
      // Keep a unique set of observed markers as part of the cluster so that
      // we don't add multiple (ambiguous) poses to one cluster that
      // were generated by the same observed marker.
      obsMarkerSet_.insert(obsMarker);
      
    }
    
    bool ObservableObjectLibrary::PoseCluster::TryToAddMatch(const PoseMatchPair& match,
                                                             const float distThreshold,
                                                             const Radians angleThreshold,
                                                             const std::vector<RotationMatrix3d>& R_ambiguities)
    {
      bool wasAdded = false;
      const Pose3d& P_other = match.first;
      Pose3d P_diff;
      if(R_ambiguities.empty()) {
        wasAdded = pose_.IsSameAs(P_other, distThreshold, angleThreshold, P_diff);
      }
      else {
        wasAdded = pose_.IsSameAs_WithAmbiguity(P_other, R_ambiguities,
                                                distThreshold, angleThreshold, true, P_diff);
      } // if/else the ambiguities list is empty
      
      // Note that we check to see if a pose stemming from this match's
      // observed marker is already present in this cluster so that
      // we don't add multiple (ambiguous) poses to one cluster that
      // were generated by the same observed marker.  We will still return
      // this match as "added" though since it matched this cluster's pose.
      // That way it will get marked as "assigned" and not used in another
      // cluster.  We just don't want to use a bunch of clusters that all
      // actually came from the same observed marker in a "RecomputePose"
      // call later.
      if(wasAdded && obsMarkerSet_.count(match.second.first)==0) {
        
        obsMarkerSet_.insert(match.second.first);
        
        // Create a match between the original observed marker pointer
        // and a copy of the original KnownMarker.
        matches_.emplace_back(match.second.first, *match.second.second);
        
        // Mark the new KnownMarker object in this match as observed
        //matches_.back().second.SetWasObserved(true);
        
        // If there were rotation ambiguities to look for, modify
        // the original (canonical) KnownMarker's pose based on P_diff
        // so that all cluster members' KnownMarkers will be in the same
        // coordinate frame and we can use them later to recompute this
        // cluster's pose based on all its member matches.
        if(not R_ambiguities.empty()) {
          Pose3d newPose(matches_.back().second.GetPose());
          newPose.PreComposeWith(P_diff);
          
          // This should preserve the pose tree (i.e. parent relationship
          // of the KnownMarker to its parent object's pose)
          //CORETECH_ASSERT(newPose.GetParent() == &libObject->GetPose());
          
          matches_.back().second.SetPose(newPose);
        } // if there were rotation ambiguities
      }
      
      return wasAdded;
    } // TryToAddMatch()
    
    void ObservableObjectLibrary::PoseCluster::RecomputePose()
    {
      if(GetSize() > 1) {
        fprintf(stdout, "Re-computing pose from all %zu members of cluster.\n",
                GetSize());
        
        std::vector<Point2f> imgPoints;
        std::vector<Point3f> objPoints;
        imgPoints.reserve(4*GetSize());
        objPoints.reserve(4*GetSize());
        
        // TODO: Implement ability to estimate object pose with markers seen by different cameras
        const Camera* camera = &matches_.front().first->GetSeenBy();
        
        for(auto & match : matches_)
        {
          const Vision::ObservedMarker* obsMarker = match.first;
          
          if(obsMarker->GetSeenBy().GetID() == camera->GetID())
          {
            const Vision::KnownMarker* libMarker = &match.second;
            
            const Quad2f& imgCorners2d = obsMarker->GetImageCorners();
            const Quad3f& objCorners3d = libMarker->Get3dCorners();
            
            for(Quad::CornerName i_corner=Quad::FirstCorner;
                i_corner < Quad::NumCorners; ++i_corner)
            {
              imgPoints.push_back(imgCorners2d[i_corner]);
              objPoints.push_back(objCorners3d[i_corner]);
            }
          }
          else {
            fprintf(stdout, "Ability to re-estimate single object's "
                    "pose from markers seen by two different cameras "
                    "not yet implemented. Will just use markers from "
                    "first camera in the cluster.\n");
          }
          
        } // for each cluster member
        
        // Compute the object pose from all the corresponding 2d (image)
        // and 3d (object) points
        const Pose3d* originalParent = pose_.GetParent();
        pose_ = camera->ComputeObjectPose(imgPoints, objPoints);
        if(pose_.GetParent() != originalParent) {
          if(pose_.GetWithRespectTo(*originalParent, pose_) == false) {
            PRINT_NAMED_ERROR("ObservableObjectLibrary.PoseCluster.RecomputePose.OriginMisMatch",
                              "Could not get object pose w.r.t. original parent.\n");
          }
        }
      } // IF more than one member
      
    } // RecomputePose()
    
    void ObservableObjectLibrary::ClusterObjectPoses(const std::vector<PoseMatchPair>& possiblePoses,
                                                     const ObservableObject*         libObject,
                                                     const float distThreshold, const Radians angleThreshold,
                                                     std::vector<PoseCluster>& poseClusters) const
    {
      std::vector<bool> assigned(possiblePoses.size(), false);

      const std::vector<RotationMatrix3d>& R_amb = libObject->GetRotationAmbiguities();
      
      for(size_t i_pose=0; i_pose<possiblePoses.size(); ++i_pose)
      {
        if(not assigned[i_pose])
        {
          // Create a new cluster from this pose, and mark it as assigned
          assigned[i_pose] = true;
          
          poseClusters.emplace_back(possiblePoses[i_pose]);
          
          // Now look through all other unassigned pose/match pairs to see if
          // they can be added to this cluster
          for(size_t i_other=(i_pose+1); i_other<possiblePoses.size(); ++i_other)
          {
            if(not assigned[i_other])
            {
              if(poseClusters.back().TryToAddMatch(possiblePoses[i_other],
                                                   distThreshold,
                                                   angleThreshold,
                                                   R_amb))
              {
                assigned[i_other] = true;
              }
                
            } // IF other pose not assigned
          } // FOR each other pose
        } // IF this pose not assigned
      } // FOR each pose
      
    } // ClusterObjectPoses()
   
    
  } // namespace Vision
} // namespace Anki
