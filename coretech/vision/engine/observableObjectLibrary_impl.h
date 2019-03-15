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

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/observableObjectLibrary.h"

#include "coretech/common/engine/exceptions.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/quad_impl.h"

namespace Anki {
namespace Vision {
  
  namespace {
    // After observing the charger, we clamp its pose to be 'flat' if it is within this tolerance of being flat already
    constexpr float kClampChargerPoseAngleTol_rad = DEG_TO_RAD(5.f);
  }
  
  template<class ObsObjectType>
  const std::set<const ObsObjectType*> ObservableObjectLibrary<ObsObjectType>::sEmptyObjectVector;
  
  template<class ObsObjectType>
  ObservableObjectLibrary<ObsObjectType>::~ObservableObjectLibrary<ObsObjectType>()
  {

  }
  
  template<class ObsObjectType>
  const ObsObjectType* ObservableObjectLibrary<ObsObjectType>::GetObjectWithCode(const Marker::Code& code) const
  {
    auto temp = _objectWithCode.find(code);
    if(temp != _objectWithCode.end()) {
      return temp->second;
    }
    else {
      return nullptr;
    }
  }
  
  template<class ObsObjectType>
  const ObsObjectType* ObservableObjectLibrary<ObsObjectType>::GetObjectWithMarker(const Marker& marker) const
  {
    return GetObjectWithCode(marker.GetCode());
  }
  
  template<class ObsObjectType>
  Result ObservableObjectLibrary<ObsObjectType>::AddObject(std::unique_ptr<const ObsObjectType>&& object)
  {
    // Make sure we don't have anything already using any of the markers on this object
    // Note that we don't need to worry if we find an object with the same markers that also
    // has the same type: it will be completely replaced by this new object anyway.
    // We do this first so that we can't accidentally remove an existing type, _then_ discover another object (with
    // different type) that has duplicate markers, return RESULT_FAIL, and then not have actually replaced the type
    // we intended to.
    for(auto & marker : object->GetMarkers())
    {
      const ObsObjectType* knownObjWithMarker = GetObjectWithMarker(marker);
      if(nullptr != knownObjWithMarker && (knownObjWithMarker->GetType() != object->GetType()))
      {
        PRINT_NAMED_WARNING("ObservableObjectsLibrary.AddObject.MarkerAlreadyInUse",
                            "Cannot add %s object. Already have %s object using %s marker",
                            EnumToString(object->GetType()),
                            EnumToString(knownObjWithMarker->GetType()),
                            MarkerTypeStrings[marker.GetCode()]);
        
        return RESULT_FAIL;
      }
    }
    
    // See if we are replacing an existing known type. If so, remove it and unregister its markers.
    s32 numTypesMatched = 0;
    for (auto knownObjectIter = _knownObjects.begin(); knownObjectIter != _knownObjects.end(); )
    {
      const ObsObjectType* knownObject = knownObjectIter->get();
      if (knownObject->GetType() == object->GetType())
      {
        PRINT_NAMED_WARNING("ObservableObjectLibrary.AddObject.RemovingPreviousDefinition",
                            "The old definition for a %s object was erased from the library.",
                            EnumToString(object->GetType()));
       
        // Since each Marker can only be bound to one Object, clear out the entries for the
        // matching known object we are about to replace
        for (const auto& marker : knownObject->GetMarkers())
        {
          _objectWithCode.erase(marker.GetCode());
        }
        
        knownObjectIter = _knownObjects.erase(knownObjectIter);
        ++numTypesMatched;
      }
      else
      {
        ++knownObjectIter;
      }
    }
    
    // It should never be possible to have more than one object of the same type in the library, so we can't have
    // matched more than one
    DEV_ASSERT(numTypesMatched <= 1, "ObservableObjectLibrary.AddObject.MultipleTypeMatchesFound");
    
    // Actually add the object to our known objects and register each marker it has on it
    for(auto & marker : object->GetMarkers()) {
      _objectWithCode[marker.GetCode()] = object.get();
      _unknownMarkerWarningIssued.erase(marker.GetCode()); 
    }
    _knownObjects.push_back(std::move(object));
    
    return RESULT_OK;
  }
  
