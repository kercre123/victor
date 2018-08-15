/**
 * File: matlabVisualization.h
 *
 * Author: Andrew Stein
 * Date:   3/28/2014
 *
 * Description: Code for visualizing the VisionSystem's tracking and detection
 *              status using a Matlab Engine.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_MATLAB_VISUALIZATION_H
#define ANKI_MATLAB_VISUALIZATION_H

#include "coretech/common/robot/array2d_declarations.h"
#include "coretech/common/robot/geometry_declarations.h"

#include "coretech/vision/shared/MarkerCodeDefinitions.h"


namespace Anki {
  using namespace Embedded;
  
  namespace Vector {
    
    namespace MatlabVisualization {
      
      Result Initialize();
      Result ResetFiducialDetection(const Array<u8>& image);
      Result SendFiducialDetection(const Quadrilateral<f32> &corners,
                                   const Vision::MarkerType &markerCode );
      Result SendDrawNow();
      
      Result SendTrackInit(const Array<u8> &image,
                           const Quadrilateral<f32>& quad);
      /*
      Result SendTrackInit(const Array<u8> &image,
                           const VisionSystem::Tracker& tracker,
                           MemoryStack scratch);
      */
      Result SendTrack(const Array<u8>& image,
                       const Quadrilateral<f32>& quad,
                       const bool converged);
      /*
      Result SendTrack(const Array<u8>& image,
                       const VisionSystem::Tracker& tracker,
                       const bool converged,
                       MemoryStack scratch);
      */
      Result SendTrackerPrediction_Before(const Array<u8>& image,
                                          const Quadrilateral<f32>& quad);
      
      Result SendTrackerPrediction_After(const Quadrilateral<f32>& quad);
      
    } // namespace MatlabVisualization
  } // namespace Vector
} // namespace Anki

#endif // ANKI_MATLAB_VISUALIZATION_H

