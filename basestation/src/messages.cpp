/**
 * File: messages.cpp  (Basestation)
 *
 * Author: Andrew Stein
 * Date:   1/21/2014
 *
 * Description: Defines a base Message class from which all the other messages
 *              inherit.  The inherited classes are auto-generated via multiple
 *              re-includes of the MessageDefinitions.h file, with specific
 *              "mode" flags set.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include <cstring> // for memcpy used in the constructor, created by MessageDefinitions


#include "anki/cozmo/basestation/messages.h"


namespace Anki {
  namespace Cozmo {
    
    // Impelement all the message classes' constructors
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_CONSTRUCTOR_MODE
#include "anki/cozmo/MessageDefinitions.h"
    
    // Implement all the message classes' GetSize() methods
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_GETSIZE_MODE
#include "anki/cozmo/MessageDefinitions.h"
    
    /*
      // THese are dummy placeholders to avoid linker errors for now
      
      void ProcessClearPathMessage(const ClearPath&) {}
      
      void ProcessSetMotionMessage(const SetMotion&) {}
      
      void ProcessRobotAvailableMessage(const RobotAvailable&) {}
      
      void ProcessVisionMarkerMessage(const VisionMarker&) {}
      
      void ProcessMatMarkerObservedMessage(const MatMarkerObserved&) {}
      
      void ProcessRobotAddedToWorldMessage(const RobotAddedToWorld&) {}
      
      void ProcessSetPathSegmentArcMessage(const SetPathSegmentArc&) {}
      
      void ProcessDockingErrorSignalMessage(const DockingErrorSignal&) {}
      
      void ProcessSetPathSegmentLineMessage(const SetPathSegmentLine&) {}
      
      void ProcessBlockMarkerObservedMessage(const BlockMarkerObserved&) {}
      
      void ProcessTemplateInitializedMessage(const TemplateInitialized&) {}
      
      void ProcessTotalVisionMarkersSeenMessage(const TotalVisionMarkersSeen&) {}
      
      void ProcessMatCameraCalibrationMessage(const MatCameraCalibration&) {}
      
      void ProcessAbsLocalizationUpdateMessage(const AbsLocalizationUpdate&) {}
      
      void ProcessHeadCameraCalibrationMessage(const HeadCameraCalibration&) {}
      
      void ProcessRobotStateMessage(const RobotState& msg) {}
      */

  } // namespace Cozmo
} // namespace Anki
