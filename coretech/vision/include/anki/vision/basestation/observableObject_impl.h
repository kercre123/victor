/**
 * File: observableObject_impl.h
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Implements an abstract ObservableObject class, which is an
 *              general 3D object, with type, ID, and pose, and a set of Markers
 *              on it at known locations.  Thus, it is "observable" by virtue of
 *              having these markers, and its 3D / 6DoF pose can be estimated by
 *              matching up ObservedMarkers with the KnownMarkers it possesses.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef __Anki_Vision_ObservableObject_H__
#define __Anki_Vision_ObservableObject_H__

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/observableObject.h"

#include "anki/common/basestation/exceptions.h"

#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

// Ignore rotation completely when calling IsSameAs()? If so, just match position
// (translation) and block type.
#define IGNORE_ROTATION_FOR_IS_SAME_AS 1

namespace Anki {
namespace Vision {
  
  template<typename FAMILY, typename TYPE>
  ObservableObject<FAMILY,TYPE>::ObservableObject(FAMILY family, TYPE type)
  : _type(type)
  , _family(family)
  , _lastObservedTime(0)
  , _numTimesObserved(0)
  {
    
  }
  
  template<typename FAMILY, typename TYPE>
  bool ObservableObject<FAMILY,TYPE>::IsVisibleFrom(const Camera &camera,
                                       const f32 maxFaceNormalAngle,
                                       const f32 minMarkerImageSize,
                                       const bool requireSomethingBehind,
                                       const u16     xBorderPad,
                                       const u16     yBorderPad) const
  {
    // Return true if any of this object's markers are visible from the
    // given camera
    for(auto & marker : _markers) {
      if(marker.IsVisibleFrom(camera, maxFaceNormalAngle, minMarkerImageSize,
                              requireSomethingBehind, xBorderPad, yBorderPad)) {
        return true;
      }
    }
    
    return false;
    
  } // ObservableObject<FAMILY,TYPE>::IsObservableBy()
  
  template<typename FAMILY, typename TYPE>
  Vision::KnownMarker const& ObservableObject<FAMILY,TYPE>::AddMarker(const Marker::Code&  withCode,
                                                         const Pose3d&        atPose,
                                                         const f32            size_mm)
  {
    // Copy the pose and set this object's pose as its parent
    Pose3d poseCopy(atPose);
    poseCopy.SetParent(&_pose);
    
    // Construct a marker at that pose and store it keyed by its code
    _markers.emplace_back(withCode, poseCopy, size_mm);
    _markersWithCode[withCode].push_back(&_markers.back());
    
    return _markers.back();
  } // ObservableObject<FAMILY,TYPE>::AddMarker()
  
  template<typename FAMILY, typename TYPE>
  std::vector<KnownMarker*> const& ObservableObject<FAMILY,TYPE>::GetMarkersWithCode(const Marker::Code& withCode) const
  {
    auto returnVec = _markersWithCode.find(withCode);
    if(returnVec != _markersWithCode.end()) {
      return returnVec->second;
    }
    else {
      static const std::vector<KnownMarker*> EmptyMarkerVector(0);
      return EmptyMarkerVector;
    }
  } // ObservableObject<FAMILY,TYPE>::GetMarkersWithCode()
  
  template<typename FAMILY, typename TYPE>
  bool ObservableObject<FAMILY,TYPE>::IsSameAs(const ObservableObject& otherObject,
                                  const Point3f& distThreshold,
                                  const Radians& angleThreshold,
                                  Point3f& Tdiff,
                                  Radians& angleDiff) const
  {
    // The two objects can't be the same if they aren't the same type!
    bool isSame = this->GetType() == otherObject.GetType();
    
    if(isSame) {
      Pose3d otherPose;
      
      // Other object is this object's parent, leave otherPose as default
      // identity transformation and hook up parent connection.
      if(&otherObject.GetPose() == _pose.GetParent()) {
        otherPose.SetParent(&otherObject.GetPose());
      }
      
      else if(_pose.IsOrigin()) {
        // This object is an origin, GetParent() will be null, so we can't
        // dereference it below to make them have the same parent.  So try
        // to get other pose w.r.t. this origin.  If the other object is the
        // same as an origin pose (which itself is an identity transformation)
        // then the remaining transformation should be the identity.
        if(otherObject.GetPose().GetWithRespectTo(_pose, otherPose) == false) {
          PRINT_NAMED_WARNING("ObservableObject.IsSameAs.ObjectsHaveDifferentOrigins",
                              "Could not get other object w.r.t. this origin object. Returning isSame == false.\n");
          isSame = false;
        }
      }
      
      // Otherwise, attempt to make the two poses have the same parent so they
      // are comparable
      else if(otherObject.GetPose().GetWithRespectTo(*_pose.GetParent(), otherPose) == false) {
        PRINT_NAMED_WARNING("ObservableObject.IsSameAs.ObjectsHaveDifferentOrigins",
                            "Could not get other object w.r.t. this object's parent. Returning isSame == false.\n");
        isSame = false;
      }
      
      if(isSame) {
        
        CORETECH_ASSERT(otherPose.GetParent() == _pose.GetParent() ||
                        (_pose.IsOrigin() && otherPose.GetParent() == &_pose));
        
#         if IGNORE_ROTATION_FOR_IS_SAME_AS
        Point3f Tdiff(_pose.GetTranslation());
        Tdiff -= otherPose.GetTranslation();
        Tdiff = _pose.GetRotation() * Tdiff;
        Tdiff.Abs();
        isSame = Tdiff < distThreshold;
#         else
        if(this->GetRotationAmbiguities().empty()) {
          isSame = _pose.IsSameAs(otherPose, distThreshold, angleThreshold,
                                  Tdiff, angleDiff);
        }
        else {
          isSame = _pose.IsSameAs_WithAmbiguity(otherPose,
                                                this->GetRotationAmbiguities(),
                                                distThreshold, angleThreshold, true,
                                                Tdiff, angleDiff);
        } // if/else there are ambiguities
#         endif // IGNORE_ROTATION_FOR_IS_SAME_AS
        
      } // if(isSame) [inner]
    } // if(isSame) [outer]
    
    return isSame;
    
  } // ObservableObject<FAMILY,TYPE>::IsSameAs()

  template<typename FAMILY, typename TYPE>
  void ObservableObject<FAMILY,TYPE>::ComputePossiblePoses(const ObservedMarker*     obsMarker,
                                              std::vector<PoseMatchPair>& possiblePoses) const
  {
    auto matchingMarkers = _markersWithCode.find(obsMarker->GetCode());
    
    if(matchingMarkers != _markersWithCode.end()) {
      
      for(auto matchingMarker : matchingMarkers->second) {
        // Note that the known marker's pose, and thus its 3d corners, are
        // defined in the coordinate frame of parent object, so computing its
        // pose here _is_ the pose of the parent object.
        Pose3d markerPoseWrtCamera;
        Result result = matchingMarker->EstimateObservedPose(*obsMarker, markerPoseWrtCamera);
        if(result == RESULT_OK) {
          // Store the pose in the camera's coordinate frame, along with the pair
          // of markers that generated it
          possiblePoses.emplace_back(markerPoseWrtCamera,
                                     MarkerMatch(obsMarker, matchingMarker));
        } else {
          PRINT_NAMED_ERROR("ObservableObject.ComputePossiblePoses",
                            "Failed to estimate pose of observed marker.\n");
        }
      }
    }
    
  } // ComputePossiblePoses()

  /*
  Point3f ObservableObject<FAMILY,TYPE>::GetRotatedBoundingCube(const Pose3d &atPose)
  {
    // TODO: Rotate using quaternion
    Point3f cubeSize(atPose.GetRotation() * GetCanonicalBoundingCube());
    return cubeSize;
  }
   */
  
  template<typename FAMILY, typename TYPE>
  std::vector<RotationMatrix3d> const& ObservableObject<FAMILY,TYPE>::GetRotationAmbiguities() const
  {
    static const std::vector<RotationMatrix3d> RotationAmbiguities; // default is empty
    return RotationAmbiguities;
  }
  
  template<typename FAMILY, typename TYPE>
  void ObservableObject<FAMILY,TYPE>::GetCorners(std::vector<Point3f>& corners) const
  {
    GetCorners(_pose, corners);
  }

  template<typename FAMILY, typename TYPE>
  void ObservableObject<FAMILY,TYPE>::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const
  {
    atPose.ApplyTo(GetCanonicalCorners(), corners);
  }
  
  template<typename FAMILY, typename TYPE>
  void ObservableObject<FAMILY,TYPE>::GetObservedMarkers(std::vector<const KnownMarker*>& observedMarkers,
                                            const TimeStamp_t sinceTime) const
  {
    if(sinceTime > 0) {
      for(auto const& marker : this->_markers)
      {
        if(marker.GetLastObservedTime() >= sinceTime) {
          observedMarkers.push_back(&marker);
        }
      }
    }
  } // GetObservedMarkers()
  
  template<typename FAMILY, typename TYPE>
  Result ObservableObject<FAMILY,TYPE>::UpdateMarkerObservationTimes(const ObservableObject& otherObject)
  {
    if(otherObject.GetType() != this->GetType()) {
      PRINT_NAMED_ERROR("ObservableObject.UpdateMarkerObservationTimes.TypeMismatch",
                        "Tried to update the marker observations between dissimilar object types.\n");
      return RESULT_FAIL;
    }

    std::list<KnownMarker> const& otherMarkers = otherObject.GetMarkers();

    // If these objects are the same type they have to have the same number of
    // markers, by definition.
    CORETECH_ASSERT(otherMarkers.size() == _markers.size());
    
    std::list<KnownMarker>::const_iterator otherMarkerIter = otherMarkers.begin();
    std::list<KnownMarker>::iterator markerIter = _markers.begin();
    
    for(;otherMarkerIter != otherMarkers.end() && markerIter != _markers.end();
        ++otherMarkerIter, ++markerIter)
    {
      markerIter->SetLastObservedTime(std::max(markerIter->GetLastObservedTime(),
                                               otherMarkerIter->GetLastObservedTime()));
    }
    
    return RESULT_OK;
  }
  
  template<typename FAMILY, typename TYPE>
  void ObservableObject<FAMILY,TYPE>::SetMarkersAsObserved(const Marker::Code& withCode,
                                              const TimeStamp_t   atTime)
  {
    auto markers = _markersWithCode.find(withCode);
    if(markers != _markersWithCode.end()) {
      for(auto marker : markers->second) {
        marker->SetLastObservedTime(atTime);
      }
    }
    else {
      PRINT_NAMED_WARNING("ObservableObject.SetMarkersAsObserved",
                          "No markers found with code %d.\n",
                          withCode);
    }
    
  } // SetMarkersAsObserved()
  
  template<typename FAMILY, typename TYPE>
  void ObservableObject<FAMILY,TYPE>::SetMarkerAsObserved(const ObservedMarker* nearestTo,
                                             const TimeStamp_t     atTime,
                                             const f32             centroidDistThreshold,
                                             const f32             areaRatioThreshold)
  {
    // Find the marker that projects nearest to the observed one and update
    // its observed time. Only consider those with a matching code.
    Pose3d markerPoseWrtCamera;
    const Camera& camera = nearestTo->GetSeenBy();
    
    const Point2f obsCentroid = nearestTo->GetImageCorners().ComputeCentroid();
    const f32     invObsArea  = 1.f / nearestTo->GetImageCorners().ComputeArea();
    
    auto markers = _markersWithCode.find(nearestTo->GetCode());
    if(markers != _markersWithCode.end()) {
      
      f32 minCentroidDistSq = centroidDistThreshold*centroidDistThreshold;
      Vision::KnownMarker* whichMarker = nullptr;
      for(auto marker : markers->second)
      {
        if(true == marker->GetPose().GetWithRespectTo(camera.GetPose(), markerPoseWrtCamera))
        {
          Quad2f projPoints;
          camera.Project3dPoints(marker->Get3dCorners(markerPoseWrtCamera), projPoints);
          
          const Point2f knownCentroid = projPoints.ComputeCentroid();
          
          const f32 centroidDistSq = (knownCentroid - obsCentroid).LengthSq();
          if(centroidDistSq < minCentroidDistSq) {
            const f32 knownArea = projPoints.ComputeArea();
            if(NEAR(knownArea * invObsArea, 1.f, areaRatioThreshold)) {
              minCentroidDistSq = centroidDistSq;
              whichMarker = marker;
            }
          }
        }
      } // for each marker on this object
      
      if(whichMarker != nullptr) {
        whichMarker->SetLastObservedTime(atTime);
      } else {
        PRINT_NAMED_WARNING("ObservableObject.SetMarkerAsObserved",
                            "No markers found within specfied projected distance threshold (%f).\n",
                            centroidDistThreshold);
      }
    } else {
      PRINT_NAMED_WARNING("ObservableObject.SetMarkerAsObserved",
                          "No markers found with code (%d) of given 'nearestTo' observed marker.\n",
                          nearestTo->GetCode());
    }
  } // SetMarkerAsObserved()
  
  template<typename FAMILY, typename TYPE>
  Quad2f ObservableObject<FAMILY,TYPE>::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
  {
    const std::vector<Point3f>& canonicalCorners = GetCanonicalCorners();
    const RotationMatrix3d& R = atPose.GetRotationMatrix();
    
    std::vector<Point2f> points;
    points.reserve(canonicalCorners.size());
    for(auto corner : canonicalCorners) {
      
      // Move canonical point to correct (padded) size
      corner.x() += (signbit(corner.x()) ? -padding_mm : padding_mm);
      corner.y() += (signbit(corner.y()) ? -padding_mm : padding_mm);
      corner.z() += (signbit(corner.z()) ? -padding_mm : padding_mm);
      
      // Rotate to given pose
      corner = R*corner;
      
      // Project onto XY plane, i.e. just drop the Z coordinate
      points.emplace_back(corner.x(), corner.y());
    }
    
    Quad2f boundingQuad = GetBoundingQuad(points);
    
    // Re-center
    Point2f center(atPose.GetTranslation().x(), atPose.GetTranslation().y());
    boundingQuad += center;
    
    return boundingQuad;
  } // GetBoundingQuadXY()
  
