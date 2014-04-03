/**
 * File: perspectivePoseEstimation.h
 *
 * Author: Andrew Stein
 * Date:   04-02-2014
 *
 * Description: Embedded methods for the three-point perspective pose estimation 
 *              problem.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_EMBEDDEDVISION_PERSPECTIVEPOSEESTIMATION_H
#define ANKI_EMBEDDEDVISION_PERSPECTIVEPOSEESTIMATION_H

#include "anki/common/robot/geometry_declarations.h"
#include "anki/common/robot/array2d.h"

namespace Anki {
  namespace Embedded {
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
      
      /*
      template<typename PRECISION>
      ReturnCode computePossiblePoses(const std::array<Point<3,PRECISION>,3>& worldPoints,
                                      const std::array<Point<3,PRECISION>,3>& imageRays,
                                      std::array<Pose3d,4>& poses);
      */
      
      template<typename PRECISION>
      ReturnCode computePossiblePoses(const Point3<PRECISION>& worldPoint1,
                                      const Point3<PRECISION>& worldPoint2,
                                      const Point3<PRECISION>& worldPoint3,
                                      const Point3<PRECISION>& imageRay1,
                                      const Point3<PRECISION>& imageRay2,
                                      const Point3<PRECISION>& imageRay3,
                                      Array<PRECISION>& R1, Array<PRECISION>& T1,
                                      Array<PRECISION>& R2, Array<PRECISION>& T2,
                                      Array<PRECISION>& R3, Array<PRECISION>& T3,
                                      Array<PRECISION>& R4, Array<PRECISION>& T4,
                                      MemoryStack memory);
      
      
    } // namespace P3P
  } // namespace Embedded
} // namespace Anki


#endif // ANKI_VISION_PERSPECTIVEPOSEESTIMATION_H