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
    const u16 Rmask = 0xf800;
    const u16 Gmask = 0x07e0;
    const u16 Bmask = 0x001f;
    const u16 Rshift = 8;
    const u16 Gshift = 5;
    const u16 Bshift = 3;
    
    for (u8 i = 0; i < FACE_DISPLAY_HEIGHT; ++i) {
      for (u8 j = 0; j < FACE_DISPLAY_WIDTH; ++j) {
        
        int color = ((*frame & Rmask) << Rshift) +
                    ((*frame & Gmask) << Gshift) +
                    ((*frame & Bmask) << Bshift);
        ++frame;
        
        face_->setColor(color);
        face_->drawPixel(j, i);
      }
    }
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
