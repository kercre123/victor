/**
 * File: observableObjectLibrary.h
 *
 * Author: Andrew Stein
 * Date:   8-11-2014
 *
 * Description: Defines an ObservableObjectLibrary class for keeping up with
 *              a set of known ObservableObjects we expect to see in the world.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/


#ifndef ANKI_VISION_OBSERVABLE_OBJECT_LIBRARY_H
#define ANKI_VISION_OBSERVABLE_OBJECT_LIBRARY_H

#include <list>
#include <set>
#include <map>

#include "coretech/vision/engine/observableObject.h"
#include "coretech/vision/engine/visionMarker.h"


namespace Anki {
  namespace Vision {
    
    template<class ObsObjectType>
    class ObservableObjectLibrary
    {
    public:
      
      //using ObsObjectType = ObsObjectType;
      
      ObservableObjectLibrary() {};
      ~ObservableObjectLibrary();
      
      u32 GetNumObjects() const;
      
      // Add an object to the list of known objects this library can instantiate from observed markers.
      // Note that this permits addition of new objects at run time.
      // Will return RESULT_FAIL if library already contains an object using a marker present on the given object.
      Result AddObject(std::unique_ptr<const ObsObjectType>&& object);
      
      // Remove any object that uses the given marker code.
      // Returns true if an object is found and removed, false otherwise.
      bool RemoveObjectWithMarker(const Marker::Code& code);
      
      // Groups markers referring to the same object type, and clusters them into observed objects, returned in
      // objectsSeen. The object poses will be with respect to origin.
      void CreateObjectsFromMarkers(const std::list<ObservedMarker>& markers,
                                    std::vector<std::shared_ptr<ObsObjectType>>& objectsSeen) const;
      
      // Return a pointer to a known object with at least one of the specified marker or code on it. If there is no
      // object with that marker/code, a NULL pointer is returned.
      const ObsObjectType* GetObjectWithMarker(const Marker& marker) const;
      const ObsObjectType* GetObjectWithCode(const Marker::Code& code) const;
      
    protected:
      
      static const std::set<const ObsObjectType*> sEmptyObjectVector;
      
      std::list<std::unique_ptr<const ObsObjectType>> _knownObjects;
      
      // Backwards lookup table for which object uses a given marker code
      // Note that there can only be one object with each marker
      std::map<Marker::Code, const ObsObjectType*> _objectWithCode;
      
      // Only warn once per unknown marker to avoid spamming, e.g. while staring at an
      // undefined object (whose markers haven't been registered)
      mutable std::set<Marker::Code> _unknownMarkerWarningIssued;
      
      // A PoseCluster is a pairing of a single pose and all the marker matches
      // that imply that pose
      class PoseCluster
      {
      public:
        using MatchList = std::list<std::pair<const ObservedMarker*, KnownMarker>>;
        
        PoseCluster(const PoseMatchPair& match);
        
        // Returns true if match was added
        bool TryToAddMatch(const PoseMatchPair& match,
                           const float distThreshold, const Radians angleThreshold,
                           const RotationAmbiguities& R_ambiguities);
        
        const Pose3d& GetPose() const { return _pose; }
        const size_t  GetSize() const { return _matches.size(); }
        
        const MatchList& GetMatches() const { return _matches; }
        
        // Updates pose based on all member matches. Does nothing if there is
        // only one member.
        void RecomputePose();
        
      protected:
        Pose3d _pose;
        
        MatchList _matches;
        
        std::set<const ObservedMarker*> _obsMarkerSet;
        
      }; // class PoseCluster
      
      void ClusterObjectPoses(const std::vector<PoseMatchPair>& possiblePoses,
                              const ObsObjectType*         libObject,
                              const float distThreshold, const Radians angleThreshold,
                              std::vector<PoseCluster>& poseClusters) const;
      
    }; // class ObservableObjectLibrary
    
  } // namespace Vision
} // namespace Anki

#endif // ANKI_VISION_OBSERVABLE_OBJECT_H
