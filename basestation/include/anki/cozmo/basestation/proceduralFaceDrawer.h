#ifndef __Anki_Cozmo_ProceduralFaceDrawer_H__
#define __Anki_Cozmo_ProceduralFaceDrawer_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/matrix.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/vision/basestation/image.h"

namespace Anki {
  
  // Forward declaration:
  namespace Vision {
    class TrackedFace;
  }
  
namespace Cozmo {

  class ProceduralFaceDrawer
  {
  public:
    
    // Closes eyes and switches interlacing. Call until it returns false, which
    // indicates there are no more blink frames and the face is back in its
    // original state. The output "offset" indicates the desired timing since
    // the previous state.
    static bool GetNextBlinkFrame(ProceduralFace& faceData, TimeStamp_t& offset);
    
    // Actually draw the face with the current parameters
    static Vision::Image DrawFace(const ProceduralFace& faceData);
    
    // To avoid burn-in this switches which scanlines to use (odd or even), e.g.
    // to be called each time we blink.
    static void SwitchInterlacing();
    
  private:
    
    using Parameter = ProceduralEyeParameter;
    using WhichEye = ProceduralFace::WhichEye;
    using Value = ProceduralFace::Value;
    
    static void DrawEye(const ProceduralFace& faceData, WhichEye whichEye, Vision::Image& faceImg,
                        Rectangle<f32>& eyeBoundingBox);
    
    static SmallMatrix<2,3,f32> GetTransformationMatrix(f32 angleDeg, f32 scaleX, f32 scaleY,
                                                        f32 tX, f32 tY, f32 x0 = 0.f, f32 y0 = 0.f);
    
    static u8 _firstScanLine;
    
  }; // class ProceduralFace
  
  
#pragma mark Inlined Methods
  
  inline void ProceduralFaceDrawer::SwitchInterlacing() {
    _firstScanLine = 1 - _firstScanLine;
  }
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFaceDrawer_H__
