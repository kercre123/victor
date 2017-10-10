/**
 * File: setFaceAction.h
 *
 * Author: Andrew Stein
 * Date:   9/12/2016
 *
 *
 * Description: Defines an action for setting a procedural or bitmap face on Cozmo.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Cozmo_Actions_SetFaceAction_H__
#define __Anki_Cozmo_Actions_SetFaceAction_H__

#include "engine/actions/actionInterface.h"

#include "anki/vision/basestation/image.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

// TODO: SetFaceAction is broken (ProceduralFace and Animation only exist in animation process) (VIC-360)
  
class SetFaceAction : public IAction
{
public:
  
  // Hold the specified face for the specified duration. Use u32_MAX to loop continuously.
  // (NOTE: For continuous looping, scanlines will be auto-switched automatically every 30sec to avoid screen burn-in)
  SetFaceAction(Robot& robot, const Vision::Image& faceImage, u32 duration_ms);
  //SetFaceAction(Robot& robot, const ProceduralFace& procFace, u32 duration_ms);
  
  virtual ~SetFaceAction();
  
  // Base the timeout on the requested duration of the face animation, plus some slop.
  // If looping continously, timeout is effectively infinite
  virtual f32 GetTimeoutInSeconds() const override;
  
protected:
  
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;

private:
  
  Vision::Image                  _faceImage;
  //ProceduralFace                 _procFace;
  //Animation                      _animation;
  std::unique_ptr<IActionRunner> _playAnimationAction  = nullptr;
  u32                            _duration_ms;
  bool                           _loopContinuously;
  
}; // class SetFaceAction


} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Actions_SetFaceAction_H__ */
