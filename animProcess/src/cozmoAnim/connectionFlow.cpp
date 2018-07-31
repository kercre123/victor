/** File: connectionFlow.cpp
 *
 * Author: Al Chaussee
 * Created: 02/28/2018
 *
 * Description: Functions for updating what to display on the face
 *              during various parts of the connection flow
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "cozmoAnim/connectionFlow.h"

#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animComms.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/robotDataLoader.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/data/dataScope.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/image_impl.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_sendAnimToRobot_helper.h"

#include "util/console/consoleSystem.h"
#include "util/logging/logging.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

#include "anki/cozmo/shared/factory/emrHelper.h"

#include "osState/osState.h"

namespace Anki {
namespace Cozmo {

namespace {
u32 _pin = 123456;

const f32 kRobotNameScale = 0.6f;
const std::string kURL = "anki.com/v";
const ColorRGBA   kColor(0.9f, 0.9f, 0.9f, 1.f);

bool s_enteredAnyScreen = false;
}

// Draws BLE name and url to screen
bool DrawStartPairingScreen(AnimationStreamer* animStreamer)
{
  // Robot name will be empty until switchboard has set the property
  std::string robotName = OSState::getInstance()->GetRobotName();
  if(robotName == "")
  {
    return false;
  }
  
  s_enteredAnyScreen = true;  

  auto* img = new Vision::ImageRGBA(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img->FillWith(Vision::PixelRGBA(0, 0));

  img->DrawTextCenteredHorizontally(robotName, CV_FONT_NORMAL, kRobotNameScale, 1, kColor, 15, false);

  cv::Size textSize;
  float scale = 0;
  Vision::Image::MakeTextFillImageWidth(kURL, CV_FONT_NORMAL, 1, img->GetNumCols(), textSize, scale);
  img->DrawTextCenteredHorizontally(kURL, CV_FONT_NORMAL, scale, 1, kColor, (FACE_DISPLAY_HEIGHT + textSize.height)/2, true);

  auto handle = std::make_shared<Vision::SpriteWrapper>(img);
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetFaceImage(handle, shouldRenderInEyeHue, 0);

  return true;
}

// Draws BLE name, key icon, and BLE pin to screen
void DrawShowPinScreen(AnimationStreamer* animStreamer, const AnimContext* context, const std::string& pin)
{
  s_enteredAnyScreen = true;
  
  Vision::ImageRGB key;
  key.Load(context->GetDataLoader()->GetSpritePaths()->GetValue(Vision::SpriteName::Pairing_Icon_Key));

  auto* img = new Vision::ImageRGBA(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img->FillWith(Vision::PixelRGBA(0, 0));

  Point2f p((FACE_DISPLAY_WIDTH - key.GetNumCols())/2,
            (FACE_DISPLAY_HEIGHT - key.GetNumRows())/2);
  img->DrawSubImage(key, p);

  img->DrawTextCenteredHorizontally(OSState::getInstance()->GetRobotName(), CV_FONT_NORMAL, kRobotNameScale, 1, kColor, 15, false);

  img->DrawTextCenteredHorizontally(pin, CV_FONT_NORMAL, 0.8f, 1, kColor, FACE_DISPLAY_HEIGHT-5, false);

  auto handle = std::make_shared<Vision::SpriteWrapper>(img);
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetFaceImage(handle, shouldRenderInEyeHue, 0);
}

// Uses a png sequence animation to draw wifi icon to screen
void DrawWifiScreen(AnimationStreamer* animStreamer)
{
  s_enteredAnyScreen = true;
  
  const bool shouldInterrupt = true;
  const bool shouldOverrideEyeHue = true;
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_wifi", 0, 0, shouldInterrupt,
                                      shouldOverrideEyeHue, shouldRenderInEyeHue);
}

// Uses a png sequence animation to draw os updating icon to screen
void DrawUpdatingOSScreen(AnimationStreamer* animStreamer)
{
  s_enteredAnyScreen = true;
  
  const bool shouldInterrupt = true;
  const bool shouldOverrideEyeHue = true;
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_update", 0, 0, shouldInterrupt,
                                      shouldOverrideEyeHue, shouldRenderInEyeHue);
}

// Uses a png sequence animation to draw os updating error icon to screen
void DrawUpdatingOSErrorScreen(AnimationStreamer* animStreamer)
{
  s_enteredAnyScreen = true;
  
  const bool shouldInterrupt = true;
  const bool shouldOverrideEyeHue = true;
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_update_error", 0, 0, shouldInterrupt,
                                      shouldOverrideEyeHue, shouldRenderInEyeHue);
}

// Uses a png sequence animation to draw waiting for app icon to screen
void DrawWaitingForAppScreen(AnimationStreamer* animStreamer)
{
  s_enteredAnyScreen = true;
  
  const bool shouldInterrupt = true;
  const bool shouldOverrideEyeHue = true;
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_awaitingapp", 0, 0, shouldInterrupt,
                                      shouldOverrideEyeHue, shouldRenderInEyeHue);
}

void SetBLEPin(uint32_t pin)
{
  _pin = pin;
}

bool InitConnectionFlow(AnimationStreamer* animStreamer)
{
  if(FACTORY_TEST)
  {
    // Don't start connection flow if not packed out
    if(!Factory::GetEMR()->fields.PACKED_OUT_FLAG)
    {
      return true;
    }

    return DrawStartPairingScreen(animStreamer);
  }
  return true;
}

void UpdateConnectionFlow(const SwitchboardInterface::SetConnectionStatus& msg,
                          AnimationStreamer* animStreamer,
                          const AnimContext* context)
{
  using namespace SwitchboardInterface;

  PRINT_NAMED_INFO("ConnectionFlow.UpdateConnectionFlow.NewStatus", "%s", EnumToString(msg.status));
  
  // isPairing is a proxy for "switchboard is doing something and needs to display something on face"
  const bool isPairing = msg.status != ConnectionStatus::NONE &&
                         msg.status != ConnectionStatus::COUNT &&
                         msg.status != ConnectionStatus::END_PAIRING;

  // Enable pairing screen if status is anything besides NONE, COUNT, and END_PAIRING
  // Should do nothing if called multiple times with same argument such as when transitioning from
  // START_PAIRING to SHOW_PRE_PIN
  FaceInfoScreenManager::getInstance()->EnablePairingScreen(isPairing);

  // Disable face keepalive, but don't re-enable it when ending pairing. The engine will send a message
  // when it's ready to re-enable it, since it needs time to send its first animation upon resuming
  if (isPairing) {
    animStreamer->Abort();
    animStreamer->EnableKeepFaceAlive(false, 0);

    // Always look up since we're displaying something that user will want to see
    RobotInterface::SetHeadAngle msg;
    msg.angle_rad             = MAX_HEAD_ANGLE;
    msg.max_speed_rad_per_sec = DEG_TO_RAD(60);
    msg.accel_rad_per_sec2    = DEG_TO_RAD(360);
    msg.duration_sec          = 0;
    msg.actionID              = 0;
    SendAnimToRobot(std::move(msg));
  }


  switch(msg.status)
  {
    case ConnectionStatus::NONE:
    {

    }
    break;
    case ConnectionStatus::START_PAIRING:
    {
      DrawStartPairingScreen(animStreamer);
    }
    break;
    case ConnectionStatus::SHOW_PRE_PIN:
    {
      DrawShowPinScreen(animStreamer, context, "######");
    }
    break;
    case ConnectionStatus::SHOW_PIN:
    {
      DrawShowPinScreen(animStreamer, context, std::to_string(_pin));
    }
    break;
    case ConnectionStatus::SETTING_WIFI:
    {
      DrawWifiScreen(animStreamer);
     }
    break;
    case ConnectionStatus::UPDATING_OS:
    {
      DrawUpdatingOSScreen(animStreamer);
    }
    break;
    case ConnectionStatus::UPDATING_OS_ERROR:
    {
      DrawUpdatingOSErrorScreen(animStreamer);
    }
    break;
    case ConnectionStatus::WAITING_FOR_APP:
    {
      DrawWaitingForAppScreen(animStreamer);
    }
    break;
    case ConnectionStatus::END_PAIRING:
    {
      if(s_enteredAnyScreen)
      {
        animStreamer->Abort();
      }
      s_enteredAnyScreen = false;
      

      // Probably will never get here because we will restart
      // while updating os
      if(FACTORY_TEST)
      {
        DrawStartPairingScreen(animStreamer);
      }
    }
    break;
    case ConnectionStatus::COUNT:
    {

    }
    break;
  }
}

}
}
