/**
 * File: perspectivePoseEstimation_impl.h
 *
 * Author: Andrew Stein
 * Date:   04-01-2014
 *
 * Description: Implementation of templated methods for the three-point 
 *              perspective pose estimation problem.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/


// NOTE:
// Some of the P3P code below was adapated from Laurent Kneip's source, which
// originally included this copyright message:

/*
 * Copyright (c) 2011, Laurent Kneip, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of ETH Zurich nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ETH ZURICH BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "anki/vision/basestation/perspectivePoseEstimation.h"

namespace Anki {
  namespace Vision {
    namespace P3P {
      
      template<typename PRECISION>
      ReturnCode createIntermediateCameraFrameHelper(Point<3,PRECISION>& f1,
                                                     Point<3,PRECISION>& f2,
                                                     Point<3,PRECISION>& f3,
                                                     SmallSquareMatrix<3,PRECISION>& T)
      {
        Point<3,PRECISION> e1 = f1;
        Point<3,PRECISION> e3 = cross(f1, f2);
        e3.makeUnitLength();
        Point<3,PRECISION> e2 = cross(e3, e1);
       
        // The e vectors are the rows of the T matrix
        T = {e1, e2, e3};
        T.Transpose();
        
        f3 = T * f3;
        
        return EXIT_SUCCESS;
      } // createIntermediateCameraFrameHelper()
      
      
      template<typename PRECISION>
      ReturnCode solveQuartic(const std::array<PRECISION,5>& factors,
                              std::array<PRECISION,4>& realRoots)
      {
        PRECISION A = factors[0];
        PRECISION B = factors[1];
        PRECISION C = factors[2];
        PRECISION D = factors[3];
        PRECISION E = factors[4];
        
        PRECISION A_pw2 = A*A;
        PRECISION B_pw2 = B*B;
        PRECISION A_pw3 = A_pw2*A;
        PRECISION B_pw3 = B_pw2*B;
        PRECISION A_pw4 = A_pw3*A;
        PRECISION B_pw4 = B_pw3*B;
        
        PRECISION alpha = -3*B_pw2/(8*A_pw2)+C/A;
        PRECISION beta = B_pw3/(8*A_pw3)-B*C/(2*A_pw2)+D/A;
        PRECISION gamma = -3*B_pw4/(256*A_pw4)+B_pw2*C/(16*A_pw3)-B*D/(4*A_pw2)+E/A;
        
        PRECISION alpha_pw2 = alpha*alpha;
        PRECISION alpha_pw3 = alpha_pw2*alpha;
        
        std::complex<PRECISION> P (-alpha_pw2/12-gamma,0);
        std::complex<PRECISION> Q (-alpha_pw3/108+alpha*gamma/3-beta*beta/8,0);
        std::complex<PRECISION> R = -Q/2.0+sqrt(pow(Q,2.0)/4.0+pow(P,3.0)/27.0);
        
        std::complex<PRECISION> U = pow(R,(1.0/3.0));
        std::complex<PRECISION> y;
        
        if (U.real() == 0) {
          y = -5.0*alpha/6.0-pow(Q,(1.0/3.0));
        } else {
          y = -5.0*alpha/6.0-P/(3.0*U)+U;
        }
        
        std::complex<PRECISION> w = sqrt(alpha+2.0*y);
        
        std::complex<PRECISION> temp;
        
        temp = -B/(4.0*A) + 0.5*(w+sqrt(-(3.0*alpha+2.0*y+2.0*beta/w)));
        realRoots[0] = temp.real();
        temp = -B/(4.0*A) + 0.5*(w-sqrt(-(3.0*alpha+2.0*y+2.0*beta/w)));
        realRoots[1] = temp.real();
        temp = -B/(4.0*A) + 0.5*(-w+sqrt(-(3.0*alpha+2.0*y-2.0*beta/w)));
        realRoots[2] = temp.real();
        temp = -B/(4.0*A) + 0.5*(-w-sqrt(-(3.0*alpha+2.0*y-2.0*beta/w)));
        realRoots[3] = temp.real();
        
        return EXIT_SUCCESS;
        
      } // solveQuartic()
      
      
      template<typename PRECISION>
      ReturnCode computePossiblePoses(const Point<3,PRECISION>& worldPoint1,
                                      const Point<3,PRECISION>& worldPoint2,
                                      const Point<3,PRECISION>& worldPoint3,
                                      const Point<3,PRECISION>& imageRay1,
                                      const Point<3,PRECISION>& imageRay2,
                                      const Point<3,PRECISION>& imageRay3,
                                      std::array<Pose3d,4>& poses)
      {
        // Typedef the templated classes for brevity below
        typedef Point<3,PRECISION>              POINT;
        typedef SmallSquareMatrix<3,PRECISION>  MATRIX;
        
        POINT P1(worldPoint1);
        POINT P2(worldPoint2);
        POINT P3(worldPoint3);
        
        // Verify the world points are not colinear
        if(cross(P2 - P1, P3 - P1).length() == 0) {
          return EXIT_FAILURE;
        }
        
        POINT f1(imageRay1);
        POINT f2(imageRay2);
        POINT f3(imageRay3);
        
        MATRIX T;
        
        // Create intermediate camera frame
        createIntermediateCameraFrameHelper(f1, f2, f3, T);
        
        // Reinforce that f3[2] > 0 for theta in [0,pi]
        if(f3[2] > 0)
        {
          f1 = imageRay2;
          f2 = imageRay1;
          f3 = imageRay3;
          
          createIntermediateCameraFrameHelper(f1, f2, f3, T);
          
          P1 = worldPoint2;
          P2 = worldPoint1;
          P3 = worldPoint3;
        }
        
        // Creation of intermediate world frame
        POINT n1 = P2 - P1;
        n1.makeUnitLength();
        
        POINT n3(cross(n1, (P3-P1)));
        n3.makeUnitLength();
        
        POINT n2(cross(n3,n1));
        
        // the n vectors are the rows of the N matrix (and thus the columns
        // of the N^T matrix)
        MATRIX N = {n1, n2, n3};
        N.Transpose();
        
        // Extraction of known parameters
        
        P3 = N*(P3-P1);
        
        PRECISION d_12 = (P2-P1).length();
        PRECISION f_1 = f3[0]/f3[2];
        PRECISION f_2 = f3[1]/f3[2];
        PRECISION p_1 = P3[0];
        PRECISION p_2 = P3[1];
        
        PRECISION cos_beta = dot(f1, f2);
        PRECISION b = 1/(1-cos_beta*cos_beta) - 1;
        
        if (cos_beta < 0) {
          b = -sqrt(b);
        }	else {
          b = sqrt(b);
        }
        
        // Definition of temporary variables for avoiding multiple computation
        
        PRECISION f_1_pw2 = f_1*f_1; //pow(f_1,2);
        PRECISION f_2_pw2 = f_2*f_2; //pow(f_2,2);
        PRECISION p_1_pw2 = p_1*p_1; //pow(p_1,2);
        PRECISION p_1_pw3 = p_1_pw2 * p_1;
        PRECISION p_1_pw4 = p_1_pw3 * p_1;
        PRECISION p_2_pw2 = p_2*p_2; //pow(p_2,2);
        PRECISION p_2_pw3 = p_2_pw2 * p_2;
        PRECISION p_2_pw4 = p_2_pw3 * p_2;
        PRECISION d_12_pw2 = d_12*d_12; //pow(d_12,2);
        PRECISION b_pw2 = b*b; //pow(b,2);
        
        
        // Computation of factors of 4th degree polynomial
        std::array<PRECISION,5> factors;
        
        factors[0] = -f_2_pw2*p_2_pw4
        -p_2_pw4*f_1_pw2
        -p_2_pw4;
        
        factors[1] = 2*p_2_pw3*d_12*b
        +2*f_2_pw2*p_2_pw3*d_12*b
        -2*f_2*p_2_pw3*f_1*d_12;
        
        factors[2] = -f_2_pw2*p_2_pw2*p_1_pw2
        -f_2_pw2*p_2_pw2*d_12_pw2*b_pw2
        -f_2_pw2*p_2_pw2*d_12_pw2
        +f_2_pw2*p_2_pw4
        +p_2_pw4*f_1_pw2
        +2*p_1*p_2_pw2*d_12
        +2*f_1*f_2*p_1*p_2_pw2*d_12*b
        -p_2_pw2*p_1_pw2*f_1_pw2
        +2*p_1*p_2_pw2*f_2_pw2*d_12
        -p_2_pw2*d_12_pw2*b_pw2
        -2*p_1_pw2*p_2_pw2;
        
        factors[3] = 2*p_1_pw2*p_2*d_12*b
        +2*f_2*p_2_pw3*f_1*d_12
        -2*f_2_pw2*p_2_pw3*d_12*b
        -2*p_1*p_2*d_12_pw2*b;
        
        factors[4] = -2*f_2*p_2_pw2*f_1*p_1*d_12*b
        +f_2_pw2*p_2_pw2*d_12_pw2
        +2*p_1_pw3*d_12
        -p_1_pw2*d_12_pw2
        +f_2_pw2*p_2_pw2*p_1_pw2
        -p_1_pw4
        -2*f_2_pw2*p_2_pw2*p_1*d_12
        +p_2_pw2*f_1_pw2*p_1_pw2
        +f_2_pw2*p_2_pw2*d_12_pw2*b_pw2;
        
        // Computation of roots
        std::array<PRECISION,4> realRoots;
        solveQuartic(factors, realRoots);
        
        // Backsubstitution of each solution
        MATRIX Tt = T.getTranspose();
        MATRIX Nt = N.getTranspose();
        for(s32 i=0; i<4; i++)
        {
          PRECISION cot_alpha = (-f_1*p_1/f_2-realRoots[i]*p_2+d_12*b)/(-f_1*realRoots[i]*p_2/f_2+p_1-d_12);
          
          PRECISION cos_theta = realRoots[i];
          PRECISION sin_theta = sqrt(1-realRoots[i]*realRoots[i]);
          PRECISION sin_alpha = sqrt(1/(cot_alpha*cot_alpha+1));
          PRECISION cos_alpha = sqrt(1-sin_alpha*sin_alpha);
          
          if (cot_alpha < 0) {
            cos_alpha = -cos_alpha;
          }
          
          POINT C(d_12*cos_alpha*(sin_alpha*b+cos_alpha),
                  cos_theta*d_12*sin_alpha*(sin_alpha*b+cos_alpha),
                  sin_theta*d_12*sin_alpha*(sin_alpha*b+cos_alpha));
          
          MATRIX R;
          R(0,0) = -cos_alpha;    R(0,1) = -sin_alpha*cos_theta;   R(0,2) = -sin_alpha*sin_theta;
          R(1,0) =  sin_alpha;    R(1,1) = -cos_alpha*cos_theta;   R(1,2) = -cos_alpha*sin_theta;
          R(2,0) = 0;             R(2,1) = -sin_theta;             R(2,2) = cos_theta;
          
          // TODO: Clean up all these redundant transposes!
          
          // Assign this solution's rotation vector to the output
          // TODO: Make this more elegant
          //MATRIX R_temp = Nt * Rt * T;
          MATRIX R_temp = Tt * R * N;
          SmallSquareMatrix<3, float> R_float;
          for(s32 i=0; i<3; ++i) {
            for(s32 j=0; j<3; ++j) {
              R_float(i,j) = static_cast<float>(R_temp(i,j));
            }
          }
          poses[i].set_rotation(R_float);
          
          // Assign this solution's translation vector to the output
          POINT translation = R_temp * (P1 + Nt*C);
          translation *= -PRECISION(1);
          Point<3,float> t_float(static_cast<float>(translation.x()),
                                 static_cast<float>(translation.y()),
                                 static_cast<float>(translation.z()));
          poses[i].set_translation(t_float);

        }
        
        return EXIT_SUCCESS;
      } // computePossiblePoses(from individually-listed points)
      

      template<typename PRECISION>
      ReturnCode computePossiblePoses(const std::array<Point<3,PRECISION>,3>& worldPoints,
                                      const std::array<Point<3,PRECISION>,3>& imageRays,
                                      std::array<Pose3d,4>& poses)
      {
        return computePossiblePoses(worldPoints[0], worldPoints[1], worldPoints[2],
                                    imageRays[0], imageRays[1], imageRays[2],
                                    poses);
      } // computePossiblePoses(from std::arrays)
      
      
      template<typename INPUT_PRECISION, typename WORKING_PRECISION>
      ReturnCode computePose(const Quadrilateral<2,INPUT_PRECISION>& imgQuad,
                             const Quadrilateral<3,INPUT_PRECISION>& worldQuad,
                             const CameraCalibration&                calib,
                             Pose3d& pose)
      {
        // Turn the three image points into unit vectors corresponding to rays
        // in the direction of the image points
        const SmallSquareMatrix<3,WORKING_PRECISION> invK = calib.get_invCalibrationMatrix<WORKING_PRECISION>();
        const SmallSquareMatrix<3,WORKING_PRECISION> K    = calib.get_calibrationMatrix<WORKING_PRECISION>();
        
        Quadrilateral<3, WORKING_PRECISION> imgRays, worldPoints;

        for(Quad::CornerName i_corner=Quad::FirstCorner; i_corner < Quad::NumCorners; ++i_corner)
        {
          // Get unit vector pointing along each image ray
          //   imgRay = K^(-1) * [u v 1]^T
          imgRays[i_corner].x() = static_cast<WORKING_PRECISION>(imgQuad[i_corner].x());
          imgRays[i_corner].y() = static_cast<WORKING_PRECISION>(imgQuad[i_corner].y());
          imgRays[i_corner].z() = WORKING_PRECISION(1);
          
          imgRays[i_corner] = invK * imgRays[i_corner];
          imgRays[i_corner].makeUnitLength();
          
          // cast each world quad into working precision quad
          worldPoints[i_corner].x() = static_cast<WORKING_PRECISION>(worldQuad[i_corner].x());
          worldPoints[i_corner].y() = static_cast<WORKING_PRECISION>(worldQuad[i_corner].y());
          worldPoints[i_corner].z() = static_cast<WORKING_PRECISION>(worldQuad[i_corner].z());
        }
        
        
        // Compute best pose from each subset of three corners, keeping the one
        // with the lowest error
        INPUT_PRECISION minErrorOuter = std::numeric_limits<INPUT_PRECISION>::max();
        for(Quad::CornerName i_validate=Quad::FirstCorner; i_validate < Quad::NumCorners; ++i_validate)
        {
          // Create the list of three points to use for estimation (all but
          // the current validation point)
          std::vector<Quad::CornerName> estimateIndex;
          estimateIndex.reserve(3);

          for(Quad::CornerName i_corner=Quad::FirstCorner; i_corner < Quad::NumCorners; ++i_corner) {
            if(i_corner != i_validate) {
              estimateIndex.push_back(i_corner);
            }
          }
          
          CORETECH_ASSERT(estimateIndex.size() == 3);
          
          // Use those three points to get four possible poses
          std::array<Pose3d,4> possiblePoses;
          computePossiblePoses(worldPoints[estimateIndex[0]],
                               worldPoints[estimateIndex[1]],
                               worldPoints[estimateIndex[2]],
                               imgRays[estimateIndex[0]],
                               imgRays[estimateIndex[1]],
                               imgRays[estimateIndex[2]],
                               possiblePoses);
          
          // Find the pose with the least reprojection error for the 4th
          // validation corner (which was not used in estimating the pose)
          s32 bestSolution = -1;
          INPUT_PRECISION minErrorInner = std::numeric_limits<INPUT_PRECISION>::max();
          
          for(s32 i_solution=0; i_solution<4; ++i_solution)
          {
            //Point3f projectedPoint3d = K * (possiblePoses[i_solution]*worldValidationPoint);
            Point<3,WORKING_PRECISION> projectedPoint3d = K * (possiblePoses[i_solution]*worldQuad[i_validate]);
            
            Point<2,INPUT_PRECISION> projectedPoint2d(static_cast<INPUT_PRECISION>(projectedPoint3d.x() / projectedPoint3d.z()),
                                                      static_cast<INPUT_PRECISION>(projectedPoint3d.y() / projectedPoint3d.z()));
            
            INPUT_PRECISION error = (projectedPoint2d - imgQuad[i_validate]).length();
            
            if(error < minErrorInner) {
              minErrorInner = error;
              bestSolution = i_solution;
            }
            
          } // for each solution
          
          CORETECH_ASSERT(bestSolution >= 0);

          // If the pose using this validation corner is better than the
          // best so far, keep it
          if(minErrorInner < minErrorOuter) {
            minErrorOuter = minErrorInner;
            pose = possiblePoses[bestSolution];
          }
          
        } // for each validation corner
        
        return EXIT_SUCCESS;
        
      } // computePose()
      
    } // namespace P3P
  } // namespace Vision
} // namespace Ank
