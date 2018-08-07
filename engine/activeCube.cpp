//
//  activeCube.cpp
//  Products_Cozmo
//
//  Created from block.cpp by Andrew Stein on 10/22/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

//#include "coretech/common/engine/math/linearAlgebra_impl.h"

#include "engine/activeCube.h"

//#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/point_impl.h"

//#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/imgproc/imgproc.hpp"
#endif

#include <iomanip>

namespace Anki {
namespace Vector {

ActiveCube::ActiveCube(ObjectType type)
: ObservableObject(ObjectFamily::LightCube, type)
, Block(ObjectFamily::LightCube, type)
{
  _activeID = -1;
  _factoryID = "";
  
  // Skip the check for ghost objects, which are a special case (they have 6 unknown markers)
  if(ANKI_DEVELOPER_CODE && (type != ObjectType::Block_LIGHTCUBE_GHOST))
  {
    // For now, assume 6 different markers, so we can avoid rotation ambiguities
    // Verify that here by making sure a set of markers has as many elements
    // as the original list:
    std::list<Vision::KnownMarker> const& markerList = GetMarkers();
    std::set<Vision::Marker::Code> uniqueCodes;
    for(auto & marker : markerList) {
      uniqueCodes.insert(marker.GetCode());
    }
    DEV_ASSERT(uniqueCodes.size() == markerList.size(), "ActiveCube.Constructor.InvalidMarkerList");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActiveCube::ActiveCube(ActiveID activeID, FactoryID factoryID, ObjectType objType)
: ActiveCube(objType)
{
  DEV_ASSERT(IsValidLightCube(objType, false) ||
             objType == ObjectType::Block_LIGHTCUBE_GHOST,
             "ActiveCube.InvalidFactoryID");
  
  _activeID = activeID;
  _factoryID = factoryID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActiveCube::MakeStateRelativeToXY(const Point2f& xyPosition, MakeRelativeMode mode)
{
  WhichCubeLEDs referenceLED = WhichCubeLEDs::NONE;
  switch(mode)
  {
    case MakeRelativeMode::RELATIVE_LED_MODE_OFF:
      // Nothing to do
      return;
      
    case MakeRelativeMode::RELATIVE_LED_MODE_BY_CORNER:
      referenceLED = GetCornerClosestToXY(xyPosition);
      break;
      
    case MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE:
      referenceLED = GetFaceClosestToXY(xyPosition);
      break;
      
    default:
      PRINT_NAMED_ERROR("ActiveCube.MakeStateRelativeToXY", "Unrecognized relative LED mode %s.", MakeRelativeModeToString(mode));
      return;
  }
  
  switch(referenceLED)
  {
    //
    // When using upper left corner (of current top face) as reference corner:
    //
    //   OR
    //
    // When using upper side (of current top face) as reference side:
    // (Note this is the current "Left" face of the block.)
    //
      
    case WhichCubeLEDs::FRONT_RIGHT:
    case WhichCubeLEDs::FRONT:
      // Nothing to do
      return;
      
    case WhichCubeLEDs::FRONT_LEFT:
    case WhichCubeLEDs::LEFT:
      // Rotate clockwise one slot
      RotatePatternAroundTopFace(true);
      return;
      
    case WhichCubeLEDs::BACK_RIGHT:
    case WhichCubeLEDs::RIGHT:
      // Rotate counterclockwise one slot
      RotatePatternAroundTopFace(false);
      return;
      
    case WhichCubeLEDs::BACK_LEFT:
    case WhichCubeLEDs::BACK:
      // Rotate two slots (either direction)
      // TODO: Do this in one shot
      RotatePatternAroundTopFace(true);
      RotatePatternAroundTopFace(true);
      return;
      
    default:
      PRINT_STREAM_ERROR("ActiveCube.MakeStateRelativeToXY",
                        "Unexpected reference LED " << static_cast<int>(referenceLED) << ".");
      return;
  }
} // MakeStateRelativeToXY()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhichCubeLEDs ActiveCube::MakeWhichLEDsRelativeToXY(const WhichCubeLEDs whichLEDs,
                                                     const Point2f& xyPosition,
                                                     MakeRelativeMode mode) const
{
  WhichCubeLEDs referenceLED = WhichCubeLEDs::NONE;
  switch(mode)
  {
    case MakeRelativeMode::RELATIVE_LED_MODE_OFF:
      // Nothing to do
      return whichLEDs;
      
    case MakeRelativeMode::RELATIVE_LED_MODE_BY_CORNER:
      referenceLED = GetCornerClosestToXY(xyPosition);
      break;
      
    case MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE:
      referenceLED = GetFaceClosestToXY(xyPosition);
      break;
      
    default:
      PRINT_NAMED_ERROR("ActiveCube.MakeStateRelativeToXY", "Unrecognized relateive LED mode %s.", MakeRelativeModeToString(mode));
      return whichLEDs;
  }
  
  switch(referenceLED)
  {
    //
    // When using upper left corner (of current top face) as reference corner:
    //
    //  OR
    //
    // When using upper side (of current top face) as reference side:
    // (Note this is the current "Left" face of the block.)
    //
    
    case WhichCubeLEDs::FRONT_RIGHT:
    case WhichCubeLEDs::FRONT:
      // Nothing to do
      return whichLEDs;
      
    case WhichCubeLEDs::FRONT_LEFT:
    case WhichCubeLEDs::LEFT:
      // Rotate clockwise one slot
      return RotateWhichLEDsAroundTopFace(whichLEDs, true);
      
    case WhichCubeLEDs::BACK_RIGHT:
    case WhichCubeLEDs::RIGHT:
      // Rotate counterclockwise one slot
      return RotateWhichLEDsAroundTopFace(whichLEDs, false);

    case WhichCubeLEDs::BACK_LEFT:
    case WhichCubeLEDs::BACK:
      // Rotate two slots (either direction)
      // TODO: Do this in one shot
      return RotateWhichLEDsAroundTopFace(RotateWhichLEDsAroundTopFace(whichLEDs, true), true);

    default:
      PRINT_STREAM_ERROR("ActiveCube.MakeStateRelativeToXY",
                        "Unexpected reference LED " << static_cast<int>(referenceLED) << ".");
      return whichLEDs;
  }
} // MakeWhichLEDsRelativeToXY()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhichCubeLEDs ActiveCube::GetCornerClosestToXY(const Point2f& xyPosition) const
{
  // Get a vector from center of marker in its current pose to given xyPosition
  Pose3d topMarkerPose;
  const Vision::KnownMarker& topMarker = GetTopMarker(topMarkerPose);
  const Vec2f topMarkerCenter(topMarkerPose.GetTranslation());
  Vec2f v(xyPosition);
  v -= topMarkerCenter;
  
  if (topMarker.GetCode() != GetMarker(FaceName::TOP_FACE).GetCode()) {
    PRINT_NAMED_WARNING("ActiveCube.GetCornerClosestToXY.IgnoringBecauseBlockOnSide", "");
    return WhichCubeLEDs::FRONT_LEFT;
  }
  
  PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "ActiveCube " << GetID().GetValue() << "'s TopMarker is = " << topMarker.GetCodeName() << ", angle = " << std::setprecision(3) << topMarkerPose.GetRotation().GetAngleAroundZaxis().getDegrees() << "deg");
  
  Radians angle = std::atan2(v.y(), v.x());
  angle -= topMarkerPose.GetRotationAngle<'Z'>();
  //assert(angle >= -M_PI && angle <= M_PI); // No longer needed: Radians class handles this
  
  WhichCubeLEDs whichLEDs = WhichCubeLEDs::NONE;
  if(angle > 0.f) {
    if(angle < M_PI_2) {
      // Between 0 and 90 degrees: Upper Right Corner
      PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest corner to (" << xyPosition.x() << "," << xyPosition.y() << "): Back Left");
      whichLEDs = WhichCubeLEDs::BACK_LEFT;
    } else {
      // Between 90 and 180: Upper Left Corner
      //assert(angle<=M_PI);
      PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest corner to (" << xyPosition.x() << "," << xyPosition.y() << "): Front Left");
      whichLEDs = WhichCubeLEDs::FRONT_LEFT;
    }
  } else {
    //assert(angle >= -M_PI);
    if(angle > -M_PI_2_F) {
      // Between -90 and 0: Lower Right Corner
      PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest corner to (" << xyPosition.x() << "," << xyPosition.y() << "): Back Right");
      whichLEDs = WhichCubeLEDs::BACK_RIGHT;
    } else {
      // Between -90 and -180: Lower Left Corner
      //assert(angle >= -M_PI);
      PRINT_STREAM_INFO("ActiveCube.GetCornerClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest corner to (" << xyPosition.x() << "," << xyPosition.y() << "): Front Right");
      whichLEDs = WhichCubeLEDs::FRONT_RIGHT;
    }
  }
  
  return whichLEDs;
} // GetCornerClosestToXY()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhichCubeLEDs ActiveCube::GetFaceClosestToXY(const Point2f& xyPosition) const
{
  // Get a vector from center of marker in its current pose to given xyPosition
  Pose3d topMarkerPose;
  const Vision::KnownMarker& topMarker = GetTopMarker(topMarkerPose);
  const Vec3f topMarkerCenter(topMarkerPose.GetTranslation());
  

  const Vec2f v(xyPosition.x()-topMarkerCenter.x(), xyPosition.y()-topMarkerCenter.y());
  
  if (topMarker.GetCode() != GetMarker(FaceName::TOP_FACE).GetCode()) {
    PRINT_NAMED_WARNING("ActiveCube.GetFaceClosestToXY.IgnoringBecauseBlockOnSide", "");
    return WhichCubeLEDs::FRONT;
  }
  
  
  PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "ActiveCube " << GetID().GetValue() << "'s TopMarker is = " << Vision::MarkerTypeStrings[topMarker.GetCode()] << ", angle = " << std::setprecision(3) << topMarkerPose.GetRotation().GetAngleAroundZaxis().getDegrees() << "deg");
  
  Radians angle = std::atan2(v.y(), v.x());
  angle = -(topMarkerPose.GetRotationAngle<'Z'>() - angle);
  
  
  WhichCubeLEDs whichLEDs = WhichCubeLEDs::NONE;
  if(angle < M_PI_4_F && angle >= -M_PI_4_F) {
    // Between -45 and 45 degrees: Back Face
    whichLEDs = WhichCubeLEDs::BACK;
    PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest face to (" << xyPosition.x() << "," << xyPosition.y() << "): Back");
  }
  else if(angle < 3*M_PI_4_F && angle >= M_PI_4_F) {
    // Between 45 and 135 degrees: Left Face
    whichLEDs = WhichCubeLEDs::LEFT;
    PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest face to (" << xyPosition.x() << "," << xyPosition.y() << "): Left");
  }
  else if(angle < -M_PI_4_F && angle >= -3*M_PI_4_F) {
    // Between -45 and -135: Right Face
    whichLEDs = WhichCubeLEDs::RIGHT;
    PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest face to (" << xyPosition.x() << "," << xyPosition.y() << "): Right");
  }
  else {
    // Between -135 && +135: Front Face
    assert(angle < -3*M_PI_4_F || angle > 3*M_PI_4_F);
    whichLEDs = WhichCubeLEDs::FRONT;
    PRINT_STREAM_INFO("ActiveCube.GetFaceClosestToXY", "Angle = " << std::setprecision(3) << angle.getDegrees() <<  "deg, Closest face to (" << xyPosition.x() << "," << xyPosition.y() << "): Front");
  }
  
  return whichLEDs;
} // GetFaceClosestToXY()

/*
WhichCubeLEDs ActiveCube::RotatePatternAroundTopFace(WhichCubeLEDs pattern, bool clockwise)
{
  static const u8 MASK = 0x88; // 0b10001000
  const u8 oldPattern = static_cast<u8>(pattern);
  if(clockwise) {
    return static_cast<WhichCubeLEDs>( ((oldPattern << 1) & ~MASK) | ((oldPattern & MASK) >> 3) );
  } else {
    return static_cast<WhichCubeLEDs>( ((oldPattern >> 1) & ~MASK) | ((oldPattern & MASK) << 3) );
  }
}
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline const u8* GetRotationLUT(bool clockwise)
{
  static const u8 cwRotatedPosition[ActiveCube::NUM_LEDS] = {
    3, 0, 1, 2
  };
  static const u8 ccwRotatedPosition[ActiveCube::NUM_LEDS] = {
    1, 2, 3, 0
  };
  
  // Choose the appropriate LUT
  const u8* rotatedPosition = (clockwise ? cwRotatedPosition : ccwRotatedPosition);
  
  return rotatedPosition;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActiveCube::RotatePatternAroundTopFace(bool clockwise)
{
  const u8* rotatedPosition = GetRotationLUT(clockwise);
  
  // Create the new state array
  std::array<LEDstate,NUM_LEDS> newState;
  for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
    newState[rotatedPosition[iLED]] = _ledState[iLED];
  }
  
  // Swap new state into place
  std::swap(newState, _ledState);
  
} // RotatePatternAroundTopFace()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhichCubeLEDs ActiveCube::RotateWhichLEDsAroundTopFace(WhichCubeLEDs whichLEDs, bool clockwise)
{
  const u8* rotatedPosition = GetRotationLUT(clockwise);
  
  u8 rotatedWhichLEDs = 0;
  u8 currentBit = 1;
  for(u8 iLED=0; iLED<NUM_LEDS; ++iLED) {
    // Set the corresponding rotated bit if the current bit is set
    rotatedWhichLEDs |= ((currentBit & (u8)whichLEDs)>0) << rotatedPosition[iLED];
    currentBit = (u8)(currentBit << 1);
  }

  return (WhichCubeLEDs)rotatedWhichLEDs;
}

} // namespace Vector
} // namespace Anki
