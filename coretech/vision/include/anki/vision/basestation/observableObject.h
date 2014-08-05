/**
 * File: observableObject.h
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Defines an abstract ObservableObject class, which is an 
 *              general 3D object, with type, ID, and pose, and a set of Markers
 *              on it at known locations.  Thus, it is "observable" by virtue of
 *              having these markers, and its 3D / 6DoF pose can be estimated by
 *              matching up ObservedMarkers with the KnownMarkers it possesses.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

// TODO: Separate ObservableObjectLibrary into its own h/cpp files

#ifndef ANKI_VISION_OBSERVABLE_OBJECT_H
#define ANKI_VISION_OBSERVABLE_OBJECT_H

#include <list>
#include <set>

#include "anki/vision/basestation/visionMarker.h"

#include "anki/common/basestation/objectTypesAndIDs.h"

namespace Anki {
  namespace Vision {
    
    // Forward declaration
    class Camera;
    
    // A marker match is a pairing of an ObservedMarker with a KnownMarker
    using MarkerMatch = std::pair<const Vision::ObservedMarker*, const Vision::KnownMarker*>;
    
    // Pairing of a pose and the match which implies it
    using PoseMatchPair = std::pair<Pose3d, MarkerMatch>;
    
    
    class ObservableObject
    {
    public:
      // Do we want to be req'd to instantiate with all codes up front?
      ObservableObject(){};
      
      ObservableObject(ObjectType type);
      
      //ObservableObject(const std::vector<std::pair<Marker::Code,Pose3d> >& codesAndPoses);
      
      virtual ~ObservableObject(){};
      
      // Specify the extistence of a marker with the given code at the given
      // pose (relative to the object's origin), and the specfied size in mm
      Vision::KnownMarker const& AddMarker(const Marker::Code& withCode,
                                           const Pose3d&       atPose,
                                           const f32           size_mm);
      
      std::list<KnownMarker> const& GetMarkers() const {return markers_;}
      
      // Return a const reference to a vector all this object's Markers with the
      // specified code. The returned vector will be empty if there are no markers
      // with that code.
      std::vector<KnownMarker*> const& GetMarkersWithCode(const Marker::Code& whichCode) const;
      
      // Populate a vector of const pointers to all this object's markers that
      // have been observed since the specified time. The vector will be unchanged
      // if no markers have been observed since then. Note that the vector is
      // not cleared internally, so this method _adds_ markers to existing
      // container. It is the caller's responsibility to clear the vector if
      // desired.
      void GetObservedMarkers(std::vector<const KnownMarker*>& observedMarkers,
                              const TimeStamp_t sinceTime) const;

      // Updates the observation times of this object's markers with the newer
      // of the current times and the times of the corresponding markers on the
      // other object. If the other object is not the same type, RESULT_FAIL
      // is returned.
      Result UpdateMarkerObservationTimes(const ObservableObject& otherObject);
      
      // Add possible poses implied by seeing the observed marker to the list.
      // Each pose will be paired with a pointer to the known marker on this
      // object from which it was computed.
      // If the marker doesn't match any found on this object, the possiblePoses
      // list will not be modified.
      void ComputePossiblePoses(const ObservedMarker*     obsMarker,
                                std::vector<PoseMatchPair>& possiblePoses) const;

      // Sets all markers with the specified code as having been observed
      // at the given time
      void SetMarkersAsObserved(const Marker::Code& withCode,
                                const TimeStamp_t atTime);
      
      // Return true if any of the object's markers is visible from the given
      // camera. See also KnownMarker::IsVisibleFrom().
      bool IsVisibleFrom(const Camera& camera,
                         const f32 maxFaceNormalAngle,
                         const f32 minMarkerImageSize,
                         const bool requireSomethingBehind) const;
      
      // Accessors:
      ObjectID        GetID()     const;
      ObjectType      GetType()   const;
      const Pose3d&   GetPose()   const;
      //virtual float GetMinDim() const = 0;
      
      void SetID();
      void SetPose(const Pose3d& newPose);
      void SetPoseParent(const Pose3d* newParent);
      
      void SetLastObservedTime(TimeStamp_t t) {lastObservedTime_ = t;}
      const TimeStamp_t GetLastObservedTime() const {return lastObservedTime_;}
      
      // Return true if this object is the same as the other. Sub-classes can
      // overload this function to provide for rotational ambiguity when
      // comparing, e.g. for cubes or other blocks.
      virtual bool IsSameAs(const ObservableObject&  otherObject,
                            const Point3f&  distThreshold,
                            const Radians&  angleThreshold,
                            Pose3d& P_diff) const;
      
      virtual bool IsSameAs(const ObservableObject&  otherObject,
                            const Point3f& distThreshold,
                            const Radians& angleThreshold) const;
      
      // Same as other IsSameAs() calls above, except the tolerance thresholds
      // given by the virtual members below are used (so that per-object-class
      // thresholds can be defined)
      bool IsSameAs(const ObservableObject& otherObject) const;
      bool IsSameAs(const ObservableObject& otherObject, Pose3d& P_diff) const;
      
      virtual Point3f GetSameDistanceTolerance() const = 0;
      virtual Radians GetSameAngleTolerance() const = 0;
      
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const;
      
      // For creating derived objects from a pointer to this base class, see
      // ObservableObjectBase below
      virtual ObservableObject* Clone() const = 0;
      
      virtual void GetCorners(std::vector<Point3f>& corners) const;
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const = 0;
      
      virtual void Visualize() = 0;
      virtual void EraseVisualization() = 0;
      
      Quad2f GetBoundingQuadXY(const f32 padding_mm = 0.f) const;
      virtual Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm = 0.f) const = 0;
      
    protected:
      
      static const std::vector<RotationMatrix3d> sRotationAmbiguities;
      static const std::vector<KnownMarker*> sEmptyMarkerVector;
      
      ObjectType   type_;
      ObjectID     ID_;
      TimeStamp_t  lastObservedTime_;
      
      // Using a list here so that adding new markers does not affect references
      // to pre-existing markers
      std::list<KnownMarker> markers_;
      
      // Holds a LUT (by code) to all the markers of this object which have that
      // code.
      std::map<Marker::Code, std::vector<KnownMarker*> > markersWithCode_;
      
      bool wasObserved_;
      
      // For subclasses can get a modifiable pose reference
      Pose3d& GetNonConstPose() { return pose_; }
      
      /*
      // Canonical pose used as the parent pose for the object's markers so
      // possiblePoses for this object can be computed from observations of its
      // markers
      Pose3d canonicalPose_;
      
      // We will represent any ambiguity in an object's pose by simply keeping
      // a list of possible poses.  Since a single (oriented) marker can
      // completely determine 6DoF pose, the ambiguity aries from identical
      // markers used multiple times on an object with symmetry -- and thus
      // there should be a small, finite, discrete set of possible poses we need
      // to store here.
      // Note this is a separate notion than the uncertainty of each estimated
      // pose, which should be stored inside the Pose3d object, if represented
      // at all.
      std::list<Pose3d> possiblePoses_;
      */
    private:
      // Force setting of pose through SetPose() to keep pose name updated
      Pose3d pose_;
      
      // Don't allow a copy constuctor, because it won't handle fixing the
      // marker pointers and pose parents
      ObservableObject(const ObservableObject& other);
      
    };
  
    
    //
    // Inline accessors
    //
    /*
    inline std::set<const Camera*> const& ObservableObject::GetObserver() const {
      return observers_;
    }
     */
    
    inline ObjectID ObservableObject::GetID() const {
      return ID_;
    }
    
    inline ObjectType ObservableObject::GetType() const {
      return type_;
    }
    
    //virtual float GetMinDim() const = 0;
    inline const Pose3d& ObservableObject::GetPose() const {
      return pose_;
    }
    
    inline void ObservableObject::SetID() { //const ObjectID newID) {
      ID_.Set();
      //ID_ = newID;
    }
  
    inline void ObservableObject::SetPose(const Pose3d& newPose) {
      pose_ = newPose;
      
      std::string poseName(GetType().GetName());
      if(poseName.empty()) {
        poseName = "Object";
      }
      poseName += "_" + std::to_string(GetID().GetValue());
      pose_.SetName(poseName);
    }
    
    inline void ObservableObject::SetPoseParent(const Pose3d* newParent) {
      pose_.SetParent(newParent);
    }
    
    inline bool ObservableObject::IsSameAs(const ObservableObject&  otherObject,
                                           const Point3f&           distThreshold,
                                           const Radians&           angleThreshold) const
    {
      Pose3d P_diff_temp;
      return this->IsSameAs(otherObject, distThreshold, angleThreshold,
                            P_diff_temp);
    }
    
    inline std::vector<RotationMatrix3d> const& ObservableObject::GetRotationAmbiguities() const
    {
      return ObservableObject::sRotationAmbiguities;
    }
    
    inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject, Pose3d& P_diff) const {
      return IsSameAs(otherObject, this->GetSameDistanceTolerance(), this->GetSameAngleTolerance(), P_diff);
    }
    
    inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject) const {
      return IsSameAs(otherObject, this->GetSameDistanceTolerance(), this->GetSameAngleTolerance());
    }
    
    /*
    // Derive specific observable objects from this class to get a clone method,
    // without having to specifically write one in each derived class.
    //
    // I believe this is know as the Curiously Recurring Template Pattern (CRTP)
    //
    // For example:
    //   class SomeNewObject : public ObservableObjectBase<SomeNewObject> { ... };
    //
    template <class Derived>
    class ObservableObjectBase : public ObservableObject
    {
    public:
      virtual ObservableObject* clone() const
      {
        // Call the copy constructor
        return new Derived(static_cast<const Derived&>(*this));
      }
    };
    */
    
    class ObservableObjectLibrary
    {
    public:
      
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
      const ObservableObject* GetObjectWithType(const ObjectType type) const;
      
      // Return a pointer to a vector of pointers to known objects with at
      // least one of the specified markers or codes on it. If  there is no
      // object with that marker/code, a NULL pointer is returned.
      std::set<const ObservableObject*> const& GetObjectsWithMarker(const Marker& marker) const;
      std::set<const ObservableObject*> const& GetObjectsWithCode(const Marker::Code& code) const;
      
    protected:
      
      static const std::set<const ObservableObject*> sEmptyObjectVector;
      
      //std::list<const ObservableObject*> knownObjects_;
      std::map<ObjectType, const ObservableObject*> knownObjects_;
      
      // Store a list of pointers to all objects that have at least one marker
      // with that code.  You can then use the objects' GetMarkersWithCode()
      // method to get the list of markers on each object.
      //std::map<Marker::Code, std::vector<const ObservableObject*> > objectsWithCode_;
      std::map<Marker::Code, std::set<const ObservableObject*> > objectsWithCode_;
      
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
        
        const Pose3d& GetPose() const { return pose_; }
        const size_t  GetSize() const { return matches_.size(); }
        
        const MatchList& GetMatches() const { return matches_; }
        
        // Updates pose based on all member matches. Does nothing if there is
        // only one member.
        void RecomputePose();
        
      protected:
        Pose3d pose_;
        
        MatchList matches_;
        
        std::set<const ObservedMarker*> obsMarkerSet_;
        
      }; // class PoseCluster
      
      void ClusterObjectPoses(const std::vector<PoseMatchPair>& possiblePoses,
                              const ObservableObject*         libObject,
                              const float distThreshold, const Radians angleThreshold,
                              std::vector<PoseCluster>& poseClusters) const;

    }; // class ObservableObjectLibrary
    
    
  } // namespace Vision
} // namespace Anki

#endif // ANKI_VISION_OBSERVABLE_OBJECT_H