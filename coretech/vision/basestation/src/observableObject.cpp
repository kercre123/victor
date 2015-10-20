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
  
  ObservableObject::ObservableObject()
  {
    
  }
  
  void ObservableObject::Delocalize()
  {
    PRINT_NAMED_INFO("ObservableObject.DeLocalize", "Delocalizing object %d.", GetID().GetValue());
    _isLocalized = false;
  }

  
  bool ObservableObject::IsVisibleFrom(const Camera &camera,
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
    
  } // ObservableObject::IsObservableBy()
  
  
  Vision::KnownMarker const& ObservableObject::AddMarker(const Marker::Code&  withCode,
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
  } // ObservableObject::AddMarker()
  
  
  std::vector<KnownMarker*> const& ObservableObject::GetMarkersWithCode(const Marker::Code& withCode) const
  {
    auto returnVec = _markersWithCode.find(withCode);
    if(returnVec != _markersWithCode.end()) {
      return returnVec->second;
    }
    else {
      static const std::vector<KnownMarker*> EmptyMarkerVector(0);
      return EmptyMarkerVector;
    }
  } // ObservableObject::GetMarkersWithCode()
  
  
  bool ObservableObject::IsSameAs(const ObservableObject& otherObject,
                                  const Point3f& distThreshold,
                                  const Radians& angleThreshold,
                                  Point3f& Tdiff,
                                  Radians& angleDiff) const
  {
    bool isSame = true;
    
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
      // If this fails, the objects must not have the same origin, so we cannot
      // determine sameness based only on pose.
      
      // No need to warn: this can easily happen after the robot gets kidnapped
      //  PRINT_NAMED_WARNING("ObservableObject.IsSameAs.ObjectsHaveDifferentOrigins",
      //                      "Could not get other object w.r.t. this object's parent. Returning isSame == false.\n");
      
      isSame = false;
    }
    
    if(isSame) {
      
      CORETECH_ASSERT(otherPose.GetParent() == _pose.GetParent() ||
                      (_pose.IsOrigin() && otherPose.GetParent() == &_pose));
      
#     if IGNORE_ROTATION_FOR_IS_SAME_AS
      Point3f Tdiff(_pose.GetTranslation());
      Tdiff -= otherPose.GetTranslation();
      Tdiff = _pose.GetRotation() * Tdiff;
      Tdiff.Abs();
      isSame = Tdiff < distThreshold;
#     else
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
#     endif // IGNORE_ROTATION_FOR_IS_SAME_AS
      
    } // if(isSame)
    
    return isSame;
    
  } // ObservableObject::IsSameAs()

  
  void ObservableObject::ComputePossiblePoses(const ObservedMarker*     obsMarker,
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
  Point3f ObservableObject::GetRotatedBoundingCube(const Pose3d &atPose)
  {
    // TODO: Rotate using quaternion
    Point3f cubeSize(atPose.GetRotation() * GetCanonicalBoundingCube());
    return cubeSize;
  }
   */
  
  
  std::vector<RotationMatrix3d> const& ObservableObject::GetRotationAmbiguities() const
  {
    static const std::vector<RotationMatrix3d> RotationAmbiguities; // default is empty
    return RotationAmbiguities;
  }
  
  
  void ObservableObject::GetCorners(std::vector<Point3f>& corners) const
  {
    GetCorners(_pose, corners);
  }

  
  void ObservableObject::GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const
  {
    atPose.ApplyTo(GetCanonicalCorners(), corners);
  }
  
  
  void ObservableObject::GetObservedMarkers(std::vector<const KnownMarker*>& observedMarkers,
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
  
  
  Result ObservableObject::UpdateMarkerObservationTimes(const ObservableObject& otherObject)
  {
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
  
  
  void ObservableObject::SetMarkersAsObserved(const Marker::Code& withCode,
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
  
  
  void ObservableObject::SetMarkerAsObserved(const ObservedMarker* nearestTo,
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
  
  
  Quad2f ObservableObject::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
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
  
  bool ObservableObject::IsRestingFlat(const Radians& angleTol) const
  {
    bool isFlat = (NEAR(GetPose().GetRotationAngle<'X'>(), 0, angleTol) &&
                   NEAR(GetPose().GetRotationAngle<'Y'>(), 0, angleTol));
    
    return isFlat;
  }
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_ObservableObject_H__

