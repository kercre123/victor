/**
 * File: observableObject.cpp
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

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/observableObject.h"

#include "coretech/common/engine/exceptions.h"

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/quad_impl.h"

// Ignore rotation completely when calling IsSameAs()? If so, just match position
// (translation) and block type.
#define IGNORE_ROTATION_FOR_IS_SAME_AS 1

// verbose output for each IsVisible check
#define DEBUG_PRINT_NOT_VISIBLE_REASONS 0

namespace Anki {
namespace Vision {

  ObservableObject::ObservableObject()
  {
    
  }

  bool ObservableObject::IsVisibleFrom(const Camera &camera,
                                       const f32 maxFaceNormalAngle,
                                       const f32 minMarkerImageSize,
                                       const bool requireSomethingBehind,
                                       const u16     xBorderPad,
                                       const u16     yBorderPad) const
  {
    if(!camera.IsCalibrated())
    {
      // Can't do visibility checks with an uncalibrated camera
      PRINT_NAMED_WARNING("ObservableObject.IsVisibleFrom.CameraNotCalibrated", "");
      return false;
    }
 
    // Return true if any of this object's markers are visible from the
    // given camera
    for(auto const& marker : _markers) {
      KnownMarker::NotVisibleReason reason = KnownMarker::NotVisibleReason::IS_VISIBLE;
      if(marker.IsVisibleFrom(camera, maxFaceNormalAngle, minMarkerImageSize,
                              requireSomethingBehind, xBorderPad, yBorderPad, reason)) {
        if( DEBUG_PRINT_NOT_VISIBLE_REASONS ) {
          PRINT_NAMED_DEBUG("ObservableObject.IsVisibleFrom.MarkerVisible",
                            "marker '%s' is visible",
                            marker.GetCodeName());
        }
        return true;
      }
      else if( DEBUG_PRINT_NOT_VISIBLE_REASONS ) {
        PRINT_NAMED_DEBUG("ObservableObject.IsVisibleFrom.MarkerNotVisible",
                          "marker '%s' is not visible for reason '%s'",
                          marker.GetCodeName(),
                          NotVisibleReasonToString(reason));
      }
    }
    
    return false;
    
  } // ObservableObject::IsObservableBy()
  
  bool ObservableObject::IsVisibleFrom(const Camera& camera,
                                       const f32     maxFaceNormalAngle,
                                       const f32     minMarkerImageSize,
                                       const u16     xBorderPad,
                                       const u16     yBorderPad,
                                       bool& hasNothingBehind) const
  {
    KnownMarker::NotVisibleReason reason = KnownMarker::NotVisibleReason::IS_VISIBLE;
    hasNothingBehind = false;
    for(auto const& marker : _markers) {
      
      if(marker.IsVisibleFrom(camera, maxFaceNormalAngle, minMarkerImageSize,
                              true, xBorderPad, yBorderPad, reason))
      {
        // As soon as any marker is visible from the camera, the object is visible from this camera
        if( DEBUG_PRINT_NOT_VISIBLE_REASONS ) {
          PRINT_NAMED_DEBUG("ObservableObject.IsVisibleFrom.MarkerVisible",
                            "marker '%s' is visible",
                            marker.GetCodeName());
        }
        return true;
      } else if(reason == KnownMarker::NotVisibleReason::NOTHING_BEHIND) {
        // The marker is not visible from this camera and the reason given is
        // that nothing was behind it. That means the _only_ reason we didn't mark
        // it visible is the lack of something behind it. This only has to be true
        // for a single marker.
        hasNothingBehind |= true;
      }

      if( DEBUG_PRINT_NOT_VISIBLE_REASONS ) {
        PRINT_NAMED_DEBUG("ObservableObject.IsVisibleFrom.MarkerNotVisible",
                          "marker '%s' is not visible for reason '%s'",
                          marker.GetCodeName(),
                          NotVisibleReasonToString(reason));
      }
    }
    
    return false;
  }

  KnownMarker::NotVisibleReason ObservableObject::IsVisibleFromWithReason(const Camera& camera,
                                                                          const f32     maxFaceNormalAngle,
                                                                          const f32     minMarkerImageSize,
                                                                          const bool    requireSomethingBehind,
                                                                          const u16     xBorderPad,
                                                                          const u16     yBorderPad) const
  {
    KnownMarker::NotVisibleReason latestReason = KnownMarker::NotVisibleReason::IS_VISIBLE;
    for(auto const& marker : _markers) {
      KnownMarker::NotVisibleReason reason = KnownMarker::NotVisibleReason::IS_VISIBLE;
      if(marker.IsVisibleFrom(camera, maxFaceNormalAngle, minMarkerImageSize,
                              requireSomethingBehind, xBorderPad, yBorderPad, reason))
      {
        // As soon as any marker is visible from the camera, the object is visible from this camera
        return KnownMarker::NotVisibleReason::IS_VISIBLE;
      } else {
        if( Util::EnumToUnderlying(reason) > Util::EnumToUnderlying(latestReason) ) {
          latestReason = reason;
        }
      }
    }

    return latestReason;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  const Pose3d& ObservableObject::GetPose() const
  {
    // We can't use the pose if the poseState is invalid, since it's potentially garbage. The calling code
    // should instead handle the case of receiving an object whose pose is not valid
    DEV_ASSERT(HasValidPose(), "ObservableObject.GetPose.ObjectPoseIsNotValid");
    return _pose;
  }

  Vision::KnownMarker const& ObservableObject::AddMarker(const Marker::Code&  withCode,
                                                         const Pose3d&        atPose,
                                                         const Point2f&       size_mm)
  {
    DEV_ASSERT(!atPose.HasParent(), "ObservableObject.AddMarker.MarkerPoseShouldBeRelative");
    
    // Copy the pose and set this object's pose as its parent
    Pose3d poseCopy(atPose);
    poseCopy.SetParent(_pose);
    
    // Construct a marker at that pose and store it keyed by its code
    _markers.emplace_back(withCode, poseCopy, size_mm);
    DEV_ASSERT(_markers.back().GetPose().IsChildOf(_pose), "ObservableObject.AddMarker.IncorrectParent");
    
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
                                  const Radians& angleThresholdIn,
                                  Point3f& Tdiff,
                                  Radians& angleDiff) const
  {
    // If the #define for ignoring rotation is set, use M_PI as the rotation angle
    // threshold, in which case rotation isn't even checked, since it always passes
    const Radians angleThreshold = (IGNORE_ROTATION_FOR_IS_SAME_AS ? M_PI : angleThresholdIn);
    
    const bool isSame = _pose.IsSameAs_WithAmbiguity(otherObject.GetPose(),
                                                     GetRotationAmbiguities(),
                                                     distThreshold, angleThreshold,
                                                     Tdiff, angleDiff);
    
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
                            "Failed to estimate pose of observed marker");
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
  
  RotationAmbiguities const& ObservableObject::GetRotationAmbiguities() const
  {
    static const RotationAmbiguities kUnambiguous;
    
    return kUnambiguous;
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
                          "No markers found with code %d",
                          withCode);
    }
    
  } // SetMarkersAsObserved()
  
  
  void ObservableObject::SetMarkerAsObserved(const ObservedMarker* nearestTo,
                                             const TimeStamp_t     atTime,
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

      f32 minCentroidDistSq = std::numeric_limits<f32>::max();
      
      Vision::KnownMarker* whichMarker = nullptr;
      for(auto marker : markers->second)
      {
        if(true == marker->GetPose().GetWithRespectTo(camera.GetPose(), markerPoseWrtCamera))
        {
          Quad2f projPoints;
          camera.Project3dPoints(marker->Get3dCorners(markerPoseWrtCamera), projPoints);
          
          const bool obsCentroidWithinProjectedQuad = projPoints.Contains(obsCentroid);
          if(obsCentroidWithinProjectedQuad)
          {
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
        }
      } // for each marker on this object
      
      if(whichMarker != nullptr) {
        whichMarker->SetLastObservedTime(atTime);
      } else {
        PRINT_NAMED_WARNING("ObservableObject.SetMarkerAsObserved.NoMatchingMarkerFound", "Marker:%s",
                            nearestTo->GetCodeName());
      }
    } else {
      PRINT_NAMED_WARNING("ObservableObject.SetMarkerAsObserved",
                          "No markers found with code (%d:%s) of given 'nearestTo' observed marker",
                          nearestTo->GetCode(), nearestTo->GetCodeName());
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

  static inline bool IsPoseFlat(const Pose3d& pose, const Radians& angleTol, AxisName* outWhichAxis)
  {
    const RotationMatrix3d Rmat = pose.GetWithRespectToRoot().GetRotationMatrix();
    const bool isFlat = (Rmat.GetAngularDeviationFromParentAxis<'Z'>(outWhichAxis) < angleTol);
    return isFlat;
  }
  
  bool ObservableObject::ClampPoseToFlat(Pose3d& pose, const Radians& angleTol)
  {
    // First make sure the pose is close enough to flat to be clamped
    // If it is, then keepAxis will be the one around which we'd like to preserve rotation
    // (the one aligned with Z in the parent frame)
    AxisName keepAxis;
    if(IsPoseFlat(pose, angleTol, &keepAxis))
    {
      // Original rotation
      const Rotation3d& R = pose.GetRotation();
      const RotationMatrix3d& Rmat = R.GetRotationMatrix();
      
      const s32 whichCol = AxisToIndex(keepAxis);
      const bool isNeg = (keepAxis == AxisName::X_NEG) || (keepAxis == AxisName::Y_NEG) || (keepAxis == AxisName::Z_NEG);
      const Vec3f& signedZ = (isNeg ? Z_AXIS_3D() * -1.f : Z_AXIS_3D());
      
      // Compute the angle and axis required to align the top side normal with the signed Z axis
      // Don't bother doing anything if the angle is already near zero
      const Vec3f& topSideNormal = Rmat.GetColumn(whichCol);
      Radians angle = std::acosf( DotProduct(topSideNormal, signedZ) );
      
      const bool alreadyAligned = Util::IsNearZero(angle.ToFloat());
      if(!alreadyAligned)
      {
        Vec3f axis  = CrossProduct(topSideNormal, signedZ);
        
        const bool zeroLength = Util::IsNearZero( axis.MakeUnitLength() );
        DEV_ASSERT(!zeroLength, "ObservableObject.ClampPoseToFlat.ComputedRotationAxisZeroLength");
        
        if(!zeroLength)
        {
          // Update the rotation by applying the rotation required to put the top side normal in line with the
          // (signed) Z axis
          Rotation3d Rdiff(angle, axis);
          pose.SetRotation(Rdiff * R);
          return true;
        }
      }
    }
    
    return false;
  }
  
  bool ObservableObject::IsRestingFlat(const Radians& angleTol) const
  {
    return IsPoseFlat(GetPose(), angleTol, nullptr);
  }
  
  Pose3d ObservableObject::GetZRotatedPointAboveObjectCenter(f32 heightFraction) const
  {
    // calculate the z translation of the top marker using the current object
    // center and half the size of the object along the current rotated parent axis
    const Pose3d poseWRTOrigin = _pose.GetWithRespectToRoot();
    float zAxisTrans = poseWRTOrigin.GetTranslation().z();
    
    const f32 zSize = GetDimInParentFrame<'Z'>(poseWRTOrigin);
    zAxisTrans += (heightFraction * zSize);
    
    return Pose3d(poseWRTOrigin.GetRotation().GetAngleAroundZaxis(),
                  Z_AXIS_3D(),
                  {_pose.GetTranslation().x(),
                    _pose.GetTranslation().y(),
                    zAxisTrans},
                  poseWRTOrigin.FindRoot());
  }


  bool ObservableObject::IsRestingAtHeight(float height, float tolerance) const
  {
    DEV_ASSERT(HasValidPose(), "ObservableObject.IsRestingAtHeight.ObjectPoseIsNotValid");
    
    const Pose3d& pose = GetPose().GetWithRespectToRoot();
    const f32 blockHeight = GetDimInParentFrame<'Z'>(pose);
    return std::abs(height - (pose.GetTranslation().z() - blockHeight/2)) < tolerance;
  }

  void ObservableObject::SetObservationTimes(const ObservableObject* otherObject)
  {
    DEV_ASSERT(nullptr != otherObject, "ObservableObject.SetObservationTimes.NullOtherObject");
    
    _lastObservedTime = std::max(_lastObservedTime, otherObject->_lastObservedTime);
    
    UpdateMarkerObservationTimes(*otherObject);
  }

  Result ObservableObject::GetClosestMarkerPose(const Pose3d& referencePose, const bool ignoreZ,
                                                Pose3d& closestPoseWrtReference) const
  {
    Marker marker(Marker::ANY_CODE);
    return GetClosestMarkerPose(referencePose, ignoreZ, closestPoseWrtReference, marker);
  }
  
  
  Result ObservableObject::GetClosestMarkerPose(const Pose3d& referencePose, const bool ignoreZ,
                                                Pose3d& closestPoseWrtReference,
                                                Marker& closestMarker) const
  {
    Result result = RESULT_FAIL_ORIGIN_MISMATCH;
    
    f32 minDistSq = std::numeric_limits<f32>::max();
    
    auto markers = GetMarkers();
    for(auto marker : markers)
    {
      Pose3d markerPoseWrtRef;
      if(marker.GetPose().GetWithRespectTo(referencePose, markerPoseWrtRef))
      {
        const f32 distSq = (ignoreZ ?
                            Point2f(markerPoseWrtRef.GetTranslation()).LengthSq() :
                            markerPoseWrtRef.GetTranslation().LengthSq());
        
        if(distSq < minDistSq)
        {
          minDistSq = distSq;
          closestPoseWrtReference = markerPoseWrtRef;
          closestMarker = marker;
          result = RESULT_OK;
        }
      }
    }
    
    return result;
  }
  
  // Helper for the following functions to reason about the object's height (mid or top)
  // relative to specified threshold
  bool ObservableObject::IsPoseTooHigh(const Pose3d& poseWrtRobot, float heightMultiplier,
                                       float heightTol, float offsetFraction) const
  {
    const float rotatedHeight = GetDimInParentFrame<'Z'>(poseWrtRobot);
    const float z = poseWrtRobot.GetTranslation().z() + (rotatedHeight*offsetFraction);
    const bool isTooHigh = FLT_GT(z, (heightMultiplier * rotatedHeight + heightTol));
    return isTooHigh;
  }
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_ObservableObject_H__

