/**
File: matrix.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/matrix.h"
#include "util/math/math.h"

namespace Anki {
  namespace Embedded {
    namespace Matrix {
      
      float atan2(float y, float x)
      {
        AnkiAssert(y != 0 || x != 0);

        float arg = y/x;
        float atan_val = asinf( arg / sqrtf(arg*arg + 1));

        if (x > 0) {
          return atan_val;
        } else if (y >= 0 && x < 0) {
          return atan_val + M_PI_F;
        } else if (y < 0 && x < 0) {
          return atan_val - M_PI_F;
        } else if (y > 0 && x == 0) {
          return M_PI_2_F;
        }
        //else if (y < 0 && x == 0) {
        return -M_PI_2_F;
        //}
      }
      
      Result GetEulerAngles(const Array<f32>& R,
        f32& angle_x, f32& angle_y, f32& angle_z)
      {
        AnkiConditionalErrorAndReturnValue(R.IsValid() && AreEqualSize(3, 3, R),
          RESULT_FAIL_INVALID_SIZE,
          "GetEulerAngles",
          "R should be a valid 3x3 Array.");

        if(FLT_NEAR(fabs(R[2][0]), 1.f) ){
          angle_z = 0.f;
          if(R[2][0] > 0) { // R(2,0) = +1
            angle_y = M_PI_2;
            angle_x = atan2(R[0][1], R[0][2]);
          } else { // R(2,0) = -1
            angle_y = -M_PI_2;
            angle_x = atan2(-R[0][1], -R[0][2]);
          }
        } else {
          angle_y = asinf(R[2][0]);
          const f32 inv_cy = 1.f / cosf(angle_y);
          angle_x = atan2(-R[2][1]*inv_cy, R[2][2]*inv_cy);
          angle_z = atan2(-R[1][0]*inv_cy, R[0][0]*inv_cy);
        }

        return RESULT_OK;
      } // GetEulerAngles()
      
      Result GetRotationMatrix(const f32 angle_x, const f32 angle_y, const f32 angle_z,
                               Array<f32>& R)
      {
        AnkiConditionalErrorAndReturnValue(R.IsValid() && AreEqualSize(3, 3, R),
                                           RESULT_FAIL_INVALID_SIZE,
                                           "GetRotationMatrix",
                                           "R should be a valid 3x3 Array.");
        
        const f32 cY = cosf(angle_y);
        const f32 sY = -sinf(angle_y);
        const f32 cX = cosf(angle_x);
        const f32 sX = -sinf(angle_x);
        const f32 cZ = cosf(angle_z);
        const f32 sZ = -sinf(angle_z);
        
        R[0][0] = cY*cZ;  R[0][1] = sX*sY*cZ - cX*sZ;  R[0][2] = cX*sY*cZ + sX*sZ;
        R[1][0] = cY*sZ;  R[1][1] = sX*sY*sZ + cX*cZ;  R[1][2] = cX*sY*sZ - sX*cZ;
        R[2][0] = -sY;    R[2][1] = sX*cY;             R[2][2] = cX*cY;
        
        return RESULT_OK;
      }
      
    } // namespace Matrix
  } // namespace Embedded
} // namespace Anki
