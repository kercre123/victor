#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/observableObject.h"


namespace Anki {
  namespace Vision {
    
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
    
    void ObservableObjectLibrary::AddObject(const ObservableObject* object)
    {
      // TODO: Warn/error if we are overwriting an existing object with this ID?
      knownObjects_[object->GetID()] = object;
      for(auto marker : object->GetMarkers()) {
        objectsWithCode_[marker.GetCode()].push_back(object);
      }
    }
    
    
    void ObservableObjectLibrary::CreateObjectsFromMarkers(std::list<ObservedMarker>& markers,
                                                           std::vector<ObservableObject*> objectsSeen) const
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
      
      // Now go through each object ID we saw a marker for, and create an
      // instance of that object with the pose implied by where we saw the marker.
      // Then, cluster the observations and add one or more objects of that
      // type to the "objectsSeen" vector.
      for(auto & objIdMarkersPair : markersWithObjectID)
      {
        // A place to store all computed poses for this object
        std::vector<std::vector<MatchWithPose> > possibleObjPoses;
        //std::map<const Marker*, std::vector<Pose3d> > objPoses;
        
        const Vision::ObservableObject* libObject = GetObjectWithID(objIdMarkersPair.first);
        
        for(auto marker : objIdMarkersPair.second)
        {
          // We will compute possible poses from each observed marker, paired
          // with a reference to that marker.  (This is so we can recompute
          // object pose from clusters of markers later, without having to
          // reassociate them.) Note that each marker can generate multiple
          // poses because an object may have the same marker on it several
          // times.
          //objPoses[marker] = std::vector<Pose3d>();
          possibleObjPoses.emplace_back();
          ComputeObjectPoses(marker, libObject, possibleObjPoses.back());
          
        } // FOR each marker
        
        // Group individual poses into objects of this ID actually seen,
        // clustering as needed
        GroupPosesIntoObjects(possibleObjPoses, libObject, objectsSeen);
        
      } // FOR each objectID
      
      // Return the unused markers
      std::swap(markers, markersUnused);
      
    } // CreateObjectsFromMarkers()
    
    
    std::vector<const ObservableObject*> const* ObservableObjectLibrary::GetObjectsWithMarker(const Marker& marker) const
    {
      printf("ERROR: ObservableObjectLibrary::GetObjectsWithMarker() UNDEFINED\n");
      return NULL;
    }
    
    
    
    void ObservableObjectLibrary::
    ClusterObjectPoses(const std::vector<std::vector<MatchWithPose> >& poseMatches,
                       const f32 distThreshold, const Radians angleThreshold,
                       std::vector<std::vector<MatchWithPose> >& markerClusters) const
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
    
    
    void ObservableObjectLibrary::ComputeObjectPoses(const ObservedMarker* obsMarker,
                                                     const ObservableObject* libObject,
                                                     std::vector<MatchWithPose>& objPoses) const
    {
      // Lookup the set of markers on this library object which match the given
      // marker
      const std::vector<const KnownMarker*> *libMarkers = libObject->GetMarkersWithCode(obsMarker->GetCode());

      // If there are any matching markers...
      if(libMarkers != NULL) {
        // ...estimate the pose of the library marker if it was seen by the
        // camera that saw the observed marker, and add that to the list of
        // possible object poses.
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
    GroupPosesIntoObjects(const std::vector<std::vector<MatchWithPose> >&  objPoses,
                          const ObservableObject*                          libObject,
                          std::vector<ObservableObject*>&                  objectsSeen) const
    {
      // TODO: Make this a parameter somewhere
      const Radians angleDiffThreshold = 0.5f*M_PI_4;
      
      // Group markers which could be part of the same object (i.e. those
      // whose poses are colocated, noting that the pose is relative to
      // the center of the object)
      std::vector<std::vector<MatchWithPose> > markerClusters;
      ClusterObjectPoses(objPoses, 0.5f*libObject->GetMinDim(),
                         angleDiffThreshold, markerClusters);
      
      fprintf(stdout, "Grouping %lu observed 2D markers of type %d into %lu objects.\n",
              objPoses.size(), libObject->GetID(), markerClusters.size());
      
      
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
    
  } // namespace Vision
} // namespace Anki