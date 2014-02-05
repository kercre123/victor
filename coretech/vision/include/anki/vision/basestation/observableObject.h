#ifndef ANKI_VISION_OBSERVABLE_OBJECT_H
#define ANKI_VISION_OBSERVABLE_OBJECT_H

#include <list>

#include "anki/vision/basestation/visionMarker.h"

namespace Anki {
  namespace Vision {
    
    // Forward declaration
    class Camera;
    
    // A marker match is a pairing of an ObservedMarker with a KnownMarker
    typedef std::pair<const Vision::ObservedMarker*, const Vision::KnownMarker*> MarkerMatch;
    
    // Pairing of a pose and the match which implies it
    typedef std::pair<Pose3d, MarkerMatch> PoseMatchPair;
    
    
    class ObservableObject
    {
    public:
      // Do we want to be req'd to instantiate with all codes up front?
      ObservableObject(){};
      
      ObservableObject(ObjectID_t ID);
      
      //ObservableObject(const std::vector<std::pair<Marker::Code,Pose3d> >& codesAndPoses);
      
      virtual ~ObservableObject(){};
      
      // Specify the extistence of a marker with the given code at the given
      // pose (relative to the object's origin), and the specfied size in mm
      void AddMarker(const Marker::Code& withCode, const Pose3d& atPose,
                     const f32 size_mm);
      
      std::list<KnownMarker> const& GetMarkers() const {return markers_;}
      
      // Return a pointer to a vector all this object's Markers with the
      // specified code. A NULL pointer is returned if there are no markers
      // with that code.
      std::vector<const KnownMarker*> const* GetMarkersWithCode(const Marker::Code& whichCode) const;

      // Add possible poses implied by seeing the observed marker to the list.
      // Each pose will be paired with a pointer to the known marker on this
      // object from which it was computed.
      // If the marker doesn't match any found on this object, the possiblePoses
      // list will not be modified.
      void ComputePossiblePoses(const ObservedMarker*     obsMarker,
                                std::vector<PoseMatchPair>& possiblePoses) const;
      
      // Attempt to merge this object with another, based on their lists of
      // possible poses. If successful, true is returned, and this object's
      // list of possible poses will be updated.  If this new observation
      // reduced the ambiguity of the pose, the list of possible poses will
      // be shorter.
      bool MergeWith(const ObservableObject& otherObject,
                     const float distThreshold,
                     const Radians angleThreshold);
      
      // Accessors:
      ObjectID_t    GetID() const {return ID_;}
      //virtual float GetMinDim() const = 0;
      const Pose3d& GetPose() const {return pose_;}
      
      void SetPose(const Pose3d& newPose) {pose_ = newPose;}
      
      // Return true if this object is the same as the other. Sub-classes can
      // overload this function to provide for rotational ambiguity when
      // comparing, e.g. for cubes or other blocks.
      virtual bool IsSameAs(const ObservableObject&  otherObject,
                            const float   distThreshold,
                            const Radians angleThreshold,
                            Pose3d& P_diff) const;
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const = 0;
      
      // For creating derived objects from a pointer to this base class, see
      // ObservableObjectBase below
      virtual ObservableObject* Clone() const = 0;
      
    protected:
      
      //static const std::vector<RotationMatrix3d> rotationAmbiguities_;
      
      ObjectID_t ID_;
      
      // Using a list here so that adding new markers does not affect references
      // to pre-existing markers
      std::list<KnownMarker> markers_;
      
      // Holds a LUT (by code) to all the markers of this object which have that
      // code.
      std::map<Marker::Code, std::vector<const KnownMarker*> > markersWithCode_;
      
      Pose3d pose_;
      
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
    };
    
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
      // to this library will remain.
      void CreateObjectsFromMarkers(std::list<ObservedMarker>& markers,
                                    std::vector<ObservableObject*>& objectsSeen) const;
      
      const ObservableObject* GetObjectWithID(const ObjectID_t ID) const;
      
