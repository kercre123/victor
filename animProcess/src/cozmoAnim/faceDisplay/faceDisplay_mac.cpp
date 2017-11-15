/**
 * File: faceDisplay_mac.cpp
 *
 * Author: Kevin Yoon
 * Created: 07/20/2017
 *
 * Description:
 *               Defines interface to simulated face display
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "util/logging/logging.h"

#include <webots/Supervisor.hpp>
#include <webots/Display.hpp>


#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using faceDisplay_mac.cpp
#endif

extern webots::Supervisor animSupervisor;

namespace Anki {
namespace Cozmo {
  
namespace { // "Private members"

  // Face display
  webots::Display* face_;
  
  // Face 'image' to send to webots each frame
  u32 faceImg_[FACE_DISPLAY_WIDTH*FACE_DISPLAY_HEIGHT] = {0};
  
} // "private" namespace


#pragma mark --- Simulated Hardware Method Implementations ---
  
  
  // Definition of static field
  FaceDisplay* FaceDisplay::_instance = 0;
  
  /**
   * Returns the single instance of the object.
   */
  FaceDisplay* FaceDisplay::getInstance() {
    // check if the instance has been created yet
    if(0 == _instance) {
      // if not, then create it
      _instance = new FaceDisplay;
    }
    // return the single instance
    return _instance;
  }
  
  /**
   * Removes instance
   */
  void FaceDisplay::removeInstance() {
    // check if the instance has been created yet
    if(0 != _instance) {
      delete _instance;
      _instance = 0;
    }
  };
  
  FaceDisplay::FaceDisplay()
  {
    // Did you remember to call SetSupervisor()?
//      DEV_ASSERT(animSupervisor != nullptr, "animSupervisor.NullWebotsSupervisor");
    
    // Is the step time defined in the world file >= than the robot time? It should be!
//      DEV_ASSERT(TIME_STEP >= animSupervisor->getBasicTimeStep(), "animSupervisor.UnexpectedTimeStep");

    // Face display
    face_ = animSupervisor.getDisplay("face_display");
    assert(face_->getWidth() == FACE_DISPLAY_WIDTH);
    assert(face_->getHeight() == FACE_DISPLAY_HEIGHT);
    face_->setFont("Lucida Console", 8, true);
    FaceClear();
  }

  FaceDisplay::~FaceDisplay() {
    
  }

  void FaceDisplay::FaceClear()
  {
    face_->setColor(0);
    face_->fillRectangle(0,0, FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT);
  }
  
  void FaceDisplay::FaceDraw(u16* frame)
  {
    // Masks and shifts to convert an RGB565 color into a
    // 32-bit BGRA color (i.e. 0xBBGGRRAA) which webots expects
    const u16 Rmask = 0xf800;
    const u16 Gmask = 0x07e0;
    const u16 Bmask = 0x001f;
    const u16 Rshift = 0;
    const u16 Gshift = 13;
    const u16 Bshift = 27;
    
    u32* imgPtr = &faceImg_[0];
    
    for (u32 i = 0; i < FACE_DISPLAY_WIDTH*FACE_DISPLAY_HEIGHT; ++i) {
      const u16 bytesSwapped = ((*frame & 0xFF)<<8) | ((*frame >> 8)&0xFF);
      
      // Convert RGB565 color into BGRA (i.e. 0xBBGGRRAA)
      // Set alpha to 0xFF, since we don't want any transparency.
      const u32 color = ((u32) (bytesSwapped & Rmask) << Rshift) |
                        ((u32) (bytesSwapped & Gmask) << Gshift) |
                        ((u32) (bytesSwapped & Bmask) << Bshift) |
                        0x000000FF;
      
      *imgPtr++ = color;
      ++frame;
    }

    // Send the entire image to webots (by using the webots::Display 'clipboard' functionality),
    // paste it from the 'clipboard' to the main display, then delete it.
    // (see https://www.cyberbotics.com/doc/reference/display#display-functions)
    auto imgRef = face_->imageNew(FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT, faceImg_, webots::Display::ARGB);
    face_->imagePaste(imgRef, 0, 0);
    face_->imageDelete(imgRef);
  }
  
  void FaceDisplay::FacePrintf(const char* format, ...)
  {
    // TODO: Smartly insert line breaks?

    face_->setColor(0xf0ff);
   
    #define MAX_FACE_DISPLAY_CHAR_LENGTH 30
    char line[MAX_FACE_DISPLAY_CHAR_LENGTH];
    
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(line, MAX_FACE_DISPLAY_CHAR_LENGTH, format, argptr);
    va_end(argptr);
    
    face_->drawText(std::string(line), 0, 0);
  }

} // namespace Cozmo
} // namespace Anki
