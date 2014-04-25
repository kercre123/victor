/**
File: matrix.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/matrix.h"

namespace Anki {
  
  namespace Embedded {
    
    namespace Matrix {
      
      Result GetEulerAngles(const Array<f32>& R,
                            f32& angle_x, f32& angle_y, f32& angle_z)
      {
        AnkiConditionalErrorAndReturnValue(R.IsValid() && R.get_size(0)==3 && R.get_size(1)==3,
                                           RESULT_FAIL_INVALID_SIZE,
                                           "GetEulerAngles",
                                           "R should be a valid 3x3 Array.");
        
        if(FLT_NEAR(R[2][0], 1.f) ){
          angle_z = 0.f;
          if(R[2][0] > 0) { // R(2,0) = +1
            angle_y = M_PI_2;
            angle_x = atan2_acc(R[0][1], R[1][1]);
          } else { // R(2,0) = -1
            angle_y = -M_PI_2;
            angle_x = atan2_acc(-R[0][1], R[1][1]);
          }
        } else {
          angle_y = asinf(R[2][0]);
          const f32 inv_cy = 1.f / cosf(angle_y);
          angle_x = atan2_acc(-R[2][1]*inv_cy, R[2][2]*inv_cy);
          angle_z = atan2_acc(-R[1][0]*inv_cy, R[0][0]*inv_cy);
        }
        
        return RESULT_OK;
        
      } // GetEulerAngles()
      
      
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki
