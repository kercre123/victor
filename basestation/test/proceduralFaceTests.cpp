#include "gtest/gtest.h"

#include <vector>

#include "anki/common/types.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/common/basestation/math/point_impl.h"

// Sweep all parameters and make sure we don't trigger an assert or crash when
// we try to actually draw the face. (Clipping should prevent that.)
TEST(ProceduralFace, ParameterSweep)
{
  using namespace Anki::Cozmo;
  
  ProceduralFace procFace;
  
  ProceduralFaceParams& params = procFace.GetParams();
  
  const std::vector<ProceduralFaceParams::Value> values {
    -1000.f, -100.f, 0.f, 1.f, -1.f, 100, .00001f, -.00001f, .1f, 100000.f,
    90.f, 180.f, 270.f, 360.f, 45.f, -180.f, -360.f
  };
  
  // NOTE: This only checks same scale in both directions
  const std::vector<ProceduralFaceParams::Value> faceScales = {
    -100.f, -1.f, 0.f, 0.5f, 1.f, 1.5f, 100.f
  };
  
  const std::vector<ProceduralFaceParams::Value> faceAngles = {
    -1000.f, -360.f, -180.f -90.f, -45.f -10.f, -1.f, -0.001f, 0.f,
    .001f, 1.f, 10.f, 45.f, 90.f, 180.f, 360.f, 1000.f
  };
  
  using Param = ProceduralEyeParameter;
  
  // We know the following is gonna issue a zillion warnings. Let's not have them
  // all display.
  ProceduralFaceParams::EnableClippingWarning(false);
  
  for(auto faceScale : faceScales) {
    params.SetFaceScale({faceScale, faceScale});

    for(auto faceAngle : faceAngles) {
      params.SetFaceAngle(faceAngle);
      
      for(size_t iParam = 0; iParam < (size_t)Param::NumParameters; ++iParam)
      {
        //for(auto whichEye : {ProceduralFaceParams::Left, ProceduralFaceParams::Right}) {
          for(auto value : values) {
            params.SetParameter(ProceduralFaceParams::Left, (Param)iParam, value);
            params.SetParameter(ProceduralFaceParams::Right, (Param)iParam, value);
            
            EXPECT_NO_FATAL_FAILURE(procFace.GetFace());
          }
        //}
      }
    }
  }
  
} // TEST(ProceduralFace, ParameterSweep)
