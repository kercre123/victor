/**
 * File: perspectivePoseEstimation.cpp
 *
 * Author: Andrew Stein
 * Date:   04-02-2014
 *
 * Description: Embedded implementation of templated methods for the three-point
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



#include "anki/vision/robot/perspectivePoseEstimation.h"

namespace Anki {
  namespace Embedded {
    namespace P3P {
      
      template<typename PRECISION>
      ReturnCode createIntermediateCameraFrameHelper(Point3<PRECISION>& f1,
                                                     Point3<PRECISION>& f2,
                                                     Point3<PRECISION>& f3,
                                                     Array<PRECISION>& T)
      {
        Point3<PRECISION> e1 = f1;
        Point3<PRECISION> e3 = CrossProduct(f1, f2);
        e3 *= PRECISION(1) / (PRECISION) e3.Length();
        Point3<PRECISION> e2 = CrossProduct(e3, e1);
       
        // The e vectors are the rows of the T matrix (and T should already be allocated)
        AnkiAssert(T.get_size(0) == 3 && T.get_size(1) == 3);
        T[0][0] = e1.x;   T[0][1] = e1.y;   T[0][2] = e1.z;
        T[1][0] = e2.x;   T[1][1] = e2.y;   T[1][2] = e2.z;
        T[2][0] = e3.x;   T[2][1] = e3.y;   T[2][2] = e3.z;
        
        f3 = T * f3;
        
        return EXIT_SUCCESS;
      } // createIntermediateCameraFrameHelper()
      
      
      template<typename PRECISION>
      ReturnCode solveQuartic(const PRECISION* factors, //const Array<PRECISION>& factors,  // 1x5
                              PRECISION* realRoots) //Array<PRECISION>& realRoots)      // 1x4
      {
        //AnkiAssert(factors.get_size(0) == 1 && factors.get_size(1) == 5);
        //AnkiAssert(realRoots.get_size(0) == 1 && realRoots.get_size(1) == 4);
        
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
        std::complex<PRECISION> R = -Q/PRECISION(2)+sqrt(pow(Q,PRECISION(2))/PRECISION(4)+pow(P,PRECISION(3))/PRECISION(27));
        
        std::complex<PRECISION> U = pow(R,PRECISION(1.0/3.0));
        std::complex<PRECISION> y;
        
        if (U.real() == 0) {
          y = -PRECISION(5)*alpha/PRECISION(6)-pow(Q,PRECISION(1.0/3.0));
        } else {
          y = -PRECISION(5)*alpha/PRECISION(6)-P/(PRECISION(3)*U)+U;
        }
        
        std::complex<PRECISION> w = sqrt(alpha+PRECISION(2)*y);
        
        std::complex<PRECISION> temp;
        
        temp = -B/(PRECISION(4)*A) + PRECISION(0.5)*(w+sqrt(-(PRECISION(3)*alpha+PRECISION(2)*y+PRECISION(2)*beta/w)));
        realRoots[0] = temp.real();
        temp = -B/(PRECISION(4)*A) + PRECISION(0.5)*(w-sqrt(-(PRECISION(3)*alpha+PRECISION(2)*y+PRECISION(2)*beta/w)));
        realRoots[1] = temp.real();
        temp = -B/(PRECISION(4)*A) + PRECISION(0.5)*(-w+sqrt(-(PRECISION(3)*alpha+PRECISION(2)*y-PRECISION(2)*beta/w)));
        realRoots[2] = temp.real();
        temp = -B/(PRECISION(4)*A) + PRECISION(0.5)*(-w-sqrt(-(PRECISION(3)*alpha+PRECISION(2)*y-PRECISION(2)*beta/w)));
        realRoots[3] = temp.real();
        
        return EXIT_SUCCESS;
        
      } // solveQuartic()
      
      
      template<typename PRECISION>
      ReturnCode computePossiblePoses(const Point3<PRECISION>& worldPoint1,
                                      const Point3<PRECISION>& worldPoint2,
                                      const Point3<PRECISION>& worldPoint3,
                                      const Point3<PRECISION>& imageRay1,
                                      const Point3<PRECISION>& imageRay2,
                                      const Point3<PRECISION>& imageRay3,
                                      Array<PRECISION>& R1, Point3<PRECISION>& T1,
                                      Array<PRECISION>& R2, Point3<PRECISION>& T2,
                                      Array<PRECISION>& R3, Point3<PRECISION>& T3,
                                      Array<PRECISION>& R4, Point3<PRECISION>& T4,
                                      MemoryStack memory)
      {
        // Typedef the templated classes for brevity below
        typedef Point3<PRECISION> POINT;
        typedef Array<PRECISION>  MATRIX;
        
        POINT P1(worldPoint1);
        POINT P2(worldPoint2);
        POINT P3(worldPoint3);
        
        // Verify the world points are not colinear
        if(CrossProduct(P2 - P1, P3 - P1).Length() == 0) {
          return EXIT_FAILURE;
        }
        
        POINT f1(imageRay1);
        POINT f2(imageRay2);
        POINT f3(imageRay3);
        
        MATRIX T = MATRIX(3,3,memory);
        
        // Create intermediate camera frame
        createIntermediateCameraFrameHelper(f1, f2, f3, T);
        
        // Reinforce that f3[2] > 0 for theta in [0,pi]
        if(f3.z > 0)
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
        n1 *= PRECISION(1) / (PRECISION) n1.Length();
        
        POINT n3(CrossProduct(n1, (P3-P1)));
        n3 *= PRECISION(1) / (PRECISION) n3.Length();
        
        POINT n2(CrossProduct(n3,n1));
        
        // the n vectors are the rows of the N matrix
        MATRIX N = MATRIX(3,3,memory);
        N[0][0] = n1.x; N[0][1] = n1.y; N[0][2] = n1.z;
        N[1][0] = n2.x; N[1][1] = n2.y; N[1][2] = n2.z;
        N[2][0] = n3.x; N[2][1] = n3.y; N[2][2] = n3.z;
        
        // Extraction of known parameters
        
        P3 = N*(P3-P1);
        
        PRECISION d_12 = (P2-P1).Length();
        PRECISION f_1 = f3.x/f3.z;
        PRECISION f_2 = f3.y/f3.z;
        PRECISION p_1 = P3.x;
        PRECISION p_2 = P3.y;
        
        PRECISION cos_beta = DotProduct(f1, f2);
        PRECISION b = PRECISION(1)/(PRECISION(1)-cos_beta*cos_beta) - PRECISION(1);
        
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
        PRECISION factors[5];
        
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
        PRECISION realRoots[4];
        solveQuartic(factors, realRoots);
        
        // Backsubstitution of each solution
        
        // Make an array of pointers to the outputs so we can loop over them
        // to create each solution below
        Array<PRECISION>*  Rout[4] = {&R1, &R2, &R3, &R4};
        Point3<PRECISION>* Tout[4] = {&T1, &T2, &T3, &T4};
        
        MATRIX Tt = MATRIX(3,3,memory);
        Matrix::Transpose(T, Tt);
        
        MATRIX Nt = MATRIX(3,3,memory);
        Matrix::Transpose(N, Nt);
        
        MATRIX R = MATRIX(3,3,memory);
        MATRIX temp = MATRIX(3,3,memory);
        
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
          
          // Fill in the initial R matrix
          R[0][0] = -cos_alpha;    R[0][1] = -sin_alpha*cos_theta;   R[0][2] = -sin_alpha*sin_theta;
          R[1][0] =  sin_alpha;    R[1][1] = -cos_alpha*cos_theta;   R[1][2] = -cos_alpha*sin_theta;
          R[2][0] =  0;            R[2][1] = -sin_theta;             R[2][2] = cos_theta;

          // Assign this solution's rotation matrix to the output
          //  Rout[i] = Tt * R * N;
          Matrix::Multiply(Tt, R, temp);
          Matrix::Multiply(temp, N, *Rout[i]);

          POINT C(d_12*cos_alpha*(sin_alpha*b+cos_alpha),
                  cos_theta*d_12*sin_alpha*(sin_alpha*b+cos_alpha),
                  sin_theta*d_12*sin_alpha*(sin_alpha*b+cos_alpha));
          
          // Assign this solution's translation vector to the output
          *Tout[i] = *Rout[i] * (P1 + Nt*C);
          *Tout[i] *= -PRECISION(1);
          
        }
        
        return EXIT_SUCCESS;
      } // computePossiblePoses(from individually-listed points)
      
      
      // Explicit instatiation for single and double precision
      template ReturnCode computePossiblePoses<float>(const Point3<float>& worldPoint1,
                                                      const Point3<float>& worldPoint2,
                                                      const Point3<float>& worldPoint3,
                                                      const Point3<float>& imageRay1,
                                                      const Point3<float>& imageRay2,
                                                      const Point3<float>& imageRay3,
                                                      Array<float>& R1, Point3<float>& T1,
                                                      Array<float>& R2, Point3<float>& T2,
                                                      Array<float>& R3, Point3<float>& T3,
                                                      Array<float>& R4, Point3<float>& T4,
                                                      MemoryStack memory);
      
      template ReturnCode computePossiblePoses<double>(const Point3<double>& worldPoint1,
                                                       const Point3<double>& worldPoint2,
                                                       const Point3<double>& worldPoint3,
                                                       const Point3<double>& imageRay1,
                                                       const Point3<double>& imageRay2,
                                                       const Point3<double>& imageRay3,
                                                       Array<double>& R1, Point3<double>& T1,
                                                       Array<double>& R2, Point3<double>& T2,
                                                       Array<double>& R3, Point3<double>& T3,
                                                       Array<double>& R4, Point3<double>& T4,
                                                       MemoryStack memory);
      
      
/*
      template<typename PRECISION>
      ReturnCode computePossiblePoses(const std::array<Point3<PRECISION>,3>& worldPoints,
                                      const std::array<Point3<PRECISION>,3>& imageRays,
                                      std::array<Pose3d,4>& poses)
      {
        return computePossiblePoses(worldPoints[0], worldPoints[1], worldPoints[2],
                                    imageRays[0], imageRays[1], imageRays[2],
                                    poses);
      } // computePossiblePoses(from std::arrays)
  */
      
    } // namespace P3P
  } // namespace Embedded
} // namespace Ank