  template<class ObsObjectType>
  bool ObservableObjectLibrary<ObsObjectType>::RemoveObjectWithMarker(const Marker::Code& code)
  {
    auto objectWithCodeIter = _objectWithCode.find(code);
    if(objectWithCodeIter == _objectWithCode.end())
    {
      // Code not in use by any known objects
      return false;
    }
    else
    {
      bool objectFound = false;
      const ObsObjectType* objectToRemove = objectWithCodeIter->second;
      auto knownObjectIter = _knownObjects.begin();
      while(knownObjectIter != _knownObjects.end())
      {
        if(knownObjectIter->get() == objectToRemove)
        {
          // Get rid of the reverse lookup entry for _all_ codes on the matching
          // known object (not just the one passed in that we used to find the object)
          for (const auto& marker : (*knownObjectIter)->GetMarkers())
          {
            _objectWithCode.erase(marker.GetCode());
          }
          
          // Now erase the known object entry
          _knownObjects.erase(knownObjectIter);
          objectFound = true;
          break;
        }
        
        ++knownObjectIter;
      }
      
      DEV_ASSERT(objectFound, "ObservableObjectLibrary.RemoveObjectWithMarker.ObjectNotFound");
      DEV_ASSERT(_objectWithCode.find(code) == _objectWithCode.end(),
                 "ObservableObjectLibrary.RemoveObjectWithMarker.ObjectWithCodeEntryStillFound");
      
      return true;
    }
  }
  
