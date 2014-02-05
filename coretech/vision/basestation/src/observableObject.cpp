#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/observableObject.h"


namespace Anki {
  namespace Vision {
    
    ObservableObject::ObservableObject(ObjectID_t ID)
    : ID_(ID)
    {
      
    }
    
    void ObservableObject::AddMarker(const Marker::Code&  withCode,
                                     const Pose3d&        atPose,
                                     const f32            size_mm)
    {
      // Copy the pose and set this object's pose as its parent
      Pose3d poseCopy(atPose);
      poseCopy.set_parent(&pose_);
      
      // Construct a marker at that pose and store it keyed by its code
      markers_.emplace_back(withCode, poseCopy, size_mm);
      markersWithCode_[withCode].push_back(&markers_.back());
    }
    
    std::vector<const KnownMarker*> const* ObservableObject::GetMarkersWithCode(const Marker::Code& withCode) const
    {
      auto returnVec = markersWithCode_.find(withCode);
      if(returnVec != markersWithCode_.end()) {
        return &(returnVec->second);
      }
      else {
        return 0;
      }
    }
    
    bool ObservableObject::IsSameAs(const ObservableObject&  otherObject,
                                    const float              distThreshold,
                                    const Radians            angleThreshold,
                                    Pose3d&                  P_diff) const
    {
      bool isSame = this->ID_ == otherObject.ID_;
      
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

    
    
    const ObservableObject* ObservableObjectLibrary::GetObjectWithID(const ObjectID_t ID) const
    {
      auto obj = knownObjects_.find(ID);
      if(obj != knownObjects_.end()) {
        return obj->second;
      }
      else {
        return 0;
      }
    }
    
    
    std::vector<const ObservableObject*> const* ObservableObjectLibrary::GetObjectsWithCode(const Marker::Code& code) const
    {
      auto temp = objectsWithCode_.find(code);
      if(temp != objectsWithCode_.end()) {
        return &temp->second;
      }
      else {
        return NULL;
      }
    }
    
    std::vector<const ObservableObject*> const* ObservableObjectLibrary::GetObjectsWithMarker(const Marker& marker) const
    {
      return GetObjectsWithCode(marker.GetCode());
    }
    
    void ObservableObjectLibrary::AddObject(const ObservableObject* object)
    {
      // TODO: Warn/error if we are overwriting an existing object with this ID?
      knownObjects_[object->GetID()] = object;
      for(auto marker : object->GetMarkers()) {
        objectsWithCode_[marker.GetCode()].push_back(object);
      }
    }
    
    
    // Input:   list of observed markers
    // Outputs: the objects seen and the unused markers
    void ObservableObjectLibrary::CreateObjectsFromMarkers(std::list<ObservedMarker>& markers,
                                                           std::vector<ObservableObject*>& objectsSeen) const
    {
      // Group the markers by object ID
      std::map<ObjectID_t, std::vector<const ObservedMarker*>> markersWithObjectID;
      std::list<ObservedMarker> markersUnused;
      
      for(auto & marker : markers) {
        
        // Find all objects which use this marker...
        std::vector<const ObservableObject*> const* objectsWithMarker = GetObjectsWithMarker(marker);
        
        // ...if there are any, add this marker to the list of observed markers
        // that corresponds to this object ID.
        if(objectsWithMarker != NULL) {
          for(auto object : *objectsWithMarker) {
            markersWithObjectID[object->GetID()].push_back(&marker);
          }
        }
        else {
          // Keep track of which markers went unused
          markersUnused.push_back(marker);
        }
        
      } // For each marker we saw
      
      // Now go through each object ID we saw a marker for, and use the
      // corresponding observed markers to create a list of possible poses
      // for that object. Then cluster the possible poses into one or more
      // object instances of that ID, returned in the "objectsSeen" vector.
      for(auto & objIdMarkersPair : markersWithObjectID)
      {
        // A place to store the possible poses together with the observed/known
        // markers that implied them
        std::vector<PoseMatchPair> possiblePoses;
        
        const Vision::ObservableObject* libObject = GetObjectWithID(objIdMarkersPair.first);
        
        for(auto obsMarker : objIdMarkersPair.second)
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
        // TODO: make the distance/angle thresholds parameters or else object-type-specific
        std::vector<PoseCluster> poseClusters;
        ClusterObjectPoses(possiblePoses, libObject,
                           0.5f*libObject->GetMinDim(), 5.f*M_PI/180.f, poseClusters);
        
        // Recompute the pose for any cluster which had multiple matches in it,
        // using all of its matches simultaneously, and then add it as a new
        // object seen
        for(auto & poseCluster : poseClusters) {
          CORETECH_ASSERT(poseCluster.GetSize() > 0);
          
          // NOTE: this does nothing for singleton clusters
          poseCluster.RecomputePose();
          
          objectsSeen.push_back(libObject->Clone());
          objectsSeen.back()->SetPose(poseCluster.GetPose());
          
        } // FOR each pose cluster
        
      } // FOR each objectID
      
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
        fprintf(stdout, "Re-computing pose from all memers of cluster.\n");
        
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
        pose_ = camera->computeObjectPose(imgPoints, objPoints);
      
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
    
    /*
    void ObservableObjectLibrary::
    ClusterObjectPoses(const std::list<PoseMatchPair>& individualPoses,
                            const f32 distThreshold, const Radians angleThreshold,
                            std::list<PoseMatchListPair>& poseClusters) const
    {
      
      
      std::vector<bool> assigned(poseMatches.size(), false);
      
      for(size_t i_pose=0; i_pose < poseMatches.size(); ++i_pose)
      {
        if(not assigned[i_pose])
        {
          // Try creating cluster starting with each pose suggested by this
          // marker, keeping track of the largest cluster we find.
          const std::vector<MatchWithPose>& seedMatches = poseMatches[i_pose];
          std::vector<std::pair<size_t,size_t> > largestCluster, seedCluster;
          size_t largestSeed = 0;
          
          for(size_t i_seed =0; i_seed < seedMatches.size(); ++i_seed)
          {
            const Pose3d& seedPose = seedMatches[i_seed].second;
            
            seedCluster.clear();
           
            // Try each unassigned other poses' options
            for(size_t i_other=i_pose+1; i_other < poseMatches.size(); ++i_other)
            {
              if(not assigned[i_other])
              {
                // As soon as any one of the "other" poses is found to be
                // close enough to the current seed, put it in the cluster
                // and stop looking
                const std::vector<MatchWithPose>& otherMatches = poseMatches[i_other];
                
                bool assignedToSeed = false;
                for(size_t i_otherOption=0; not assignedToSeed &&
                    i_otherOption < otherMatches.size(); ++i_otherOption)
                {
                  if(seedPose.IsSameAs(otherMatches[i_otherOption].second,
                                       distThreshold, angleThreshold))
                  {
                    seedCluster.emplace_back(i_other, i_otherOption);
                    assignedToSeed = true;
                  }
                }
                
              } // if not assigned
            } // for each other objPose
            
            // If the cluster generated by this seed is larger than the current
            // largest cluster, keep it
            if(seedCluster.size() > largestCluster.size()) {
              std::swap(seedCluster, largestCluster);
              largestSeed = i_seed;
            }
            
          } // FOR each seed
          
          // Keep the largest cluster and mark its members as assigned
          markerClusters.emplace_back(); // add new empty cluster...
          markerClusters.back().emplace_back(poseMatches[i_pose][largestSeed]); // ...put this pose match in it
          
          // Add pose matches from the largest cluster
          for(auto clusterMember : largestCluster)
          {
            const size_t i_other  = clusterMember.first;
            const size_t i_option = clusterMember.second;
            markerClusters.back().emplace_back(poseMatches[i_other][i_option]);
            assigned[i_other] = true;
          }
          
          
          assigned[i_pose] = true;
          
        } // if not assigned
      } // for each objPose
      
      // Sanity check:
      for(bool each_assigned : assigned)
      {
        // All markers should have been assigned to an object
        CORETECH_ASSERT(each_assigned);
      }
      
    } // ClusterObjectPoses()
    */
    
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
          
          // Store the pose in the world coordinate frame, along with the pair
          // of markers that generated it
          possiblePoses.emplace_back(markerPoseWrtCamera.getWithRespectTo(Pose3d::World),
                                     MarkerMatch(obsMarker, matchingMarker));
        }
      }
      
    }
    
