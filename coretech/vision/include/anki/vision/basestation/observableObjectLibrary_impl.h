/**
 * File: observableObjectLibrary_impl.h
 *
 * Author: Andrew Stein
 * Date:   8-11-2014
 *
 * Description: Implements an ObservableObjectLibrary class for keeping up with
 *              a set of known ObservableObjects we expect to see in the world.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef __Anki_Vision_ObservableObjectLibrary_H__
#define __Anki_Vision_ObservableObjectLibrary_H__

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/observableObjectLibrary.h"

#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"


namespace Anki {
namespace Vision {
  
  template<class ObsObjectType>
  const std::set<const ObsObjectType*> ObservableObjectLibrary<ObsObjectType>::sEmptyObjectVector;
  
  template<class ObsObjectType>
  std::set<const ObsObjectType*> const& // Return value
  ObservableObjectLibrary<ObsObjectType>::GetObjectsWithCode(const Marker::Code& code) const
  {
    auto temp = _objectsWithCode.find(code);
    if(temp != _objectsWithCode.end()) {
      return temp->second;
    }
    else {
      return ObservableObjectLibrary::sEmptyObjectVector;
    }
  }
  
  template<class ObsObjectType>
  std::set<const ObsObjectType*> const& // Return value
  ObservableObjectLibrary<ObsObjectType>::GetObjectsWithMarker(const Marker& marker) const
  {
    return GetObjectsWithCode(marker.GetCode());
  }
  
  template<class ObsObjectType>
  void ObservableObjectLibrary<ObsObjectType>::AddObject(const ObsObjectType* object)
  {
    // TODO: Warn/error if we are overwriting an existing object with this type?
    _knownObjects.push_back(object);
    for(auto & marker : object->GetMarkers()) {
      _objectsWithCode[marker.GetCode()].insert(object);
    }
  }
  
  void ObservableObjectLibrary::CreateObjectsFromMarkers(const std::list<ObservedMarker*>& markers,
                                                         std::multimap<f32, ObservableObject*>& objectsSeen,
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
        
        // Create a new object of the observed type from the library, and set
        // its pose to the computed pose, in _historical_ world frame (since
        // it is computed w.r.t. a camera from a pose in history).
        //objectsSeen.push_back(libObject->CloneType());
        ObservableObject* newObject = libObject->CloneType();
        const f32 observedDistSq = poseCluster.GetPose().GetTranslation().LengthSq();
        Pose3d newPose = poseCluster.GetPose().GetWithRespectToOrigin();
        newObject->SetPose(newPose);
        
        // Set the markers in the object corresponding to those from the pose
        // cluster from which it was computed as "observed"
        for(auto & match : poseCluster.GetMatches()) {
          // Set the observed markers based on their position in the image,
          // not based on their code, since an object can have multiple markers
          // with the same code.
          //const KnownMarker& marker = match.second;
          //objectsSeen.back()->SetMarkersAsObserved(marker.GetCode(),
          //                                         observedTime);
          
          newObject->SetMarkerAsObserved(match.first, observedTime, 5.f);
        }
        
        newObject->SetLastObservedTime(observedTime);
        
        // Finally actually insert the object into the objectsSeen container,
        // using its (squared) distance from the robot (really, the robot's camera)
        // as a sorting criterion
        objectsSeen.insert(std::make_pair(observedDistSq, newObject));
        
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

#if 0
  // Input:   list of observed markers
  // Outputs: the objects seen and the unused markers
  template<class ObsObjectType>
  Result ObservableObjectLibrary<ObsObjectType>::CreateObjectsFromMarkers(const std::list<ObservedMarker*>& markers,
                                                                          std::multimap<ObsObjectType*>& objectsSeen,
                                                                          const CameraID_t seenOnlyBy) const
  {
    // Group the markers by the lib object to which they match
    std::map<const ObsObjectType*, std::vector<const ObservedMarker*>> markersByLibObject;
    
    for(auto marker : markers) {
      
      marker->MarkUsed(false);
      
      // If seenOnlyBy was specified, make sure this marker was seen by that
      // camera
      if(seenOnlyBy == ANY_CAMERA || marker->GetSeenBy().GetID() == seenOnlyBy)
      {
        // Find all objects which use this marker...
        std::set<const ObsObjectType*> const& objectsWithMarker = GetObjectsWithMarker(*marker);
        
        // ...if there are any, add this marker to the list of observed markers
        // that corresponds to this object type.
        if(!objectsWithMarker.empty()) {
          if(objectsWithMarker.size() > 1) {
            PRINT_NAMED_ERROR("ObservableObjectLibrary.CreateObjectFromMarkers.MultipleLibObjectsWithMarker",
                              "Having multiple objects in the library with the "
                              "same marker ('%s') is not supported.",
                              marker->GetCodeName());
            return RESULT_FAIL;
          }
          markersByLibObject[*objectsWithMarker.begin()].push_back(marker);
          
          marker->MarkUsed(true);
        } // IF objectsWithMarker != NULL
      } // IF seenOnlyBy
      
    } // For each marker we saw
    
    // Now go through each object type we saw a marker for, and use the
    // corresponding observed markers to create a list of possible poses
    // for that object. Then cluster the possible poses into one or more
    // object instances of that type, returned in the "objectsSeen" vector.
    for(auto & libObjectMarkersPair : markersByLibObject)
    {
      // A place to store the possible poses together with the observed/known
      // markers that implied them
      std::vector<PoseMatchPair> possiblePoses;
      
      const ObsObjectType* libObject = libObjectMarkersPair.first;
      
      // Get timestamp of the observed markers so that I can set the
      // lastSeenTime of the observable object.
      auto obsMarker = libObjectMarkersPair.second.begin();
      const TimeStamp_t observedTime = (*obsMarker)->GetTimeStamp();
      
      // Check that all remaining markers also have the same timestamp (which
      // they now should, since we are processing grouped by timestamp)
      while(++obsMarker != libObjectMarkersPair.second.end()) {
        CORETECH_ASSERT(observedTime == (*obsMarker)->GetTimeStamp());
      }
      
      for(auto obsMarker : libObjectMarkersPair.second)
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
        
        // Create a new object of the observed type from the library, and set
        // its pose to the computed pose, in _historical_ world frame (since
        // it is computed w.r.t. a camera from a pose in history).
        objectsSeen.push_back(libObject->CloneType());
        Pose3d newPose = poseCluster.GetPose().GetWithRespectToOrigin();
        objectsSeen.back()->SetPose(newPose);
        
        // Set the markers in the object corresponding to those from the pose
        // cluster from which it was computed as "observed"
        for(auto & match : poseCluster.GetMatches()) {
          // Set the observed markers based on their position in the image,
          // not based on their code, since an object can have multiple markers
          // with the same code.
          //const KnownMarker& marker = match.second;
          //objectsSeen.back()->SetMarkersAsObserved(marker.GetCode(),
          //                                         observedTime);
          
          objectsSeen.back()->SetMarkerAsObserved(match.first, observedTime, 5.f);
          
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
    
    return RESULT_OK;
  } // CreateObjectsFromMarkers()
  
#endif
  
  template<class ObsObjectType>
  ObservableObjectLibrary<ObsObjectType>::PoseCluster::PoseCluster(const PoseMatchPair& match)
  : _pose(match.first)
  {
    const MarkerMatch& markerMatch = match.second;
    const ObservedMarker* obsMarker = markerMatch.first;
    const KnownMarker* knownMarker  = markerMatch.second;
    _matches.emplace_back(obsMarker, *knownMarker);
    
    // Mark the new KnownMarker object in this match as observed
    //_matches.back().second.SetWasObserved(true);
    
    // Keep a unique set of observed markers as part of the cluster so that
    // we don't add multiple (ambiguous) poses to one cluster that
    // were generated by the same observed marker.
    _obsMarkerSet.insert(obsMarker);
    
  }
  
  template<class ObsObjectType>
  bool ObservableObjectLibrary<ObsObjectType>::PoseCluster::TryToAddMatch(const PoseMatchPair& match,
                                                           const float distThreshold,
                                                           const Radians angleThreshold,
                                                           const std::vector<RotationMatrix3d>& R_ambiguities)
  {
    bool wasAdded = false;
    const Pose3d& P_other = match.first;

    if(R_ambiguities.empty()) {
      wasAdded = _pose.IsSameAs(P_other, distThreshold, angleThreshold);
    }
    else {
      wasAdded = _pose.IsSameAs_WithAmbiguity(P_other, R_ambiguities,
                                              distThreshold, angleThreshold, true);
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
    if(wasAdded && _obsMarkerSet.count(match.second.first)==0) {
      
      _obsMarkerSet.insert(match.second.first);
      
      // Create a match between the original observed marker pointer
      // and a copy of the original KnownMarker.
      _matches.emplace_back(match.second.first, *match.second.second);
      
      // Mark the new KnownMarker object in this match as observed
      //_matches.back().second.SetWasObserved(true);
      
      // If there were rotation ambiguities to look for, modify
      // the original (canonical) KnownMarker's pose based on P_diff
      // so that all cluster members' KnownMarkers will be in the same
      // coordinate frame and we can use them later to recompute this
      // cluster's pose based on all its member matches.
      
      Pose3d P_diff = _pose.GetInverse();
      P_diff *= P_other;
      
      if(not R_ambiguities.empty()) {
        Pose3d newPose(_matches.back().second.GetPose());
        newPose.PreComposeWith(P_diff);
        
        // This should preserve the pose tree (i.e. parent relationship
        // of the KnownMarker to its parent object's pose)
        //CORETECH_ASSERT(newPose.GetParent() == &libObject->GetPose());
        
        _matches.back().second.SetPose(newPose);
      } // if there were rotation ambiguities
    }
    
    return wasAdded;
  } // TryToAddMatch()
  
  template<class ObsObjectType>
  void ObservableObjectLibrary<ObsObjectType>::PoseCluster::RecomputePose()
  {
    if(GetSize() > 1) {
      //fprintf(stdout, "Re-computing pose from all %zu members of cluster.\n", GetSize());
      
      std::vector<Point2f> imgPoints;
      std::vector<Point3f> objPoints;
      imgPoints.reserve(4*GetSize());
      objPoints.reserve(4*GetSize());
      
      const Camera* camera = &_matches.front().first->GetSeenBy();
      
      for(auto & match : _matches)
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
      const Pose3d* originalParent = _pose.GetParent();
      _pose = camera->ComputeObjectPose(imgPoints, objPoints);
      if(_pose.GetParent() != originalParent) {
        if(_pose.GetWithRespectTo(*originalParent, _pose) == false) {
          PRINT_NAMED_ERROR("ObservableObjectLibrary.PoseCluster.RecomputePose.OriginMisMatch",
                            "Could not get object pose w.r.t. original parent.\n");
        }
      }
    } // IF more than one member
    
  } // RecomputePose()
  
  template<class ObsObjectType>
  void ObservableObjectLibrary<ObsObjectType>::ClusterObjectPoses(const std::vector<PoseMatchPair>& possiblePoses,
                                                   const ObsObjectType*         libObject,
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

#endif // __Anki_Vision_ObservableObjectLibrary_H__

