#ifndef ANKI_VISION_PERSPECTIVEPOSEESTIMATION_H
#define ANKI_VISION_PERSPECTIVEPOSEESTIMATION_H

// POINT<N,Type> must implement:
//   operator-=(POINT<N,Type>)
//   cross(POINT<3,Type>, POINT<3,Type>)
//   operator/=(scalar)
//   norm()

// MATRIX3x3<Type> must implement
//   POINT<3,Type> operator*(POINT<3,Type>)
//   operator=(POINT<3,Type> col1, POINT<3,Type> col2, POINT<3,Type> col3)
//   transpose(


namespace Anki {
  namespace Vision {
    namespace P3P {
      
      template<template<size_t N, typename Type> class POINT,
      template <size_t NROWS, size_t NCOLS, typename Type> class MATRIX,
      typename PRECISION>
      ReturnCode createIntermediateCameraFrameHelper(POINT<3,PRECISION>& f1,
                                                     POINT<3,PRECISION>& f2,
                                                     POINT<3,PRECISION>& f3,
                                                     MATRIX<3,3,PRECISION>& T)
      {
        POINT<3,PRECISION> e1 = f1;
        POINT<3,PRECISION> e3 = cross(f1, f2);
        e3 /= e3.norm();
        POINT<3,PRECISION> e2 = cross(e3, e1);
        
        T = MATRIX<3,3,PRECISION>(e1, e2, e3);
        
        f3 = T * f3;
        
        return EXIT_SUCCESS;
      } // createIntermediateCameraFrameHelper()
      
      
      template<template<size_t N, typename Type> class POINT, class PRECISION>
      ReturnCode solveQuartic(const POINT<5,PRECISION>& factors,
                              POINT<4,PRECISION>& realRoots)
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
        
        // TODO: How do we do this complex math in embedded? (Note remaining pow() usage is complex)
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
      
      
      
      
      // Take in 3 image rays and 3 corresponding world points, output
      // 4 possible pose solutions.  PRECISION is either double or float
      template<template<size_t N, typename Type> class POINT,
      template <size_t NROWS, size_t NCOLS, typename Type> class MATRIX,
      typename PRECISION>
      ReturnCode computePossiblePoses(const POINT<3,PRECISION>& worldPoint1, const POINT<3,PRECISION>& worldPoint2, const POINT<3,PRECISION>& worldPoint3,
                              const POINT<3,PRECISION>& imageRay1,   const POINT<3,PRECISION>& imageRay2,   const POINT<3,PRECISION>& imageRay3,
                              MATRIX<3,3,PRECISION>& R1, POINT<3,PRECISION>& T1,
                              MATRIX<3,3,PRECISION>& R2, POINT<3,PRECISION>& T2,
                              MATRIX<3,3,PRECISION>& R3, POINT<3,PRECISION>& T3,
                              MATRIX<3,3,PRECISION>& R4, POINT<3,PRECISION>& T4)
      {
        typedef POINT<3,PRECISION> POINT3;
        POINT3 P1(worldPoint1);
        POINT3 P2(worldPoint2);
        POINT3 P3(worldPoint3);
        
        // Verify the world points are not colinear
        POINT3 temp1 = P2 - P1;
        POINT3 temp2 = P3 - P1;
        
        if(cross(temp1, temp2) == 0) {
          return EXIT_FAILURE;
        }
        
        POINT3 f1(imageRay1);
        POINT3 f2(imageRay2);
        POINT3 f3(imageRay3);
        
        // Create intermediate camera frame
        createIntermediateCameraFrameHelper(f1, f2, f3);
        
        // Reinforce that f3[2] > 0 for theta in [0,pi]
        if(f3[2] > 0)
        {
          f1 = imageRay2;
          f2 = imageRay1;
          f3 = imageRay3;
          
          createIntermediateCameraFrameHelper(f1, f2, f3);
          
          P1 = worldPoint2;
          P2 = worldPoint1;
          P3 = worldPoint3;
        }
        
        // Creation of intermediate world frame
        POINT3 n1 = P2 - P1;
        n1 /= n1.norm();
        
        POINT3 n3(cross(n1, (P3-P1)));
        n3 /= n3.norm();
        
        POINT3 n2(cross(n3,n1));
        
        MATRIX<3,3,PRECISION> N(n1, n2, n3);
        MATRIX<3,3,PRECISION> Nt = N.transpose();
        
        // Extraction of known parameters
        
        P3 = N*(P3-P1);
        
        PRECISION d_12 = (P2-P1).norm();
        PRECISION f_1 = f3[0]/f3[2];
        PRECISION f_2 = f3[1]/f3[2];
        PRECISION p_1 = P3[0];
        PRECISION p_2 = P3[1];
        
        PRECISION cos_beta = f1 * f2;
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
        POINT<5,PRECISION> factors;
        
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
        POINT<4,PRECISION> realRoots;
        solveQuartic(factors, realRoots);
        
        // Backsubstitution of each solution
        POINT3* T[4] = {&T1, &T2, &T3, &T4};
        MATRIX<3,3,PRECISION>* R[4] = {&R1, &R2, &R3, &R4};
        
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
          
          POINT3 C(d_12*cos_alpha*(sin_alpha*b+cos_alpha),
                   cos_theta*d_12*sin_alpha*(sin_alpha*b+cos_alpha),
                   sin_theta*d_12*sin_alpha*(sin_alpha*b+cos_alpha));
          
          // Assign this solution's translation vector to the output
          *(T[i]) = P1 + Nt*C;
          
          MATRIX<3,3,PRECISION> Rt;
          Rt[0][0] = -cos_alpha;            Rt[0][1] = sin_alpha;             Rt[0][2] = 0;
          Rt[1][0] = -sin_alpha*cos_theta;  Rt[1][1] = -cos_alpha*cos_theta;  Rt[1][2] = -cos_alpha*sin_theta;
          Rt[2][0] = -sin_alpha*sin_theta;  Rt[2][1] = -cos_alpha*sin_theta;  Rt[2][2] = cos_theta;
          
          // Assign this solution's rotation vector to the output
          *(R[i]) = Nt*Rt*T;
          
        }
        
        return EXIT_SUCCESS;
        
      } // computePossiblePoses()
      
    } // namespace P3P
  } // namespace Vision
} // namespace Anki


#endif // ANKI_VISION_PERSPECTIVEPOSEESTIMATION_H

