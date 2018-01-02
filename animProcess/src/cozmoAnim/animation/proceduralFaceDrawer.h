#ifndef __Anki_Cozmo_ProceduralFaceDrawer_H__
#define __Anki_Cozmo_ProceduralFaceDrawer_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/matrix.h"
#include "cozmoAnim/animation/proceduralFace.h"
#include "coretech/vision/engine/image.h"

namespace Anki {
  
  // Forward declaration:
  namespace Vision {
    class TrackedFace;
  }
 
  namespace Util {
    class RandomGenerator;
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
    static void DrawFace(const ProceduralFace& faceData, const Util::RandomGenerator& rng, Vision::ImageRGB& faceImg);
    
  private:
    
    using Parameter = ProceduralEyeParameter;
    using WhichEye = ProceduralFace::WhichEye;
    using Value = ProceduralFace::Value;
    
    // Despite taking in an ImageRGB, note that this method actually draws in HSV and
    // is just using ImageRGB as a "3 channel image" since we don't (yet) have an ImageHSV.
    // The resulting face image is converted to RGB by DrawFace at the end.
    static void DrawEye(const ProceduralFace& faceData, WhichEye whichEye,
                        const Util::RandomGenerator& rng, 
                        Vision::ImageRGB& faceImg, Rectangle<f32>& eyeBoundingBox);
    
    static SmallMatrix<2,3,f32> GetTransformationMatrix(f32 angleDeg, f32 scaleX, f32 scaleY,
                                                        f32 tX, f32 tY, f32 x0 = 0.f, f32 y0 = 0.f);
    
    static Vision::Image _glowImg;
    static Vision::Image _eyeShape;
    
    static const Array2d<f32>& GetNoiseImage(const Util::RandomGenerator& rng);
    
  }; // class ProceduralFaceπ
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFaceDrawer_H__