#pragma mark -
#pragma mark Inlined Accessors
  
  template<typename FAMILY, typename TYPE>
  inline ObjectID ObservableObject<FAMILY,TYPE>::GetID() const {
    return _ID;
  }
  
  template<typename FAMILY, typename TYPE>
  inline TYPE ObservableObject<FAMILY,TYPE>::GetType() const {
    return _type;
  }
  
  template<typename FAMILY, typename TYPE>
  inline FAMILY ObservableObject<FAMILY,TYPE>::GetFamily() const {
    return _family;
  }
  
  template<typename FAMILY, typename TYPE>
  inline const ColorRGBA& ObservableObject<FAMILY,TYPE>::GetColor() const {
    return _color;
  }
  
  template<typename FAMILY, typename TYPE>
  inline const Pose3d& ObservableObject<FAMILY,TYPE>::GetPose() const {
    return _pose;
  }
  
  template<typename FAMILY, typename TYPE>
  inline void ObservableObject<FAMILY,TYPE>::SetID() { //const ObjectID newID) {
    _ID.Set();
  }
  
  template<typename FAMILY, typename TYPE>
  inline void ObservableObject<FAMILY,TYPE>::SetPose(const Pose3d& newPose) {
    _pose = newPose;
    
    std::string poseName(ObjectTypeToString(GetType()));
    if(poseName.empty()) {
      poseName = "Object";
    }
    poseName += "_" + std::to_string(GetID().GetValue());
    _pose.SetName(poseName);
  }
  
  template<typename FAMILY, typename TYPE>
  inline void ObservableObject<FAMILY,TYPE>::SetPoseParent(const Pose3d* newParent) {
    _pose.SetParent(newParent);
  }
  
  template<typename FAMILY, typename TYPE>
  inline bool ObservableObject<FAMILY,TYPE>::IsSameAs(const ObservableObject& otherObject) const {
    return IsSameAs(otherObject, this->GetSameDistanceTolerance(), this->GetSameAngleTolerance());
  }
  
  template<typename FAMILY, typename TYPE>
  inline void ObservableObject<FAMILY,TYPE>::SetColor(const Anki::ColorRGBA &color) {
    _color = color;
  }
  
  template<typename FAMILY, typename TYPE>
  inline void ObservableObject<FAMILY,TYPE>::Visualize() {
    Visualize(_color);
  }
  
  template<typename FAMILY, typename TYPE>
  inline Quad2f ObservableObject<FAMILY,TYPE>::GetBoundingQuadXY(const f32 padding_mm) const
  {
    return GetBoundingQuadXY(_pose, padding_mm);
  }
  
  template<typename FAMILY, typename TYPE>
  inline Point3f ObservableObject<FAMILY,TYPE>::GetSameDistanceTolerance() const {
    // TODO: This is only ok because we're only using totally symmetric 1x1x1 blocks at the moment.
    //       The proper way to do the IsSameAs check is to have objects return a scaled down bounding box of themselves
    //       and see if the the origin of the candidate object is within it.
    Point3f distTol(GetSize() * DEFAULT_SAME_DIST_TOL_FRACTION);
    return distTol;
  }
  
  template<typename FAMILY, typename TYPE>
  inline bool ObservableObject<FAMILY,TYPE>::IsSameAs(const ObservableObject& otherObject,
                                                      const Point3f& distThreshold,
                                                      const Radians& angleThreshold) const
  {
    Point3f Tdiff;
    Radians angleDiff;
    return IsSameAs(otherObject, distThreshold, angleThreshold,
                    Tdiff, angleDiff);
  }
  
  template<typename FAMILY, typename TYPE>
  inline const TimeStamp_t ObservableObject<FAMILY,TYPE>::GetLastObservedTime() const {
    return _lastObservedTime;
  }
  
  template<typename FAMILY, typename TYPE>
  inline u32 ObservableObject<FAMILY,TYPE>::GetNumTimesObserved() const {
    return _numTimesObserved;
  }
  
  template<typename FAMILY, typename TYPE>
  inline void ObservableObject<FAMILY,TYPE>::SetLastObservedTime(TimeStamp_t t) {
    _lastObservedTime = t;
    ++_numTimesObserved;
  }
  
  template<typename FAMILY, typename TYPE>
  inline void ObservableObject<FAMILY,TYPE>::GetObservedMarkers(std::vector<const KnownMarker*>& observedMarkers) const {
    GetObservedMarkers(observedMarkers, GetLastObservedTime());
  }
  

  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_ObservableObject_H__