      // Return a pointer to a vector of pointers to known objects with at
      // least one of the specified markers or codes on it. If  there is no
      // object with that marker/code, a NULL pointer is returned.
      std::vector<const ObservableObject*> const* GetObjectsWithMarker(const Marker& marker) const;
      std::vector<const ObservableObject*> const* GetObjectsWithCode(const Marker::Code& code) const;
      
    protected:
      
      //std::list<const ObservableObject*> knownObjects_;
      std::map<ObjectID_t, const ObservableObject*> knownObjects_;
      
      // Store a list of pointers to all objects that have at least one marker
      // with that code.  You can then use the objects' GetMarkersWithCode()
      // method to get the list of markers on each object.
      std::map<Marker::Code, std::vector<const ObservableObject*> > objectsWithCode_;
      
      /*
      struct MarkerMatch {
        const Vision::ObservedMarker* obsMarker;
        const Vision::KnownMarker*    libMarker;
        
        MarkerMatch(const Vision::ObservedMarker* obs=NULL,
                    const Vision::KnownMarker*    lib=NULL)
        : obsMarker(obs), libMarker(lib)
        {
          
        }
      };
      typedef std::pair<MarkerMatch, Pose3d> MatchWithPose;
      */
   
      /*
      typedef std::pair<const KnownMarker*, Pose3d> KnownMarkerPosePair;
      
      // Pairing of an observed marker with all the known markers it matches
      // and the poses those matches imply
      typedef std::pair<const ObservedMarker*, std::list< KnownMarkerPosePair > > MatchedMarkerPosesPair;
      */

      
      // Pairing of a pose and the list of matches which implied it (e.g.,
      // after clustering a list of PoseMatchPairs by pose
      typedef std::pair<Pose3d, std::list<MarkerMatch> > PoseMatchListPair;
      
      // A PoseCluster is a pairing of a single pose and all the marker matches
      // that imply that pose
      //typedef std::pair<Pose3d, std::list<MarkerMatch> > PoseCluster;
      //typedef std::pair<Pose3d, std::list<std::pair<const ObservedMarker*, KnownMarker> > > PoseCluster;
      class PoseCluster
      {
      public:
        PoseCluster(const PoseMatchPair& match);
        
        // Returns true if match was added
        bool TryToAddMatch(const PoseMatchPair& match,
                           const float distThreshold, const Radians angleThreshold,
                           const std::vector<RotationMatrix3d>& R_ambiguities);
        
        const Pose3d& GetPose() const { return pose_; }
        const size_t  GetSize() const { return matches_.size(); }
        
        // Updates pose based on all member matches. Does nothing if there is
        // only one member.
        void RecomputePose();
        
      protected:
        Pose3d pose_;
        
        std::list<std::pair<const ObservedMarker*, KnownMarker> > matches_;
      };
      
      /*
      void ComputeObjectPoses(const ObservedMarker* obsMarker,
                              const ObservableObject* libObject,
                              std::vector<MatchWithPose>& objPoses) const;
      
      void GroupPosesIntoObjects(const std::list  objPoses,
                                 const ObservableObject*                libObject,
                                 std::vector<ObservableObject*>&        objectsSeen) const;
      
      void ClusterObjectPoses(const std::list<PoseMatchPair>& individualPoses,
                              const f32 distThreshold, const Radians angleThreshold,
                              std::list<PoseMatchListPair>& poseClusters) const;
    */
      void ClusterObjectPoses(const std::vector<PoseMatchPair>& possiblePoses,
                              const ObservableObject*         libObject,
                              const float distThreshold, const Radians angleThreshold,
                              std::vector<PoseCluster>& poseClusters) const;
      // One-to-one mapping for looking up a known Marker by its code
      //std::map<Marker::Code, Marker> knownMarkers_;
      
    }; // class ObservableObjectLibrary
    
    
    /* NEEDED?
    // Specialization of an ObservableObject that exists in a fixed, known
    // location relative to the world (or some other object?)
    class ObservableObjectFixed : public ObservableObject
    {
    public:
      ObservableObjectFixed(const Pose3d& fixedPose);
      
    }; // class ObservableObjectFixed
    */
    
  } // namespace Vision
} // namespace Anki

#endif // ANKI_VISION_OBSERVABLE_OBJECT_H