/**
 * File: matlabVisionProcessor.h
 *
 * Author: Andrew Stein
 * Date:   3/28/2014
 *
 * Description: For testing vision algorithms implemented in Matlab.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_MATLABVISIONPROCESSOR_H
#define ANKI_COZMO_MATLABVISIONPROCESSOR_H

namespace Anki {
  namespace Cozmo {
    namespace MatlabVisionProcessor {
      
      using namespace Embedded;
      
      ReturnCode Initialize();
      
      ReturnCode InitTemplate(const Array<u8>& imgFull,
                              const Quadrilateral<f32>& trackingQuad,
                              MemoryStack scratch);
      
      // Update the tracker with a 3x3 transformation suitable for inverse
      // composition
      void UpdateTracker(const Array<f32>& predictionUpdate);
      
      // Update the tracker given a change in the robot's pose
      // (For planar6dof tracking.)
      void UpdateTracker(const f32 T_fwd_robot, const f32 T_hor_robot,
                         const Radians& theta_robot,
                         const Radians& theta_head);
      
      ReturnCode TrackTemplate(const Array<u8>& imgFull, bool& converged, MemoryStack scratch);
      
      Quadrilateral<f32> GetTrackerQuad();
      
      Transformations::PlanarTransformation_f32 GetTrackerTransform(MemoryStack& memory);
      
      void ComputeProjectiveDockingSignal(const Quadrilateral<f32>& transformedQuad,
                                          f32& x_distErr, f32& y_horErr, f32& angleErr);
      
    } // namespace MatlabVisionProcessor
  } // namespace Cozmo
} // namespace Anki

#endif // ifndef ANKI_COZMO_MATLABVISIONPROCESSOR_H