    /*
    void ObservableObjectLibrary::ComputeObjectPoses(const ObservedMarker* obsMarker,
                                                     const ObservableObject* libObject,
                                                     std::vector<MatchWithPose>& objPoses) const
    {
      // Lookup the set of markers on this library object which match the given
      // marker
      const std::vector<const KnownMarker*> *libMarkers = libObject->GetMarkersWithCode(obsMarker->GetCode());

      // If there are any matching markers...
      if(libMarkers != NULL) {
        // ...estimate the pose of each library marker if it was seen by the
        // camera that saw the observed marker, and add that to the list of
        // possible object poses for this observed/known marker match.
        for(auto libMarker : *libMarkers)
        {
          // Add this pairing of observed and library markers to the return
          // vector, along with the corresponding computed pose of the _object_,
          // not the marker.
          MarkerMatch match(obsMarker, libMarker);
          Pose3d markerPoseWrtCamera( libMarker->EstimateObservedPose(*obsMarker) );
          objPoses.emplace_back(match, libObject->GetPose().getWithRespectTo(&markerPoseWrtCamera) );
        }
      }
    } // ComputeObjectPoses()
    
    
    void ObservableObjectLibrary::
    GroupPosesIntoObjects(const std::list<MatchedMarkerPosesPair>&  objPoses,
                          const ObservableObject*                   libObject,
                          std::vector<ObservableObject*>&           objectsSeen) const
    {
      // TODO: Make this a parameter somewhere
      const Radians angleDiffThreshold = 0.5f*M_PI_4;
      
      // TODO: Maybe get rid of GetMinDim() usage here?
      const float distThreshold = 0.5f*libObject->GetMinDim();
      
      // Group markers which could be part of the same physical object (i.e. those
      // whose poses are colocated, noting that the pose is relative to
      // the center of the object)
      std::list<PoseCluster> poseClusters;
      ClusterObjectPoses(objPoses, distThreshold,
                         angleDiffThreshold, poseClusters);
      
      fprintf(stdout, "Grouping %lu observed 2D markers of type %d into %lu objects.\n",
              objPoses.size(), libObject->GetID(), poseClusters.size());
      
      
      // For each new object, re-estimate the pose based on all markers that
      // were assigned to it
      for(auto & cluster : markerClusters)
      {
        CORETECH_ASSERT(not cluster.empty());
        
        if(cluster.size() == 1) {
          fprintf(stdout, "Singleton cluster: using already-computed pose.\n");
          
          // No need to re-estimate, just use the one marker we saw.
          objectsSeen.push_back(libObject->clone());
          objectsSeen.back()->SetPose(cluster[0].second);
        }
        else {
          fprintf(stdout, "Re-computing pose from all memers of cluster.\n");
          
          std::vector<Point2f> imgPoints;
          std::vector<Point3f> objPoints;
          imgPoints.reserve(4*cluster.size());
          objPoints.reserve(4*cluster.size());
          
          // TODO: Implement ability to estimate object pose with markers seen by different cameras
          const Camera* camera = &cluster[0].first.obsMarker->GetSeenBy();
          
          for(auto & clusterMember : cluster)
          {
            const Vision::ObservedMarker* obsMarker = clusterMember.first.obsMarker;
            
            if(&obsMarker->GetSeenBy() == camera)
            {
              const Vision::KnownMarker* libMarker = clusterMember.first.libMarker;
              
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
          Pose3d objPose(camera->computeObjectPose(imgPoints, objPoints));
          
          // Now get the object pose in World coordinates using the pose
          // tree, instead of being in camera-centric coordinates, and
          // assign that pose to the temporary object of this type
          objectsSeen.push_back(libObject->clone());
          objectsSeen.back()->SetPose(objPose.getWithRespectTo(Pose3d::World));
          
        } // if 1 or more members in this cluster
        
      } // for each cluster
      
    } // GroupPosesIntoObjects()
    */
    
  } // namespace Vision
} // namespace Anki