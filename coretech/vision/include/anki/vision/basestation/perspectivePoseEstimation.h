/**
 * File: perspectivePoseEstimation.h
 *
 * Author: Andrew Stein
 * Date:   04-01-2014
 *
 * Description: Methods for the three-point perspective pose estimation problem.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_VISION_PERSPECTIVEPOSEESTIMATION_H
#define ANKI_VISION_PERSPECTIVEPOSEESTIMATION_H

#include <array>
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

#include "anki/vision/basestation/camera.h"

namespace Anki {
  namespace Vision {
    namespace P3P {

      // Take in 3 image rays and 3 corresponding world points, output
      // 4 possible pose solutions.  PRECISION is either double or float.
      // Image rays should be unit vectors, computed as follows from 2D image
      // coordinates (u,v) using intrinsic calibration information stored in a
      // 3x3 matrix, "K":
      //
      //   [u' v' w']^T = K^(-1) * [u v 1]^T
      //
      //   [u' v' w']^T /= norm([u' v' w']);
      //
      // Reference:
      //   "A Novel Parametrization of the Perspective-Three-Point Problem for
      //    a Direct Computation of Absolute Camera Position and Orientation"
      //   by Kneip et al.
      //
      template<typename PRECISION>
      ReturnCode computePossiblePoses(const std::array<Point<3,PRECISION>,3>& worldPoints,
                                      const std::array<Point<3,PRECISION>,3>& imageRays,
                                      std::array<Pose3d,4>& poses);
      
      // Use three points of a quadrilateral and the algorithm above to compute
      // possible camera poses, then use the fourth point to choose the valid
      // pose. Do this four times (once using each corner as the validation
      // point) and choose the best.  The templating allows the internal working
      // precision to differ from the precision of the input quadrilaterals.
      template<typename INPUT_PRECISION, typename WORKING_PRECISION>
      ReturnCode computePose(const Quadrilateral<2,INPUT_PRECISION>& imgQuad,
                             const Quadrilateral<3,INPUT_PRECISION>& worldQuad,
                             const CameraCalibration&                calib,
                             Pose3d& pose);
      
      
    } // namespace P3P
  } // namespace Vision
} // namespace Anki


#endif // ANKI_VISION_PERSPECTIVEPOSEESTIMATION_H