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

#include "anki/vision/basestation/observableObject.h"
#include "anki/vision/basestation/visionMarker.h"


namespace Anki {
  namespace Vision {
    
    template<class ObsObjectType>
    class ObservableObjectLibrary
    {
    public:
      
      using ObservableObject = ObsObjectType;
      
      ObservableObjectLibrary(){};
      
      u32 GetNumObjects() const;
      
      //bool IsInitialized() const;
      
      // Add an object to the known list. Note that this permits addition of
      // objects at run time.
      void AddObject(const ObservableObject* object);
      
      // Groups markers referring to the same type, and clusters them into
      // observed objects, returned in objectsSeen.  Used markers will be
      // removed from the input list, so markers referring to objects unknown
      // to this library will remain.  If seenOnlyBy is not ANY_CAMERA, only markers
      // seen by that camera will be considered and objectSeen poses will be returned
      // wrt to that camera. If seenOnlyBy is ANY_CAMERA, the poses are returned wrt the world.
      void CreateObjectsFromMarkers(const std::list<ObservedMarker*>& markers,
                                    std::vector<ObservableObject*>& objectsSeen,
                                    const CameraID_t seenOnlyBy = ANY_CAMERA) const;
      
      // Only one object in a library can have each type. Return a pointer to
      // that object, or NULL if none exists.
      //const ObservableObject* GetObjectWithType(const TYPE type) const;
      
      // Return a pointer to a vector of pointers to known objects with at
      // least one of the specified markers or codes on it. If  there is no
      // object with that marker/code, a NULL pointer is returned.
      std::set<const ObservableObject*> const& GetObjectsWithMarker(const Marker& marker) const;
      std::set<const ObservableObject*> const& GetObjectsWithCode(const Marker::Code& code) const;
      
    protected:
      
      static const std::set<const ObservableObject*> sEmptyObjectVector;
      
      std::list<const ObservableObject*> _knownObjects;
      //std::map<TYPE, const ObservableObject*> _knownObjects;
      
      // Store a list of pointers to all objects that have at least one marker
      // with that code.  You can then use the objects' GetMarkersWithCode()
      // method to get the list of markers on each object.
      //std::map<Marker::Code, std::vector<const ObservableObject*> > _objectsWithCode;
      std::map<Marker::Code, std::set<const ObservableObject*> > _objectsWithCode;
      
      // A PoseCluster is a pairing of a single pose and all the marker matches
      // that imply that pose
      class PoseCluster
      {
      public:
        using MatchList = std::list<std::pair<const ObservedMarker*, KnownMarker> >;
        
        PoseCluster(const PoseMatchPair& match);
        
        // Returns true if match was added
        bool TryToAddMatch(const PoseMatchPair& match,
                           const float distThreshold, const Radians angleThreshold,
                           const std::vector<RotationMatrix3d>& R_ambiguities);
        
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
                              const ObservableObject*         libObject,
                              const float distThreshold, const Radians angleThreshold,
                              std::vector<PoseCluster>& poseClusters) const;
      
    }; // class ObservableObjectLibrary
    
  } // namespace Vision
} // namespace Anki

#endif // ANKI_VISION_OBSERVABLE_OBJECT_H