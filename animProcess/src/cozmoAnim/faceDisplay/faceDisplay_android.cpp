/**
 * File: faceDisplay_android.cpp
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

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"
      
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
      // Stub
    }

    FaceDisplay::~FaceDisplay()
    {
      
    }
    
    void FaceDisplay::FaceClear()
    {
      // Stub
    }
    
    void FaceDisplay::FaceDraw(u16* frame)
    {
      // Stub
    }
    
    void FaceDisplay::FacePrintf(const char* format, ...)
    {
      // Stub
    }
  
  } // namespace Cozmo
} // namespace Anki