  template<class ObsObjectType>
  void ObservableObjectLibrary<ObsObjectType>::CreateObjectsFromMarkers(const std::list<ObservedMarker>& markers,
                                                                        std::vector<std::shared_ptr<ObsObjectType>>& objectsSeen) const
  {
    std::map<const ObsObjectType*, std::vector<const ObservedMarker*>> markersByLibObject;
    
    for(auto &marker : markers)
    {
      // Find the object which uses this marker...
      const ObsObjectType* objectWithMarker = GetObjectWithMarker(marker);
      
      // ...if there is one, add this marker to the list of observed markers
      // that corresponds to this object type.
      if(nullptr != objectWithMarker)
      {
        markersByLibObject[objectWithMarker].push_back(&marker);
      }
      else
      {
        auto result = _unknownMarkerWarningIssued.insert(marker.GetCode());
        const bool didInsert = result.second;
        if(didInsert) // i.e., new entry: no warning issued yet
        {
          PRINT_NAMED_WARNING("ObservableObjectLibrary.CreateObjectsFromMarkers.UnusedMarker",
                              "No objects in library use observed '%s' marker",
                              marker.GetCodeName());
        }
      }
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
        // Note that each observed marker can generate multiple poses because
        // an object may have the same known marker on it several times.
        
        libObject->ComputePossiblePoses(obsMarker, possiblePoses);
        
      } // FOR each observed marker
      
      // TODO: make the distance/angle thresholds parameters or else object-type-specific
      std::vector<PoseCluster> poseClusters;
      ClusterObjectPoses(possiblePoses, libObject,
                         5.f, 5.f*M_PI_F/180.f, poseClusters);
      
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
        // We default the PoseState to `known` right away, since we are assuming that the caller wants to create an
        // object based on a visual observation, and one observation is enough to 'confirm' the object's pose.
        ObsObjectType* newObject = libObject->CloneType();
        Pose3d newPose = poseCluster.GetPose().GetWithRespectToRoot();
        
        // If this is a charger (and hence could be used for localization), we want to clamp the pose to flat in order
        // to avoid weird localization updates. This implicitly assumes that the charger is indeed sitting flat on the
        // table/surface.
        if (IsChargerType(newObject->GetType(), false)) {
          ObservableObject::ClampPoseToFlat(newPose, kClampChargerPoseAngleTol_rad);
        }
        
        const float obsDistance_mm = poseCluster.GetPose().GetTranslation().Length();
        newObject->InitPose(newPose, PoseState::Known, obsDistance_mm);
        
        // Set the markers in the object corresponding to those from the pose
        // cluster from which it was computed as "observed"
        for(auto & match : poseCluster.GetMatches()) {
          // Set the observed markers based on their position in the image,
          // not based on their code, since an object can have multiple markers
          // with the same code.
          newObject->SetMarkerAsObserved(match.first, observedTime);
        }
        
        newObject->SetLastObservedTime(observedTime);
        
        // Finally actually insert the object into the objectsSeen container
        objectsSeen.emplace_back(newObject);
        
      } // FOR each pose cluster
      
    } // FOR each objectType
    
  } // CreateObjectsFromMarkers()
  
  template<class ObsObjectType>
  ObservableObjectLibrary<ObsObjectType>::PoseCluster::PoseCluster(const PoseMatchPair& match)
  : _pose(match.first)
  {
    const MarkerMatch& markerMatch = match.second;
    const ObservedMarker* obsMarker = markerMatch.first;
    const KnownMarker* knownMarker  = markerMatch.second;
    _matches.emplace_back(obsMarker, *knownMarker);
    
    // Keep a unique set of observed markers as part of the cluster so that
    // we don't add multiple (ambiguous) poses to one cluster that
    // were generated by the same observed marker.
    _obsMarkerSet.insert(obsMarker);
    
  }
  
  template<class ObsObjectType>
  bool ObservableObjectLibrary<ObsObjectType>::PoseCluster::TryToAddMatch(const PoseMatchPair& match,
                                                           const float distThreshold,
                                                           const Radians angleThreshold,
                                                           const RotationAmbiguities& R_ambiguities)
  {
    bool wasAdded = false;
    const Pose3d& P_other = match.first;

    if(R_ambiguities.HasAmbiguities()) {
      wasAdded = _pose.IsSameAs_WithAmbiguity(P_other, R_ambiguities,
                                              distThreshold, angleThreshold);
    }
    else {
      wasAdded = _pose.IsSameAs(P_other, distThreshold, angleThreshold);
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
      
      // If there were rotation ambiguities to look for, modify
      // the original (canonical) KnownMarker's pose based on P_diff
      // so that all cluster members' KnownMarkers will be in the same
      // coordinate frame and we can use them later to recompute this
      // cluster's pose based on all its member matches.
      
      Pose3d P_diff = _pose.GetInverse();
      P_diff *= P_other;
      
      if(R_ambiguities.HasAmbiguities()) {
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
      //fprintf(stdout, "Re-computing pose from all %zu members of cluster.", GetSize());
      
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
          PRINT_NAMED_WARNING("ObservableObjectLibrary.PoseCluster.RecomputePose.NotImplemented",
                              "Ability to re-estimate single object's "
                              "pose from markers seen by two different cameras "
                              "not yet implemented. Will just use markers from "
                              "first camera in the cluster.");
        }
        
      } // for each cluster member
      
      // Compute the object pose from all the corresponding 2d (image)
      // and 3d (object) points
      const Pose3d& originalParent = _pose.GetParent();
      _pose = camera->ComputeObjectPose(imgPoints, objPoints);
      if(!_pose.IsChildOf(originalParent)) {
        if(_pose.GetWithRespectTo(originalParent, _pose) == false) {
          PRINT_NAMED_ERROR("ObservableObjectLibrary.PoseCluster.RecomputePose.OriginMisMatch",
                            "Could not get object pose w.r.t. original parent.");
        }
      }
    } // IF more than one member
    
  } // RecomputePose()
  
  template<class ObsObjectType>
  void ObservableObjectLibrary<ObsObjectType>::ClusterObjectPoses(const std::vector<PoseMatchPair>& possiblePoses,
                                                                  const ObsObjectType*              libObject,
                                                                  const float                       distThreshold,
                                                                  const Radians                     angleThreshold,
                                                                  std::vector<PoseCluster>&         poseClusters) const
  {
    std::vector<bool> assigned(possiblePoses.size(), false);
    
    const RotationAmbiguities& R_amb = libObject->GetRotationAmbiguities();
    
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

