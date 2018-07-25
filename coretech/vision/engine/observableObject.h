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

#ifndef ANKI_VISION_OBSERVABLE_OBJECT_H
#define ANKI_VISION_OBSERVABLE_OBJECT_H

#include <list>
#include <set>

#include "coretech/vision/engine/visionMarker.h"

#include "coretech/common/engine/objectIDs.h"

#include "coretech/common/engine/colorRGBA.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/pose.h"

#include "clad/types/poseStructs.h"


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
      ObservableObject();
      
      // For creating a fresh new derived object from a pointer to this base class.
      // NOTE: This just creates a fresh object, and does not copy the original
      //  object's pose, ID, or other state.
      virtual ObservableObject* CloneType() const = 0;
      
      virtual ~ObservableObject(){};
      
      // Specify the extistence of a marker with the given code at the given
      // pose (relative to the object's origin), and the specfied size in mm
      Vision::KnownMarker const& AddMarker(const Marker::Code& withCode,
                                           const Pose3d&       atPose,
                                           const Point2f&      size_mm);
      
      std::list<KnownMarker> const& GetMarkers() const {return _markers;}
      
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
      
      // Same as above, but uses GetLastObservedTime() as "sinceTime"
      void GetObservedMarkers(std::vector<const KnownMarker*>& observedMarkers) const;

      // Get the pose of the marker closest to the reference pose, optionally using
      // X and Y only (when ignoreZ=true). If no marker is found due to mismatched origins
      // match, then RESULT_FAIL_ORIGIN_MISMATCH is returned.
      Result GetClosestMarkerPose(const Pose3d& referencePose, const bool ignoreZ,
                                  Pose3d& closestPoseWrtReference) const;
      Result GetClosestMarkerPose(const Pose3d& referencePose, const bool ignoreZ,
                                  Pose3d& closestPoseWrtReference,
                                  Marker& closestMarker) const;
      
      // Updates the observation times of this object's markers with the newer
      // of the current times and the times of the corresponding markers on the
      // other object. If the other object is not the same type, RESULT_FAIL
      // is returned.
      Result UpdateMarkerObservationTimes(const ObservableObject& otherObject);
      
      // "Active" objects are those that can be communicated with and have lights, accelerometers, etc.
      virtual bool IsActive() const = 0;
      
      // If object is moving, returns true and the time that it started moving in t.
      // If not moving, returns false and the time that it stopped moving in t.
      virtual bool IsMoving(TimeStamp_t* t = nullptr) const { return false; }
      
      // Set the moving state of the object and when it either started or stopped moving.
      virtual void SetIsMoving(bool isMoving, TimeStamp_t t) {};
      
      // Override for objects that can be used for localization (e.g., mats
      // or active blocks that have not moved since last localization)
      // Note that true means the object can be used for localization *now*, in its current state,
      // (not whether this object is of a type that might ever be suitable for localization).
      virtual bool CanBeUsedForLocalization() const { return false; }
      
      // How flat an object must be to be used for localization (override if different
      // objects have different tolerances)
      virtual f32 GetRestingFlatTolForLocalization_deg() const { return 5.f; }

      virtual bool IsMoveable()               const { return true; }
      
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
      
      // Set the marker whose centroid projects closest to the observed marker's
      // centroid (within threshold distance and within areaRatio threshold)
      // to have the given observed time.
      void SetMarkerAsObserved(const ObservedMarker* nearestTo,
                               const TimeStamp_t     atTime,
                               const f32             areaRatioThreshold = 0.1f);  // i.e., 1 - abs(obsArea/knownArea) < threshold
      
      // Return true if any of the object's markers is visible from the given
      // camera. See also KnownMarker::IsVisibleFrom().
      bool IsVisibleFrom(const Camera& camera,
                         const f32     maxFaceNormalAngle,
                         const f32     minMarkerImageSize,
                         const bool    requireSomethingBehind,
                         const u16     xBorderPad = 0,
                         const u16     yBorderPad = 0) const;
      
      // Same as above, except requireSomethingBehind==true.
      // If the function returns false, then hasNothingBehind contains whether
      // or not any marker had nothing behind it. If so, then we would have
      // returned true for this function if not for that reason.
      bool IsVisibleFrom(const Camera& camera,
                         const f32     maxFaceNormalAngle,
                         const f32     minMarkerImageSize,
                         const u16     xBorderPad,
                         const u16     yBorderPad,
                         bool& hasNothingBehind) const;

      // Similar to the above, but return the highest NotVisibleReason that was returned for any of the
      // markers (the latest check we got through for any one marker).
      KnownMarker::NotVisibleReason IsVisibleFromWithReason(const Camera& camera,
                                                            const f32     maxFaceNormalAngle,
                                                            const f32     minMarkerImageSize,
                                                            const bool    requireSomethingBehind,
                                                            const u16     xBorderPad = 0,
                                                            const u16     yBorderPad = 0) const;
      
      // Accessors:
      const ObjectID&    GetID()     const;
      const Pose3d&      GetPose()   const;
      const ColorRGBA&   GetColor()  const;
      
      //virtual float GetMinDim() const = 0;
      
      // Auto-set ID to unique value
      virtual void SetID();
      
      // For special situations where automatic unique ID is not desired, use this
      // method to deliberately copy the ID from another object
      void CopyID(const ObservableObject* fromOther);
            
      void SetColor(const ColorRGBA& color);
      virtual void SetPose(const Pose3d& newPose,
                           f32 fromDistance = -1.f,
                           PoseState newPoseState = PoseState::Known);
      
      // Returns last "fromDistance" supplied to SetPose(), or -1 if none set.
      f32 GetLastPoseUpdateDistance() const;
      
      void SetLastObservedTime(TimeStamp_t t);
      const TimeStamp_t GetLastObservedTime() const;
      
      // Copy observation times from another object, keeping the max counts / latest times
      void SetObservationTimes(const ObservableObject* otherObject);
      
      // Return true if this object is the same as the other. Sub-classes can
      // overload this function to provide for rotational ambiguity when
      // comparing, e.g. for cubes or other blocks.
      bool IsSameAs(const ObservableObject&  otherObject,
                    const Point3f&  distThreshold,
                    const Radians&  angleThreshold) const;
      
      // Same as above, but returns translational and angular difference
      bool IsSameAs(const ObservableObject& otherObject,
                    const Point3f& distThreshold,
                    const Radians& angleThreshold,
                    Point3f& Tdiff,
                    Radians& angleDiff) const;
      
      // Same as other IsSameAs() calls above, except the tolerance thresholds
      // given by the virtual members below are used (so that per-object-class
      // thresholds can be defined)
      bool IsSameAs(const ObservableObject& otherObject) const;
      
      // Return the dimensions of the object's bounding cube in its canonical pose
      virtual const Point3f& GetSize() const = 0;
      
      // Return the size of the object along the axes of the input pose's
      // _parent_ coordinate frame. Useful for reasoning about the size of a
      // rotated object in the world frame, as opposed to its canonically-oriented
      // size as GetSize() above does. Note that atPose is assumed to put the object
      // in a roughly axis-aligned orientation (resting "flat").
      template<char AXIS>
      f32 GetDimInParentFrame(const Pose3d& atPose) const;
      template<char AXIS>
      f32 GetDimInParentFrame() const; // use object's current pose w.r.t. origin
      
      // Same as above, but does all three axes together
      Point3f GetSizeInParentFrame(const Pose3d& atPose) const;
      Point3f GetSizeInParentFrame() const; // use object's current pose w.r.t. origin
      
      // Useful for doing calculations relative to the object which are agnostic
      // of the object's x/y rotation.  Relative to _pose it has:
      // - The same x/y translation relative to the origin
      // - A Z-translation moved from the object's center up along the origin's z axis. The
      //   amount moved up is the given fraction of the object's height in the origin's z axis.
      //   (So 0 = object center, 0.5 = top of object (common case), -0.5 = btm of object)
      // - x/y rotation of 0 and z rotation equal to _poses's rotation relative to origin
      Pose3d GetZRotatedPointAboveObjectCenter(f32 heightFraction) const;
      
      /*
      // Return the bounding cube dimensions in the specified pose (i.e., apply the
      // rotation of the given pose to the canonical cube). If no pose is supplied,
      // the object's current pose is used.
      virtual Point3f GetRotatedBoundingCube(const Pose3d& atPose) const = 0;
      Point3f GetRotatedBoundingCube() const { return GetRotatedBoundingCube(GetPose()); }
      */
      
      // Return the same-distance tolerance to use in the X/Y/Z dimensions.
      // The default implementation simply uses the canonical bounding cube,
      // scaled by the following tolerance fraction.
      constexpr static f32 DEFAULT_SAME_DIST_TOL_FRACTION = 0.8f; // fraction of GetSize() to use
      virtual Point3f GetSameDistanceTolerance() const;
      
      // Return the same angle tolerance for matching. Default is 45 degrees.
      virtual Radians GetSameAngleTolerance() const { return DEG_TO_RAD(45); }
      
      virtual RotationAmbiguities const& GetRotationAmbiguities() const;
      
      virtual void GetCorners(std::vector<Point3f>& corners) const;
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const;
      
      virtual void Visualize() const; // using internal ColorRGBA
      virtual void Visualize(const ColorRGBA& color) const = 0; // using specified color
      virtual void EraseVisualization() const = 0;
      
      Quad2f GetBoundingQuadXY(const f32 padding_mm = 0.f) const;
      virtual Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm = 0.f) const;
      
      // Check whether the object's current pose is resting flat on one of its
      // sides (within the given tolerance). This means it is allowed to have any
      // orientation around the Z axis (w.r.t. its parent), but no rotation about the
      // X and Y axes.
      bool IsRestingFlat(const Radians& angleTol = DEG_TO_RAD(10)) const;
      
      // If the pose is within angleTol of being "flat" clamp it to exactly flat. Return true if clamping occurred
      static bool ClampPoseToFlat(Pose3d& pose, const Radians& angleTol);

      PoseState GetPoseState() const { return _poseState; } // TODO Remove in favor of concepts
      void SetPoseState(PoseState newState) { _poseState = newState; }
      bool IsPoseStateKnown() const { return _poseState == PoseState::Known; }      
      
      // in general clients should not need to check for this, but it's public for unit tests and asserts
      inline bool HasValidPose() const;
      inline static bool IsValidPoseState(const PoseState poseState);

      static const char* PoseStateToString(const PoseState& state);
      
      //Check if the bottom of an object is resting at a certain height within a tolerance
      bool IsRestingAtHeight(float height, float tolerance) const;
      
      // Helper to check whether a pose is too high above an object:
      // True if pose.z is more than heightTol above (object.z + (heightMultiplier*object.height)),
      // where the object.height is its height relative to the pose's parent frame.
      // If offsetFraction != 0, then (pose.z + object.height*offsetFraction) is used.
      // This is useful for comparing the top or bottom of the object (by using
      // offsetFraction=0.5 or -0.5, respectively).
      bool IsPoseTooHigh(const Pose3d& poseWrtRobot, float heightMultiplier,
                         float heightTol, float offsetFraction) const;
      
      // Canonical corners are properties of each derived class and define the
      // objects' shape, in a canonical (unrotated, untranslated) state.
      virtual const std::vector<Point3f>& GetCanonicalCorners() const = 0;
      
    private:
      // NOTE: Declaring pose _first_ because markers use it as a parent and
      //       therefore it needs to be destroyed _after_ the markers whose
      //       poses refer to it to avoid triggering ownership assertions.
      
      // Force setting of pose through SetPose() to keep pose name updated
      Pose3d _pose;
      f32    _lastSetPoseDistance = -1.f;
      
      // Don't allow a copy constuctor, because it won't handle fixing the
      // marker pointers and pose parents
      //ObservableObject(const ObservableObject& other);
      
    protected:
      
      
      ObjectID     _ID;
      TimeStamp_t  _lastObservedTime = 0;
      ColorRGBA    _color;
      PoseState    _poseState = PoseState::Invalid;
      
      // Using a list here so that adding new markers does not affect references
      // to pre-existing markers
      std::list<KnownMarker> _markers;
      
      // Holds a LUT (by code) to all the markers of this object which have that
      // code.
      std::map<Marker::Code, std::vector<KnownMarker*> > _markersWithCode;
      
      // For subclasses can get a modifiable pose reference
      Pose3d& GetNonConstPose() { return _pose; }
      
      // Helper for GetDimInParentFraem / GetSizeInParentFrame
      template<char AXIS>
      f32 GetDimInParentFrame(const RotationMatrix3d& Rmat) const;
    };
  
    
    //
    // Inline accessors
    //
    
    inline const ObjectID& ObservableObject::GetID() const {
      return _ID;
    }
    
    inline const ColorRGBA& ObservableObject::GetColor() const {
      return _color;
    }
    
    inline void ObservableObject::SetID() { //const ObjectID newID) {
      _ID.Set();
    }
    
    inline void ObservableObject::CopyID(const ObservableObject* other) {
      _ID = other->GetID();
    }
  
    inline void ObservableObject::SetPose(const Pose3d& newPose, f32 fromDistance, PoseState newPoseState) {
      _pose = newPose;
      
      SetPoseState(newPoseState);
      
      std::string poseName("Object");
      poseName += "_" + std::to_string(GetID().GetValue());
      _pose.SetName(poseName);
      
      if(fromDistance >= 0.f) {
        _lastSetPoseDistance = fromDistance;
      }
    }
    
    inline f32 ObservableObject::GetLastPoseUpdateDistance() const {
      return _lastSetPoseDistance;
    }
    
    inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject) const {
      return IsSameAs(otherObject, this->GetSameDistanceTolerance(), this->GetSameAngleTolerance());
    }
    
    inline void ObservableObject::SetColor(const Anki::ColorRGBA &color) {
      _color = color;
    }
    
    inline void ObservableObject::Visualize() const {
      Visualize(_color);
    }
    
    inline Quad2f ObservableObject::GetBoundingQuadXY(const f32 padding_mm) const
    {
      return GetBoundingQuadXY(_pose, padding_mm);
    }
    
    inline Point3f ObservableObject::GetSameDistanceTolerance() const {
      // TODO: This is only ok because we're only using totally symmetric 1x1x1 blocks at the moment.
      //       The proper way to do the IsSameAs check is to have objects return a scaled down bounding box of themselves
      //       and see if the the origin of the candidate object is within it. COZMO-9440
      Point3f distTol(GetSize() * DEFAULT_SAME_DIST_TOL_FRACTION);
      return distTol;
    }
    
    inline bool ObservableObject::IsSameAs(const ObservableObject& otherObject,
                                           const Point3f& distThreshold,
                                           const Radians& angleThreshold) const
    {
      Point3f Tdiff;
      Radians angleDiff;
      return IsSameAs(otherObject, distThreshold, angleThreshold,
                      Tdiff, angleDiff);
    }
    
    inline const TimeStamp_t ObservableObject::GetLastObservedTime() const {
      return _lastObservedTime;
    }
    
    inline void ObservableObject::SetLastObservedTime(TimeStamp_t t) {
      _lastObservedTime = t;
    }

    inline void ObservableObject::GetObservedMarkers(std::vector<const KnownMarker*>& observedMarkers) const {
      GetObservedMarkers(observedMarkers, GetLastObservedTime());
    }
    
    inline bool ObservableObject::HasValidPose() const {
      return _poseState != PoseState::Invalid;
    }
    
    inline bool ObservableObject::IsValidPoseState(const PoseState poseState) {
      return poseState != PoseState::Invalid;
    }
	
    inline Point3f ObservableObject::GetSizeInParentFrame(const Pose3d& atPose) const
    {
      const RotationMatrix3d& Rmat = atPose.GetRotation().GetRotationMatrix();
      Point3f rotatedSize(GetDimInParentFrame<'X'>(Rmat),
                          GetDimInParentFrame<'Y'>(Rmat),
                          GetDimInParentFrame<'Z'>(Rmat));
      return rotatedSize;
    }
    
    inline Point3f ObservableObject::GetSizeInParentFrame() const
    {
      return GetSizeInParentFrame(GetPose().GetWithRespectToRoot());
    }
    
    template<char AXIS>
    f32 ObservableObject::GetDimInParentFrame(const RotationMatrix3d& Rmat) const
    {
      const AxisName axis = Rmat.GetRotatedParentAxis<AXIS>();
      switch(axis)
      {
        case AxisName::X_POS:
        case AxisName::X_NEG:
          return GetSize().x();
          
        case AxisName::Y_POS:
        case AxisName::Y_NEG:
          return GetSize().y();
          
        case AxisName::Z_POS:
        case AxisName::Z_NEG:
          return GetSize().z();
      }
    }
    
    template<char AXIS>
    inline f32 ObservableObject::GetDimInParentFrame(const Pose3d& atPose) const
    {
      const RotationMatrix3d& Rmat = atPose.GetRotation().GetRotationMatrix();
      const f32 dim = GetDimInParentFrame<AXIS>(Rmat);
      return dim;
    }
    
    template<char AXIS>
    inline f32 ObservableObject::GetDimInParentFrame() const
    {
      return GetDimInParentFrame<AXIS>(GetPose().GetWithRespectToRoot());
    }
    
  } // namespace Vision
} // namespace Anki

#endif // ANKI_VISION_OBSERVABLE_OBJECT_H
