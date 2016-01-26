#ifndef __Anki_Cozmo_ProceduralFaceUtil_H__
#define __Anki_Cozmo_ProceduralFaceUtil_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/matrix.h"
#include "anki/cozmo/basestation/proceduralFace.h"

#include <opencv2/core/core.hpp>

namespace Anki {
  
  // Forward declaration:
  namespace Vision {
    class TrackedFace;
  }
  
namespace Cozmo {

  class ProceduralFaceUtil
  {
  public:
    // Closes eyes and switches interlacing. Call until it returns false, which
    // indicates there are no more blink frames and the face is back in its
    // original state. The output "offset" indicates the desired timing since
    // the previous state.
    static bool GetNextBlinkFrame(ProceduralFace& _faceData, TimeStamp_t& offset);
    
    // Actually draw the face with the current parameters
    static cv::Mat_<u8> GetFace(const ProceduralFace& _faceData);
    
  private:
    
    using Parameter = ProceduralEyeParameter;
    using WhichEye = ProceduralFace::WhichEye;
    using Value = ProceduralFace::Value;
    
    static void DrawEye(const ProceduralFace& _faceData, WhichEye whichEye, cv::Mat_<u8>& faceImg);
    
    static SmallMatrix<2,3,f32> GetTransformationMatrix(f32 angleDeg, f32 scaleX, f32 scaleY,
                                                        f32 tX, f32 tY, f32 x0 = 0.f, f32 y0 = 0.f);
    
    // To avoid burn-in this switches which scanlines to use (odd or even), e.g.
    // to be called each time we blink.
    static void SwitchInterlacing();
    
    static u8 _firstScanLine;
  }; // class ProceduralFace
  
  
#pragma mark Inlined Methods
  
  inline void ProceduralFaceUtil::SwitchInterlacing() {
    _firstScanLine = 1 - _firstScanLine;
  }
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFaceUtil_H__
