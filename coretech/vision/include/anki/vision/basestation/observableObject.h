#ifndef ANKI_VISION_OBSERVABLE_OBJECT_H
#define ANKI_VISION_OBSERVABLE_OBJECT_H

#include <list>

#include "anki/vision/basestation/visionMarker.h"

namespace Anki {
  namespace Vision {
    
    // Forward declaration
    class Camera;
    
    class ObservableObject
    {
    public:
      // Do we want to be req'd to instantiate with all codes up front?
      ObservableObject(){};
      
      ObservableObject(ObjectID_t ID);
      
      ObservableObject(const std::vector<std::pair<Marker::Code,Pose3d> >& codesAndPoses);
      
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
      
      // Estimate the pose of this object, relative to the camera, given that
      // we saw the specified corners and code in the specified locations
      // in that camera's image.
      void UpdatePose(const Quad2f& imgCorners, const Marker::Code& code,
                      const Camera& camera);
      
      // Accessors:
      ObjectID_t    GetID() const {return ID_;}
      virtual float GetMinDim() const = 0;
      const Pose3d& GetPose() const {return pose_;}
      
      void SetPose(const Pose3d& newPose) {pose_ = newPose;}
      
      // For creating derived objects from a pointer to this base class, see
      // ObservableObjectBase below
      virtual ObservableObject* clone() const = 0;
      
    protected:
      
      ObjectID_t ID_;
      
      // Using a list here so that adding new markers does not affect references
      // to pre-existing markers
      std::list<KnownMarker> markers_;
      
      // Holds a LUT (by code) to all the markers of this object which have that
      // code.
      std::map<Marker::Code, std::vector<const KnownMarker*> > markersWithCode_;
      
      Pose3d pose_;
      
    };
    
    
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
                                    std::vector<ObservableObject*> objectsSeen) const;
      
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
      
      void ComputeObjectPoses(const ObservedMarker* obsMarker,
                              const ObservableObject* libObject,
                              std::vector<MatchWithPose>& objPoses) const;
      
      void GroupPosesIntoObjects(const std::vector<std::vector<MatchWithPose> >&  objPoses,
                                 const ObservableObject*                libObject,
                                 std::vector<ObservableObject*>&        objectsSeen) const;
      
      void ClusterObjectPoses(const std::vector< std::vector<MatchWithPose> >& objPoses,
                              const f32 distThreshold, const Radians angleThreshold,
                              std::vector<std::vector<MatchWithPose> >& markerClusters) const;
    
      
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