//
//  activeCube.h
//  Products_Cozmo
//
//  Created from block.h by Andrew Stein on 10/22/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __Anki_Cozmo_ActiveCube_H__
#define __Anki_Cozmo_ActiveCube_H__

#include "engine/block.h"
#include "engine/activeObject.h"

namespace Anki {
  
// Forward Declarations:
class Camera;

namespace Vector {
  
class ActiveCube : public Block, public ActiveObject
{
public:
  
  ActiveCube(Type type);
  ActiveCube(ActiveID activeID, FactoryID factoryID, ObjectType objectType);
  
  virtual ActiveCube* CloneType() const override {
    return new ActiveCube(this->_type);
  }
  
  // Make whatever state has been set on the block relative to a given (x,y)
  //  location.
  // When byUpperLeftCorner=true, "relative" means that the pattern is rotated
  //  so that whatever is currently specified for LED 0 is applied to the LED
  //  currently closest to the given position
  // When byUpperLeftCorner=false, "relative" means that the pattern is rotated
  //  so that whatever is specified for the side with LEDs 0 and 4 is applied
  //  to the face currently closest to the given position
  void MakeStateRelativeToXY(const Point2f& xyPosition, MakeRelativeMode mode);
  
  // Similar to above, but returns rotated WhichCubeLEDs rather than changing
  // the block's current state.
  WhichCubeLEDs MakeWhichLEDsRelativeToXY(const WhichCubeLEDs whichLEDs,
                                           const Point2f& xyPosition,
                                           MakeRelativeMode mode) const;
      
  
  // Take the given top LED pattern and create a pattern that indicates
  // the corresponding bottom LEDs as well
  static WhichCubeLEDs MakeTopAndBottomPattern(WhichCubeLEDs topPattern);
  
  // Get the LED specification for the top (and bottom) LEDs on the corner closest
  // to the specified (x,y) position, using the ActiveCube's current pose.
  WhichCubeLEDs GetCornerClosestToXY(const Point2f& xyPosition) const;
  
  // Get the LED specification for the four LEDs on the face closest
  // to the specified (x,y) position, using the ActiveCube's current pose.
  WhichCubeLEDs GetFaceClosestToXY(const Point2f& xyPosition) const;
  
  // Rotate the currently specified pattern of colors/flashing once slot in
  // the specified direction (assuming you are looking down at the top face)
  void RotatePatternAroundTopFace(bool clockwise);
  
  // Helper for figuring out which LEDs will be selected after rotating
  // a given pattern of LEDs one slot in the specified direction
  static WhichCubeLEDs RotateWhichLEDsAroundTopFace(WhichCubeLEDs whichLEDs, bool clockwise);
  
  // Populate a message specifying the current state of the block, for sending
  // out to actually set the physical block to match
  //void FillMessage(SetBlockLights& msg) const;

  // NOTE: No override for GetRotationAmbiguities because ActiveCubes have no ambiguity.
  //       (They have different markers on all sides.)
  
protected:
  
  // Temporary timer for faking duration of identification process
  // TODO: Remove once real identification is implemented
  static const s32 ID_TIME_MS = 300;
  s32 _identificationTimer = ID_TIME_MS;
  
}; // class ActiveCube


#pragma mark --- Inline Accessors Implementations ---

inline WhichCubeLEDs ActiveCube::MakeTopAndBottomPattern(WhichCubeLEDs topPattern) {
  u8 pattern = static_cast<u8>(topPattern);
  return static_cast<WhichCubeLEDs>((pattern << 4) + (pattern & 0x0F));
}
  
  
} // namespace Vector
} // namespace Anki

#endif // __Anki_Cozmo_ActiveCube_H__
