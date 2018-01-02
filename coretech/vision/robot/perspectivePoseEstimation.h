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

#include "coretech/common/robot/geometry_declarations.h"
#include "coretech/common/robot/array2d_declarations.h"

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
      // NOTE: R1,R2,R3,R4 should all already be allocated to be 3x3.
      //
      template<typename PRECISION>
      Result computePossiblePoses(const Point3<PRECISION>& worldPoint1,
                                      const Point3<PRECISION>& worldPoint2,
                                      const Point3<PRECISION>& worldPoint3,
                                      const Point3<PRECISION>& imageRay1,
                                      const Point3<PRECISION>& imageRay2,
                                      const Point3<PRECISION>& imageRay3,
                                      Array<PRECISION>& R1, Point3<PRECISION>& T1,
                                      Array<PRECISION>& R2, Point3<PRECISION>& T2,
                                      Array<PRECISION>& R3, Point3<PRECISION>& T3,
                                      Array<PRECISION>& R4, Point3<PRECISION>& T4);
      
      // Use three points of a quadrilateral and the P3P algorithm above to
      // compute possible camera poses, then use the fourth point to choose the
      // valid pose. Do this four times (once using each corner as the validation
      // point) and choose the best pose overall, which is returned in R and T.
      // NOTE: R should already be allocated to be 3x3.
      // TODO: Make a Quadrilateral3 class to store 4 world points?
      template<typename PRECISION>
      Result computePose(const Quadrilateral<PRECISION>& imgQuad,
                             const Point3<PRECISION>& worldPoint1,
                             const Point3<PRECISION>& worldPoint2,
                             const Point3<PRECISION>& worldPoint3,
                             const Point3<PRECISION>& worldPoint4,
                             const f32 focalLength_x, const f32 focalLength_y,
                             const f32 camCenter_x,   const f32 camCenter_y,
                             Array<PRECISION>& R, Point3<PRECISION>& T);
      
      
      // Find the real parts of the four rootes of a quartic (4th order polynomial)
      // factors must point to a 5-element array, and realRoots must point to
      // a 4-element array.  (I.e. allocation is caller's responsibility.)
      // NOTE: Really only exposed for testing
      template<typename PRECISION>
      Result solveQuartic(const PRECISION* factors, PRECISION* realRoots);
      
    } // namespace P3P
  } // namespace Embedded
} // namespace Anki


#endif // ANKI_EMBEDDEDVISION_PERSPECTIVEPOSEESTIMATION_H
