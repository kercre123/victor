#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/observableObject.h"


namespace Anki {
  namespace Vision {
    
#pragma mark --- ObservableObject Implementations ---
    
    //ObjectID_t ObservableObject::ObjectCounter = 0;
    const std::vector<const KnownMarker*> ObservableObject::sEmptyMarkerVector(0);
    
    ObservableObject::ObservableObject(ObjectType_t objType)
    : type_(objType), ID_(0), wasObserved_(false)
    {
      //ID_ = ObservableObject::ObjectCounter++;
    }
    
    bool ObservableObject::GetWhetherObserved() const
    {
      return wasObserved_;
    }
    
    void ObservableObject::SetWhetherObserved(const bool wasObserved)
    {
      wasObserved_ = wasObserved;
    }
    Vision::KnownMarker const& ObservableObject::AddMarker(const Marker::Code&  withCode,
                                                           const Pose3d&        atPose,
                                                           const f32            size_mm)
    {
      // Copy the pose and set this object's pose as its parent
      Pose3d poseCopy(atPose);
      poseCopy.set_parent(&pose_);
      
      // Construct a marker at that pose and store it keyed by its code
      markers_.emplace_back(withCode, poseCopy, size_mm);
      markersWithCode_[withCode].push_back(&markers_.back());
      
      return markers_.back();
    } // ObservableObject::AddMarker()
    
    std::vector<const KnownMarker*> const& ObservableObject::GetMarkersWithCode(const Marker::Code& withCode) const
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
      
      if(isSame) {
        if(this->GetRotationAmbiguities().empty()) {
          isSame = this->pose_.IsSameAs(otherObject.GetPose(),
                                        distThreshold, angleThreshold, P_diff);
        }
        else {
          isSame = this->pose_.IsSameAs_WithAmbiguity(otherObject.GetPose(),
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
    
    
    
#pragma mark --- ObservableObjectLibrary Implementations ---
    
    const std::vector<const ObservableObject*> ObservableObjectLibrary::sEmptyObjectVector(0);
    
    const ObservableObject* ObservableObjectLibrary::GetObjectWithType(const ObjectType_t type) const
    {
      auto obj = knownObjects_.find(type);
      if(obj != knownObjects_.end()) {
        return obj->second;
      }
      else {
        return 0;
      }
    }
    
    
    std::vector<const ObservableObject*> const& ObservableObjectLibrary::GetObjectsWithCode(const Marker::Code& code) const
    {
      auto temp = objectsWithCode_.find(code);
      if(temp != objectsWithCode_.end()) {
        return temp->second;
      }
      else {
        return ObservableObjectLibrary::sEmptyObjectVector;
      }
    }
    
    std::vector<const ObservableObject*> const& ObservableObjectLibrary::GetObjectsWithMarker(const Marker& marker) const
    {
      return GetObjectsWithCode(marker.GetCode());
    }
    
    void ObservableObjectLibrary::AddObject(const ObservableObject* object)
    {
      // TODO: Warn/error if we are overwriting an existing object with this type?
      knownObjects_[object->GetType()] = object;
      for(auto marker : object->GetMarkers()) {
        objectsWithCode_[marker.GetCode()].push_back(object);
      }
    }
    
    
    // Input:   list of observed markers
    // Outputs: the objects seen and the unused markers
    void ObservableObjectLibrary::CreateObjectsFromMarkers(std::list<ObservedMarker>& markers,
                                                           std::vector<ObservableObject*>& objectsSeen,
                                                           const Camera* seenOnlyBy) const
    {
      // Group the markers by object type
      std::map<ObjectType_t, std::vector<const ObservedMarker*>> markersWithObjectType;
      std::list<ObservedMarker> markersUnused;
      
      for(auto & marker : markers) {
        
        bool used = false;
        
        // If seenOnlyBy was specified, make sure this marker was seen by that
        // camera
        if(seenOnlyBy == NULL || &marker.GetSeenBy() == seenOnlyBy)
        {
          // Find all objects which use this marker...
          std::vector<const ObservableObject*> const& objectsWithMarker = GetObjectsWithMarker(marker);
          
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
            markersWithObjectType[objectsWithMarker.front()->GetType()].push_back(&marker);
            used = true;
          } // IF objectsWithMarker != NULL
        } // IF seenOnlyBy
        
        if(not used) {
          // Keep track of which markers went unused
          markersUnused.push_back(marker);
        }
        
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
            poseMatch.first = poseMatch.first.getWithRespectTo(Pose3d::World);
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
          objectsSeen.back()->SetPose(poseCluster.GetPose().getWithRespectTo(Pose3d::World));
          
        } // FOR each pose cluster
        
        /*
        if(seenOnlyBy != NULL) {
          // If there were (or could have been) multiple observers, we will put
          // all poses in a common World coordinate frame *after* pose clustering,
          // since that process takes the markers' observers into account.
          for(auto & poseMatch : possiblePoses) {
            poseMatch.first = poseMatch.first.getWithRespectTo(Pose3d::World);
          }
        }
         */
        
      } // FOR each objectType
      
      // Return the unused markers
      std::swap(markers, markersUnused);
      
    } // CreateObjectsFromMarkers()
    
    
    ObservableObjectLibrary::PoseCluster::PoseCluster(const PoseMatchPair& match)
    : pose_(match.first)
    {
      matches_.emplace_back(match.second.first, *match.second.second);
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
      
      if(wasAdded) {
        // Create a match between the original observed marker pointer
        // and a copy of the original KnownMarker.
        matches_.emplace_back(match.second.first, *match.second.second);
        
        // If there were rotation ambiguities to look for, modify
        // the original (canonical) KnownMarker's pose based on P_diff
        // so that all cluster members' KnownMarkers will be in the same
        // coordinate frame and we can use them later to recompute this
        // cluster's pose based on all its member matches.
        if(not R_ambiguities.empty()) {
          Pose3d newPose(matches_.back().second.GetPose());
          newPose.preComposeWith(P_diff);
          
          // This should preserve the pose tree (i.e. parent relationship
          // of the KnownMarker to its parent object's pose)
          //CORETECH_ASSERT(newPose.get_parent() == &libObject->GetPose());
          
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
          
          if(&obsMarker->GetSeenBy() == camera)
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
        const Pose3d* originalParent = pose_.get_parent();
        pose_ = camera->computeObjectPose(imgPoints, objPoints);
        if(pose_.get_parent() != originalParent) {
          pose_ = pose_.getWithRespectTo(originalParent);
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