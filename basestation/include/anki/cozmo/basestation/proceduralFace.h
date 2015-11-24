#ifndef __Anki_Cozmo_ProceduralFace_H__
#define __Anki_Cozmo_ProceduralFace_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/matrix.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/cozmo/basestation/proceduralFaceParams.h"
#include "clad/types/proceduralEyeParameters.h"

#include <opencv2/core/core.hpp>

#include <array>

namespace Anki {
  
  // Forward declaration:
  namespace Vision {
    class TrackedFace;
  }
  
namespace Cozmo {

  class ProceduralFace
  {
  public:
    static const int WIDTH  = FaceAnimationManager::IMAGE_WIDTH;
    static const int HEIGHT = FaceAnimationManager::IMAGE_HEIGHT;
 
    // Nominal positions/sizes for everything (these are things that aren't
    // parameterized at dynamically, but could be if we want)
    static constexpr s32   NominalEyeHeight      = 40;
    static constexpr s32   NominalEyeWidth       = 30;
    
    using Parameter = ProceduralEyeParameter;
    using WhichEye = ProceduralFaceParams::WhichEye;
    using Value = ProceduralFaceParams::Value;
    
    ProceduralFace();
    
    // Reset parameters to their nominal values
    void Reset();
    
    // Closes eyes and switches interlacing. Call until it returns false, which
    // indicates there are no more blink frames and the face is back in its
    // original state. The output "offset" indicates the desired timing since
    // the previous state.
    bool GetNextBlinkFrame(TimeStamp_t& offset);
    
    // Actually draw the face with the current parameters
    cv::Mat_<u8> GetFace() const;
    
    void MimicHumanFace(const Vision::TrackedFace& trackedFace);

    bool HasBeenSentToRobot() const;
    void MarkAsSentToRobot(bool tf);
    
    TimeStamp_t GetTimeStamp() const;
    void SetTimeStamp(TimeStamp_t t);
    
    // To avoid burn-in this switches which scanlines to use (odd or even), e.g.
    // to be called each time we blink.
    static void SwitchInterlacing();
    
    const ProceduralFaceParams& GetParams() const { return _faceData; }
    ProceduralFaceParams& GetParams() { return _faceData; }
    void SetParams(const ProceduralFaceParams& newData) { _faceData = newData; }
    void SetParams(ProceduralFaceParams&& newData) { _faceData = std::move(newData); }
    
    // REMOVE ONCE WE MERGE FROM LEE's BRANCH!!
    void Combine(const ProceduralFace& otherFace) { }
    
  private:

    void DrawEye(ProceduralFaceParams::WhichEye whichEye, cv::Mat_<u8>& faceImg) const;
    
    static SmallMatrix<2,3,f32> GetTransformationMatrix(f32 angleDeg, f32 scaleX, f32 scaleY,
                                                        f32 tX, f32 tY, f32 x0 = 0.f, f32 y0 = 0.f);
    ProceduralFaceParams _faceData;
    static u8 _firstScanLine;
    bool _sentToRobot;
    TimeStamp_t _timestamp;
  }; // class ProceduralFace
  
  
#pragma mark Inlined Methods
  
  inline bool ProceduralFace::HasBeenSentToRobot() const {
    return _sentToRobot;
  }
  
  inline void ProceduralFace::MarkAsSentToRobot(bool tf) {
    _sentToRobot = tf;
  }
  
  inline TimeStamp_t ProceduralFace::GetTimeStamp() const {
    return _timestamp;
  }
  
  inline void ProceduralFace::SetTimeStamp(TimeStamp_t t) {
    _timestamp = t;
  }
  
  inline void ProceduralFace::SwitchInterlacing() {
    _firstScanLine = 1 - _firstScanLine;
  }
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFace_H__
