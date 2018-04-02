/**
* File: faceDebugDraw.cpp
*
* Author: Lee Crippen
* Created: 12/19/2017
*
* Description: Handles navigation and drawing of the Customer Support Menu / Debug info screens.
*
* Usage: Add drawing functionality as needed from various components. 
*        Add a corresponding ScreenName in faceInfoScreenTypes.h.
*        In the new drawing functionality, return early if the ScreenName does not match appropriately.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/connectionFlow.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceInfoScreen.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/engine/image.h"
#include "util/console/consoleInterface.h"
#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/internetUtils/internetUtils.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot_sendAnimToRobot_helper.h"
#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"
#include "webServerProcess/src/webService.h"

#include "json/json.h"
#include "osState/osState.h"

#include "anki/cozmo/shared/factory/emrHelper.h"

#include <chrono>

#ifndef SIMULATOR
#include <linux/reboot.h>
#include <sys/reboot.h>
#endif


// Log options
#define LOG_CHANNEL    "FaceInfoScreenManager"

// Remove this when BLE switchboard is working
#define FORCE_TRANSITION_TO_PAIRING 0

namespace Anki {
namespace Cozmo {

// Default values for text rendering
const Point2f FaceInfoScreenManager::kDefaultTextStartingLoc_pix = {0,10};
const u32 FaceInfoScreenManager::kDefaultTextSpacing_pix = 11;
const f32 FaceInfoScreenManager::kDefaultTextScale = 0.4f;

namespace {
  // Number of tics that a wheel needs to be moving for before it registers
  // as a signal to move the menu cursor
  const u32 kMenuCursorMoveCountThresh = 10;
  
  const f32 kWheelMotionThresh_mmps = 3.f;
  
  const f32 kMenuLiftLowThresh_rad  = DEG_TO_RAD(-5);
  const f32 kMenuLiftHighThresh_rad = DEG_TO_RAD(40);
  
  const f32 kMenuHeadLowThresh_rad  = DEG_TO_RAD(-15);
  const f32 kMenuHeadHighThresh_rad = DEG_TO_RAD(40);
  
  // Amount of time the backpack button must be held before sync() is called
  const f32 kButtonHoldTimeForSync_s = 1.f;
  
  // Time at which sync() should be called
  f32 syncTime_s = 0.f; 
}

  
FaceInfoScreenManager::FaceInfoScreenManager()
: _scratchDrawingImg(new Vision::ImageRGB565())
, _wheelMovingForwardsCount(0)
, _wheelMovingBackwardsCount(0)
, _liftTriggerReady(false)
, _headTriggerReady(false)
, _debugInfoScreensUnlocked(false)
, _currScreen(nullptr)
// , _webService(nullptr)
{
  _scratchDrawingImg->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);

  memset(&_customText, 0, sizeof(_customText));
}


void FaceInfoScreenManager::Init(AnimContext* context, AnimationStreamer* animStreamer)
{
  DEV_ASSERT(context != nullptr, "FaceInfoScreenManager.Init.NullContext");

  // allow us to send debug info out to the web server
  // _webService = context->GetWebService();
  
  #define ADD_SCREEN(name, gotoScreen) \
    _screenMap.emplace(std::piecewise_construct, \
                       std::forward_as_tuple(ScreenName::name), \
                       std::forward_as_tuple(ScreenName::name, ScreenName::gotoScreen));
  
  #define ADD_SCREEN_WITH_TEXT(name, gotoScreen, staticText) \
  { \
    std::vector<std::string> temp = staticText; \
    _screenMap.emplace(std::piecewise_construct, \
                       std::forward_as_tuple(ScreenName::name), \
                       std::forward_as_tuple(ScreenName::name, ScreenName::gotoScreen, temp)); \
  }
  
  #define ADD_MENU_ITEM(screen, itemText, gotoScreen) \
    GetScreen(ScreenName::screen)->AppendMenuItem(itemText, ScreenName::gotoScreen);

  #define ADD_MENU_ITEM_WITH_ACTION(screen, itemText, action) \
    GetScreen(ScreenName::screen)->AppendMenuItem(itemText, action);

  #define SET_TIMEOUT(screen, timeout_sec, timeoutScreen) \
    GetScreen(ScreenName::screen)->SetTimeout(timeout_sec, ScreenName::timeoutScreen);

  #define DISABLE_TIMEOUT(screen) \
    GetScreen(ScreenName::screen)->SetTimeout(0.f, ScreenName::screen);


  // Screens we don't want users to have access to
  // * Microphone visualization
  // * Camera
  //const bool hideSpecialDebugScreens = (FACTORY_TEST && Factory::GetEMR()->fields.PLAYPEN_PASSED_FLAG) || !ANKI_DEV_CHEATS;  // TODO: Use this line in master
  const bool hideSpecialDebugScreens = (FACTORY_TEST && Factory::GetEMR()->fields.PLAYPEN_PASSED_FLAG);                        // Use this line in factory branch

  ADD_SCREEN_WITH_TEXT(Recovery, Recovery, {"RECOVERY MODE"});
  ADD_SCREEN(None, None);
  ADD_SCREEN(Pairing, Pairing);
  ADD_SCREEN(FAC, None);
  ADD_SCREEN(CustomText, None);
  ADD_SCREEN(Main, Network);
  ADD_SCREEN_WITH_TEXT(ClearUserData, Main, {"CLEAR USER DATA?"});
  ADD_SCREEN_WITH_TEXT(ClearUserDataFail, Main, {"CLEAR USER DATA FAILED"});
  ADD_SCREEN_WITH_TEXT(Rebooting, Rebooting, {"REBOOTING..."});
  ADD_SCREEN_WITH_TEXT(SelfTest, Main, {"START SELF TEST?"});
  ADD_SCREEN(Network, SensorInfo);
  ADD_SCREEN(SensorInfo, IMUInfo);
  ADD_SCREEN(IMUInfo, MotorInfo);
  ADD_SCREEN(MotorInfo, MicInfo);

  if (hideSpecialDebugScreens) {
    ADD_SCREEN(MicInfo, Main); // Last screen cycles back to Main
  } else {
    ADD_SCREEN(MicInfo, MicDirectionClock);
  }

  ADD_SCREEN(MicDirectionClock, Camera);  
  ADD_SCREEN(Camera, Main);    // Last screen cycles back to Main

  // Recovery screen
  FaceInfoScreen::MenuItemAction rebootAction = [this]() {
    LOG_INFO("FaceInfoScreenManager.Recovery.Rebooting", "");
    this->Reboot();

    return ScreenName::Rebooting;
  };
  ADD_MENU_ITEM_WITH_ACTION(Recovery, "EXIT", rebootAction);
  ADD_MENU_ITEM(Recovery, "CONTINUE", None);
  DISABLE_TIMEOUT(Recovery);

  // None screen 
#if FACTORY_TEST  
  FaceInfoScreen::ScreenAction drawInitConnectionScreen = [animStreamer]() {
    InitConnectionFlow(animStreamer);
  };
  GetScreen(ScreenName::None)->SetEnterScreenAction(drawInitConnectionScreen);
#endif  
  
  // FAC screen
  DISABLE_TIMEOUT(FAC);

  // Pairing screen
  // Never timeout. Let switchboard handle timeouts.
  DISABLE_TIMEOUT(Pairing);

  // Main screen menu
  ADD_MENU_ITEM(Main, "EXIT", None);
  // ADD_MENU_ITEM(Main, "Self Test", SelfTest);   // TODO: VIC-1498
  ADD_MENU_ITEM(Main, "CLEAR USER DATA", ClearUserData);
  
  // Self test screen
  ADD_MENU_ITEM(SelfTest, "EXIT", Main);
  ADD_MENU_ITEM(SelfTest, "CONFIRM", Main);        // TODO: VIC-1498
  
  // Clear User Data menu
  FaceInfoScreen::MenuItemAction confirmClearUserData = [this]() {
    // Write this file to indicate that the data partition should be wiped on reboot
    if (!Util::FileUtils::WriteFile("/run/wipe-data", "1")) {
      LOG_WARNING("FaceInfoSCreenManager.ClearUserData.Failed", "");
      return ScreenName::ClearUserDataFail;
    }

    // Reboot robot for clearing to take effect
    LOG_INFO("FaceInfoScreenManager.ClearUserData.Rebooting", "");
    this->Reboot();
    return ScreenName::Rebooting;
  };
  ADD_MENU_ITEM(ClearUserData, "EXIT", Main);
  ADD_MENU_ITEM_WITH_ACTION(ClearUserData, "CONFIRM", confirmClearUserData);
  SET_TIMEOUT(ClearUserDataFail, 2.f, Main);
  
  // Camera screen
  FaceInfoScreen::ScreenAction cameraEnterAction = [animStreamer]() {
    StreamCameraImages m;
    m.enable = true;
    RobotInterface::SendAnimToEngine(std::move(m));
    animStreamer->RedirectFaceImagesToDebugScreen(true);
  };
  FaceInfoScreen::ScreenAction cameraExitAction = [animStreamer]() {
    StreamCameraImages m;
    m.enable = false;
    RobotInterface::SendAnimToEngine(std::move(m));
    animStreamer->RedirectFaceImagesToDebugScreen(false);
  };
  GetScreen(ScreenName::Camera)->SetEnterScreenAction(cameraEnterAction);
  GetScreen(ScreenName::Camera)->SetExitScreenAction(cameraExitAction);

  // Check if we booted in recovery mode
  if (OSState::getInstance()->IsInRecoveryMode()) {
    LOG_WARNING("FaceInfoScreenManager.Init.RecoveryModeFileFound", "Going into recovery mode");
    SetScreen(ScreenName::Recovery);
  } else {
    SetScreen(ScreenName::None);
  }
}
  
FaceInfoScreen* FaceInfoScreenManager::GetScreen(ScreenName name)
{
  auto it = _screenMap.find(name);
  DEV_ASSERT(it != _screenMap.end(), "FaceInfoScreenManager.GetScreen.ScreenNotFound");
  
  return &(it->second);
}

bool FaceInfoScreenManager::IsActivelyDrawingToScreen() const
{
  switch(GetCurrScreenName()) {
    case ScreenName::None:
    case ScreenName::Pairing:
      return false;
    default:
      return true;
  }
}

void FaceInfoScreenManager::SetShouldDrawFAC(bool draw) 
{ 
  // TODO(Al): Remove once BC is written to persistent storage and it is easy to revert robots
  // to factory firmware to rerun them through playpen
  if(!FACTORY_TEST)
  {
    return;
  }

  bool changed = (_drawFAC != draw);
  _drawFAC = draw; 

  if(changed && GetCurrScreenName() != ScreenName::Recovery) 
  { 
    if(draw)
    {
      SetScreen(ScreenName::FAC);
    }
    else
    {
      SetScreen(ScreenName::None);
    }
  }
}
  
void FaceInfoScreenManager::SetScreen(ScreenName screen)
{
  // Call ExitScreen
  // _currScreen may be null on the first call of this function
  if (_currScreen != nullptr) {
    _currScreen->ExitScreen();
  }

  _currScreen = GetScreen(screen);

  // If currScreen is null now, you probably haven't called Init yet!
  DEV_ASSERT(_currScreen != nullptr, "FaceInfoScreenManager.SetScreen.NullCurrScreen");

  // Special handling for FAC screen to takeover None screen
  if(_drawFAC && GetCurrScreenName() == ScreenName::None)
  {
    _currScreen = GetScreen(ScreenName::FAC);
  }
  
#ifndef SIMULATOR
  // Enable/Disable lift
  // (If the new screen is None, or one of the screens that's normally
  // active during playpen, then re-enable lift power)
  const bool enableLift = GetCurrScreenName() == ScreenName::None ||
                          GetCurrScreenName() == ScreenName::FAC  ||
                          GetCurrScreenName() == ScreenName::CustomText;
  RobotInterface::EnableMotorPower msg;
  msg.motorID = MotorID::MOTOR_LIFT;
  msg.enable = enableLift;
  SendAnimToRobot(std::move(msg));
#endif
  
  _scratchDrawingImg->FillWith(0);
  DrawScratch();

  LOG_INFO("FaceInfoScreenManager.SetScreen.EnteringScreen", "%d", GetCurrScreenName());
  _currScreen->EnterScreen();

  // Clear menu navigation triggers
  _headTriggerReady = false;
  _liftTriggerReady = false;
  _wheelMovingForwardsCount = 0;
  _wheelMovingBackwardsCount = 0;

  // One-shot operations for screens that don't need to be updated by Update()
  switch(GetCurrScreenName()) {
    case ScreenName::Main:
      DrawMain();
      break;
    case ScreenName::Network:
      DrawNetwork();
      break;
    case ScreenName::FAC:
      DrawFAC();
      break;
    case ScreenName::CustomText:
      DrawCustomText();
      break;
    default:
      break;
  }

}

void FaceInfoScreenManager::DrawFAC()
{
  DrawTextOnScreen({"FAC"},
                   NamedColors::BLACK,
                   (Factory::GetEMR()->fields.PLAYPEN_PASSED_FLAG ? 
                      NamedColors::GREEN : NamedColors::RED),
                   { 0, FACE_DISPLAY_HEIGHT-10 },
                   10,
                   3.f);
}
  
void FaceInfoScreenManager::DrawCameraImage(const Vision::ImageRGB565& img)
{
  if (GetCurrScreenName() != ScreenName::Camera) {
    return;
  }

  _scratchDrawingImg->SetFromImageRGB565(img);
  DrawScratch();
}

void FaceInfoScreenManager::DrawConfidenceClock(
  const RobotInterface::MicDirection& micData,
  float bufferFullPercent,
  uint32_t secondsRemaining,
  bool triggerRecognized)
{
  // since we're always sending this data to the server, let's compute the max confidence
  // values each time and send the pre-computed values to the web server so that if any
  // of these default values change the server gets them too

  const auto& confList = micData.confidenceList;
  const auto& winningIndex = micData.direction;
  auto maxCurConf = (float)micData.confidence;
  for (int i=0; i<12; ++i)
  {
    if (maxCurConf < confList[i])
    {
      maxCurConf = confList[i];
    }
  }

  // Calculate the current scale for the bars to use, based on filtered and current confidence levels
  constexpr float filteredConfScale = 2.0f;
  constexpr float confMaxDefault = 1000.f;
  static auto filteredConf = confMaxDefault;
  filteredConf = ((0.98f * filteredConf) + (0.02f * maxCurConf));
  auto maxConf = filteredConf * filteredConfScale;
  if (maxConf < maxCurConf)
  {
    maxConf = maxCurConf;
  }
  if (maxConf < confMaxDefault)
  {
    maxConf = confMaxDefault;
  }

  // pre-calc the delay time as well for use in both web server and face debug ...
  const auto maxDelayTime_ms = 2000.f * 2.f;
  const auto delayTime_ms = (int) (maxDelayTime_ms * bufferFullPercent);


  // always send web server data until we feel this is too much a perf hit
  // if (nullptr != _webService)
  // {
  //   using namespace std::chrono;

  //   // if we send this data every tick, we crash the robot;
  //   // only send the web data every X seconds
  //   static double nextWebServerUpdateTime = 0.0;
  //   const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
  //   if (currentTime > nextWebServerUpdateTime)
  //   {
  //     nextWebServerUpdateTime = currentTime + 0.150;

  //     Json::Value webData;
  //     webData["confidence"] = micData.confidence;
  //     webData["dominant"] = micData.direction;
  //     webData["maxConfidence"] = maxConf;
  //     webData["triggerDetected"] = triggerRecognized;
  //     webData["delayTime"] = delayTime_ms;

  //     Json::Value& directionValues = webData["directions"];
  //     for ( float confidence : micData.confidenceList )
  //     {
  //       directionValues.append(confidence);
  //     }

  //     const std::string moduleName = "micdata";
  //     _webService->SendToWebViz( moduleName, webData );
  //   }
  // }

  if (GetCurrScreenName() != ScreenName::MicDirectionClock)
  {
    return;
  }

  if (secondsRemaining > 0)
  {
    const auto drawText = std::string(" ") + std::to_string(secondsRemaining);
    DrawTextOnScreen({drawText}, NamedColors::WHITE, NamedColors::BLACK,
    {0, FACE_DISPLAY_HEIGHT-10}, 10, 3.f);
    return;
  }

  DEV_ASSERT(_scratchDrawingImg != nullptr, "FaceInfoScreenManager::DrawConfidenceClock.InvalidScratchImage");
  Vision::ImageRGB565& drawImg = *_scratchDrawingImg;
  const auto& clearColor = NamedColors::BLACK;
  drawImg.FillWith( {clearColor.r(), clearColor.g(), clearColor.b()} );

  const Point2i center_px = { FACE_DISPLAY_WIDTH / 2, FACE_DISPLAY_HEIGHT / 2 };
  constexpr int circleRadius_px = 40;
  constexpr int innerRadius_px = 5;
  constexpr int maxBarLen_px = circleRadius_px - innerRadius_px - 4;
  constexpr int barWidth_px = 3;
  constexpr float angleFactorA = 0.866f; // cos(30 degrees)
  constexpr float angleFactorB = 0.5f; // sin(30 degrees)
  constexpr int innerRadA_px = (int) (angleFactorA * (float)innerRadius_px); // 3
  constexpr int innerRadB_px = (int) (angleFactorB * (float)innerRadius_px); // 2
  constexpr int barWidthA_px = (int) (angleFactorA * (float)barWidth_px * 0.5f); // 1
  constexpr int barWidthB_px = (int) (angleFactorB * (float)barWidth_px * 0.5f); // 0
  constexpr int halfBarWidth_px = (int) ((float)barWidth_px * 0.5f);

  // Multiplying factors (cos/sin) for the clock directions.
  // NOTE: Needs to have the 13th value so the unknown direction dot can display properly
  static const std::array<Point2f, 13> barLenFactor = 
  {{
    {0.f, 1.f}, // 12 o'clock - in front of robot so point down
    {-angleFactorB, angleFactorA}, // 1 o'clock
    {-angleFactorA, angleFactorB}, // 2 o'clock
    {-1.f, 0.f}, // 3 o'clock
    {-angleFactorA, -angleFactorB}, // 4 o'clock
    {-angleFactorB, -angleFactorA}, // 5 o'clock
    {0.f, -1.f}, // 6 o'clock - behind robot so point up
    {angleFactorB, -angleFactorA}, // 7 o'clock
    {angleFactorA, -angleFactorB}, // 8 o'clock
    {1.f, 0.f}, // 9 o'clock
    {angleFactorA, angleFactorB}, // 10 o'clock
    {angleFactorB, angleFactorA}, // 11 o'clock
    {0.f, 0.f} // Unknown direction
  }};

  // Precalculated offsets for the center of the base of each of the direction bars,
  // to be added to the center point of the clock
  static const std::array<Point2i, 12> barBaseOffset = 
  {{
    {0, innerRadius_px}, // 12 o'clock - in front of robot, point down
    {-innerRadB_px, innerRadA_px}, // 1 o'clock
    {-innerRadA_px, innerRadB_px}, // 2 o'clock
    {-innerRadius_px, 0}, // 3 o'clock
    {-innerRadA_px, -innerRadB_px}, // 4 o'clock
    {-innerRadB_px, -innerRadA_px}, // 5 o'clock
    {0, -innerRadius_px}, // 6 o'clock - behind robot, point up
    {innerRadB_px, -innerRadA_px}, // 7 o'clock
    {innerRadA_px, -innerRadB_px}, // 8 o'clock
    {innerRadius_px, 0}, // 9 o'clock
    {innerRadA_px, innerRadB_px}, // 10 o'clock
    {innerRadB_px, innerRadA_px} // 11 o'clock
  }};

  // Precalculated offsets for the lower left and lower right points of the direction bars
  // (relative to a bar pointing at 12 o'clock on the drawn face, aka 6 o'clock relative to victor).
  static const std::array<std::array<Point2i, 2>, 12> barWidthFactor = 
  {{
    {{{halfBarWidth_px, 0}, {-halfBarWidth_px, 0}}},// 12 o'clock -  point down
    {{{barWidthA_px, barWidthB_px}, {-barWidthA_px, -barWidthB_px}}}, // 1 o'clock
    {{{barWidthB_px, barWidthA_px}, {-barWidthB_px, -barWidthA_px}}}, // 2 o'clock
    {{{0, halfBarWidth_px}, {0, -halfBarWidth_px}}}, // 3 o'clock - point left
    {{{-barWidthB_px, barWidthA_px}, {barWidthB_px, -barWidthA_px}}}, // 4 o'clock
    {{{-barWidthA_px, barWidthB_px}, {barWidthA_px, -barWidthB_px}}}, // 5 o'clock
    {{{-halfBarWidth_px, 0}, {halfBarWidth_px, 0}}}, // 6 o'clock - point up
    {{{-barWidthA_px, -barWidthB_px}, {barWidthA_px, barWidthB_px}}}, // 7 o'clock
    {{{-barWidthB_px, -barWidthA_px}, {barWidthB_px, barWidthA_px}}}, // 8 o'clock
    {{{0, -halfBarWidth_px}, {0, halfBarWidth_px}}}, // 9 o'clock - point right
    {{{barWidthB_px, -barWidthA_px}, {-barWidthB_px, barWidthA_px}}}, // 10 o'clock
    {{{barWidthA_px, -barWidthB_px}, {-barWidthA_px, barWidthB_px}}} // 11 o'clock
  }};

  // Draw the outer circle
  drawImg.DrawCircle({(float)center_px.x(), (float)center_px.y()}, NamedColors::BLUE, circleRadius_px, 2);

  // Draw each of the clock directions
  for (int i=0; i<12; ++i)
  {
    const auto baseX = center_px.x() + barBaseOffset[i].x();
    const auto baseY = center_px.y() + barBaseOffset[i].y();
    const auto dirLen = confList[i] / maxConf * (float)maxBarLen_px;
    const auto lenX = (int) (barLenFactor[i].x() * (float)dirLen);
    const auto lenY = (int) (barLenFactor[i].y() * (float)dirLen);

    drawImg.DrawFilledConvexPolygon({
      {baseX + barWidthFactor[i][0].x(), baseY + barWidthFactor[i][0].y()},
      {baseX + barWidthFactor[i][0].x() + lenX, baseY + barWidthFactor[i][0].y() + lenY},
      {baseX + barWidthFactor[i][1].x() + lenX, baseY + barWidthFactor[i][1].y() + lenY},
      {baseX + barWidthFactor[i][1].x(), baseY + barWidthFactor[i][1].y()}
      }, 
      NamedColors::BLUE);
  }

  // Draw the circle indicating the current dominant direction
  drawImg.DrawFilledCircle({
    (float) (center_px.x() + (int)(barLenFactor[winningIndex].x() * (float)(circleRadius_px + 1.f))), 
    (float) (center_px.y() + (int)(barLenFactor[winningIndex].y() * (float)(circleRadius_px + 1.f)))
    }, NamedColors::RED, 5);


  // If we have an active state flag set, draw a blue circle for it
  constexpr int activeCircleRad_px = 10;
  if (micData.activeState != 0)
  {
    drawImg.DrawFilledCircle({
      (float) FACE_DISPLAY_WIDTH - activeCircleRad_px, 
      (float) FACE_DISPLAY_HEIGHT - activeCircleRad_px
      }, NamedColors::BLUE, activeCircleRad_px);
  }

  // Display the trigger recognized symbol if needed
  constexpr int triggerDispWidth_px = 15;
  constexpr int triggerDispHeight = 16;
  constexpr int triggerOffsetFromActiveCircle_px = 20;
  if (triggerRecognized)
  {
    drawImg.DrawFilledConvexPolygon({
      {FACE_DISPLAY_WIDTH - triggerDispWidth_px, 
        FACE_DISPLAY_HEIGHT - activeCircleRad_px*2 - triggerOffsetFromActiveCircle_px},
      {FACE_DISPLAY_WIDTH - triggerDispWidth_px, 
        FACE_DISPLAY_HEIGHT - activeCircleRad_px*2 - triggerOffsetFromActiveCircle_px + triggerDispHeight},
      {FACE_DISPLAY_WIDTH, 
        FACE_DISPLAY_HEIGHT - activeCircleRad_px*2 - triggerOffsetFromActiveCircle_px + triggerDispHeight/2}
      }, 
      NamedColors::GREEN);
  }

  constexpr int endOfBarHeight_px = 20;
  constexpr int endOfBarWidth_px = 5;

  constexpr int buffFullBarHeight_px = endOfBarHeight_px / 2;
  constexpr int buffFullBarWidth_px = 52;
  bufferFullPercent = CLIP(bufferFullPercent, 0.0f, 1.0f);

  // Draw the end-of-bar line
  drawImg.DrawFilledConvexPolygon({
    {buffFullBarWidth_px, FACE_DISPLAY_HEIGHT - endOfBarHeight_px},
    {buffFullBarWidth_px, FACE_DISPLAY_HEIGHT},
    {buffFullBarWidth_px + endOfBarWidth_px, FACE_DISPLAY_HEIGHT},
    {buffFullBarWidth_px + endOfBarWidth_px, FACE_DISPLAY_HEIGHT - endOfBarHeight_px}
    }, 
    NamedColors::RED);
    
  // Draw the bar showing the mic data buffer fullness
  drawImg.DrawFilledConvexPolygon({
    {0, FACE_DISPLAY_HEIGHT - endOfBarHeight_px + buffFullBarHeight_px / 2},
    {0, FACE_DISPLAY_HEIGHT - buffFullBarHeight_px / 2},
    {(int) (bufferFullPercent * (float) buffFullBarWidth_px), FACE_DISPLAY_HEIGHT - buffFullBarHeight_px / 2},
    {(int) (bufferFullPercent * (float) buffFullBarWidth_px), FACE_DISPLAY_HEIGHT - endOfBarHeight_px + buffFullBarHeight_px / 2}
    }, 
    NamedColors::RED);
    
  const std::string confidenceString = std::to_string(micData.confidence);
  drawImg.DrawText( {0.0f, 10.0f}, confidenceString, NamedColors::WHITE, 0.5f );

  // Also draw the delay time in milliseconds
  // Copied from kRawAudioPerBuffer_ms in micDataProcessor.h
  // and doubled for 2 buffers
  const auto delayStr = std::to_string(delayTime_ms);
  const Point2f textLoc = {0.f, FACE_DISPLAY_HEIGHT - endOfBarHeight_px};
  const auto textScale = 0.5f;
  _scratchDrawingImg->DrawText(textLoc,
                               delayStr,
                               NamedColors::WHITE,
                               textScale);

  // Draw the debug page number
  DrawScratch();
}  


bool CheckForDoublePress(bool buttonReleased)
{
  // Time window in which to consider a second press as a double press
  const  u32 kDoublePressWindow_ms   = 700;
  static u32 buttonTappedCount       = 0;
  static u32 timeDoublePressStart_ms = 0;

  const u32 curTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  // If it has been too long since the first button release then
  // reset time and buttonTappedCount
  if(timeDoublePressStart_ms > 0 && 
     curTime_ms - timeDoublePressStart_ms > kDoublePressWindow_ms)
  {
    timeDoublePressStart_ms = 0;
    buttonTappedCount = 0;
  }

  // If the button has been released
  if(buttonReleased)
  {
    // If this is the first release set timeDoublePressStart
    if(buttonTappedCount == 0)
    {
      timeDoublePressStart_ms = curTime_ms;
    }

    buttonTappedCount++;
  }

  // If the button has been released twice then broadcast a double pressed message
  if(buttonTappedCount == 2)
  {
    timeDoublePressStart_ms = 0;
    buttonTappedCount = 0;
    return true;
  }

  return false;
}

void FaceInfoScreenManager::ProcessMenuNavigation(const RobotState& state)
{
  static bool buttonWasPressed = false;
  const bool buttonIsPressed = static_cast<bool>(state.status & (uint16_t)RobotStatusFlag::IS_BUTTON_PRESSED);
  const bool buttonPressedEvent = !buttonWasPressed && buttonIsPressed;
  const bool buttonReleasedEvent = buttonWasPressed && !buttonIsPressed;
  buttonWasPressed = buttonIsPressed;

  const bool isOnCharger = static_cast<bool>(state.status & (uint16_t)RobotStatusFlag::IS_ON_CHARGER);

  const auto currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Call sync() when button held nearly long enough for shutdown
  // before the power is pulled to make sure pending writes are flushed
  if (buttonPressedEvent) {
    syncTime_s = currentTime_s + kButtonHoldTimeForSync_s;
  }
  if (buttonIsPressed) {
    if (syncTime_s > 0.f && (currentTime_s > syncTime_s)) {
      sync();
      syncTime_s = 0.f;
    }
  }

  const ScreenName currScreenName = GetCurrScreenName();

  // Check for conditions to enter BLE pairing mode
  if (isOnCharger && 
      // Only enter pairing from these three screens which include
      // screens that are normally active during playpen test
      (currScreenName == ScreenName::None || 
       currScreenName == ScreenName::FAC  || 
       currScreenName == ScreenName::CustomText) && 
      CheckForDoublePress(buttonReleasedEvent)) {
    LOG_INFO("FaceInfoScreenManager.ProcessMenuNavigation.GotDoublePress", "Entering pairing");
    RobotInterface::SendAnimToEngine(SwitchboardInterface::EnterPairing());

    if (FORCE_TRANSITION_TO_PAIRING) {
      LOG_WARNING("FaceInfoScreenManager.ProcessMenuNavigation.ForcedPairing", 
                  "Remove FORCE_TRANSITION_TO_PAIRING when switchboard is working");
      SetScreen(ScreenName::Pairing);
    }
  }

  // Check for button press to go to next debug screen
  if (buttonReleasedEvent) {
    if (_debugInfoScreensUnlocked && 
        (currScreenName != ScreenName::None &&
          currScreenName != ScreenName::FAC &&
          currScreenName != ScreenName::Pairing && 
          currScreenName != ScreenName::Recovery) ) {
      SetScreen(_currScreen->GetButtonGotoScreen());
    }
  }
  
  // Check for screen timeout
  if (_currScreen->IsTimedOut()) {
    SetScreen(_currScreen->GetTimeoutScreen());
  }
  

  if (_currScreen->HasMenu()) {
    
    // Process wheel motion for moving the menu select cursor.
    // NOTE: Due to lack of quadrature encoding on the wheels
    //       when they are not actively powered the reported speed
    //       of the wheels when moved manually have a fixed sign.
    //       Left wheel is always -ve and right wheel is always +ve.
    //       Consequently, moving the left wheel in any direction
    //       moves the menu cursor down and moving the right wheel
    //       in any direction moves it up.
    const auto lWheelSpeed = state.lwheel_speed_mmps;
    const auto rWheelSpeed = state.rwheel_speed_mmps;
    if (rWheelSpeed > kWheelMotionThresh_mmps) {
      
      ++_wheelMovingForwardsCount;
      _wheelMovingBackwardsCount = 0;
      
      if (_wheelMovingForwardsCount == kMenuCursorMoveCountThresh) {
        // Move menu cursor up
        _currScreen->MoveMenuCursorUp();
        DrawScratch();
      }
      
    } else if (lWheelSpeed < -kWheelMotionThresh_mmps) {
      
      ++_wheelMovingBackwardsCount;
      _wheelMovingForwardsCount = 0;
      
      if (_wheelMovingBackwardsCount == kMenuCursorMoveCountThresh) {
        // Move menu cursor down
        _currScreen->MoveMenuCursorDown();
        DrawScratch();
      }
    }
    else {
      _wheelMovingForwardsCount = 0;
      _wheelMovingBackwardsCount = 0;
    }
  }

  if (_currScreen->HasMenu() || currScreenName == ScreenName::Pairing) {
    // Process lift motion for confirming current menu selection
    const auto liftAngle = state.liftAngle;
    if (liftAngle > kMenuLiftHighThresh_rad) {
      _liftTriggerReady = true;
    } else if (_liftTriggerReady && liftAngle < kMenuLiftLowThresh_rad) {
      // Menu item confirmed. Go to next screen.
      _liftTriggerReady = false;

      if (_currScreen->HasMenu()) {
        SetScreen(_currScreen->ConfirmMenuItemAndGetNextScreen());
      } else if (GetCurrScreenName() == ScreenName::Pairing) {
        LOG_INFO("FaceInfoScreenManager.ProcessMenuNavigation.ExitPairing", "Going to Customer Service Main from Pairing");
        RobotInterface::SendAnimToEngine(SwitchboardInterface::ExitPairing());
        SetScreen(ScreenName::Main);
      }
    }
  }

  
  // Process head motion for going from Main screen to "hidden" debug info screens
  if (currScreenName == ScreenName::Main) {
    const auto headAngle = state.headAngle;
    if (headAngle > kMenuHeadHighThresh_rad) {
      _headTriggerReady = true;
    } else if (_headTriggerReady && headAngle < kMenuHeadLowThresh_rad) {
      // Menu item confirmed. Go to first debug info screen
      _headTriggerReady = false;
      _debugInfoScreensUnlocked = true;
      LOG_INFO("FaceInfoScreenManager.ProcessMenuNavigation.DebugScreensUnlocked", "");
      DrawScratch();
    }
  }
  
}
  

ScreenName FaceInfoScreenManager::GetCurrScreenName() const 
{ 
  return _currScreen->GetName(); 
}

void FaceInfoScreenManager::Update(const RobotState& state)
{

  ProcessMenuNavigation(state);
  
  const auto currScreenName = GetCurrScreenName();
  
  switch(currScreenName) {
    case ScreenName::SensorInfo:
      DrawSensorInfo(state);
      break;
    case ScreenName::IMUInfo:
      DrawIMUInfo(state);
      break;
    case ScreenName::MotorInfo:
      DrawMotorInfo(state);
      break;
    default:
      // Other screens are either updated once when SetScreen() is called
      // or updated by their own draw functions that are called externally
      break;
  }
}

void FaceInfoScreenManager::DrawMain()
{
  std::stringstream ss;
  ss << std::hex << Factory::GetEMR()->fields.ESN;
  const std::string serialNo = "ESN: "  + ss.str();

  auto *osstate = OSState::getInstance();
  const std::string osVer    = "OS: "   + osstate->GetOSBuildVersion() + (FACTORY_TEST ? " (V3)" : "");
  const std::string ssid     = "SSID: " + osstate->GetSSID(true);

  std::string ip             = osstate->GetIPAddress();
  if (ip.empty()) {
    ip = "XXX.XXX.XXX.XXX";
  }

#if !FACTORY_TEST
  const bool hasInternet = Util::InternetUtils::HasInternet();
#endif  
  

  ColoredTextLines lines = { {serialNo}, 
                             {osVer}, 
                             {ssid}, 
#if FACTORY_TEST
                             {"IP: " + ip},
#else
                             { {"IP: "}, {ip, (hasInternet ? NamedColors::GREEN : NamedColors::RED)} }
#endif                             
                           };
   
  DrawTextOnScreen(lines);
}


void FaceInfoScreenManager::DrawNetwork()
{
  auto osstate = OSState::getInstance();
  const std::string ble      = "BLE ID: " + osstate->GetRobotName();
  const std::string mac      = "MAC: "  + osstate->GetMACAddress();
  const std::string ssid     = "SSID: " + osstate->GetSSID(true);

  std::string ip             = osstate->GetIPAddress();
  if (ip.empty()) {
    ip = "XXX.XXX.XXX.XXX";
  }

#if !FACTORY_TEST
  const bool hasInternet = Util::InternetUtils::HasInternet();

  // TODO (VIC-1816): Check actual hosts for connectivity
  const bool hasAuthAccess  = false;
  const bool hasOTAAccess   = false;
  const bool hasVoiceAccess = Util::InternetUtils::CanConnectToHostName("chipper-dev.api.anki.com", 443);

  const ColoredText online("ONLINE", NamedColors::GREEN);
  const ColoredText offline("UNAVAILABLE", NamedColors::RED);

  const ColoredText authStatus  = hasAuthAccess  ? online : offline;
  const ColoredText otaStatus   = hasOTAAccess   ? online : offline;
  const ColoredText voiceStatus = hasVoiceAccess ? online : offline;
#endif

  ColoredTextLines lines = { {ble}, 
                             {mac}, 
                             {ssid}, 
#if FACTORY_TEST
                             {"IP: " + ip},
#else                             
                             { {"IP: "}, {ip, (hasInternet ? NamedColors::GREEN : NamedColors::RED)} },
#endif                             
                             { },

                             // TODO: VIC-1816
                            //  { {"AUTH: "}, authStatus },
                            //  { {"OTA: "}, otaStatus },
                            //  { {"VOICE: "}, voiceStatus }
                           };

  DrawTextOnScreen(lines);
}


void FaceInfoScreenManager::DrawSensorInfo(const RobotState& state)
{
  char temp[32] = "";
  sprintf(temp,
          "CLIFF: %4u %4u %4u %4u",
          state.cliffDataRaw[0],
          state.cliffDataRaw[1],
          state.cliffDataRaw[2],
          state.cliffDataRaw[3]);
  const std::string cliffs = temp;

  
  sprintf(temp,
          "DIST:   %3umm",
          state.proxData.distance_mm);
  const std::string prox1 = temp;
  
  sprintf(temp,
          "        (%2.1f %2.1f %3.f)",
          state.proxData.signalIntensity,
          state.proxData.ambientIntensity,
          state.proxData.spadCount);
  const std::string prox2 = temp;

  
  sprintf(temp,
          "TOUCH: %u",
          state.backpackTouchSensorRaw);
  const std::string touch = temp;

  sprintf(temp,
          "BATT:  %0.2fV",
          state.batteryVoltage);
  const std::string batt = temp;

  sprintf(temp,
          "TEMP:  %uC",
          OSState::getInstance()->GetTemperature_C());
  const std::string tempC = temp;
  

  DrawTextOnScreen({cliffs, prox1, prox2, touch, batt, tempC});
}

void FaceInfoScreenManager::DrawIMUInfo(const RobotState& state)
{
  char temp[32] = "";
  sprintf(temp,
          "%*.0f %*.2f",
          8,
          state.accel.x,
          8,
          state.gyro.x);
  const std::string accelGyroX = temp;

  sprintf(temp,
          "%*.2f %*.2f",
          8,
          state.accel.y,
          8,
          state.gyro.y);
  const std::string accelGyroY = temp;

  sprintf(temp,
          "%*.2f %*.2f",
          8,
          state.accel.z,
          8,
          state.gyro.z);
  const std::string accelGyroZ = temp;

  DrawTextOnScreen({"ACC        GYRO", accelGyroX, accelGyroY, accelGyroZ});
}

void FaceInfoScreenManager::DrawMotorInfo(const RobotState& state)
{
  char temp[32] = "";
  sprintf(temp, "HEAD:   %3.1f deg", RAD_TO_DEG(state.headAngle));
  const std::string head = temp;
  
  sprintf(temp, "LIFT:   %3.1f deg", RAD_TO_DEG(state.liftAngle));
  const std::string lift = temp;

  sprintf(temp, "LSPEED: %3.1f mm/s", state.lwheel_speed_mmps);
  const std::string lSpeed = temp;

  sprintf(temp, "RSPEED: %3.1f mm/s", state.rwheel_speed_mmps);
  const std::string rSpeed = temp;
  
  DrawTextOnScreen({head, lift, lSpeed, rSpeed});
}


void FaceInfoScreenManager::DrawMicInfo(const RobotInterface::MicData& micData)
{
  if(GetCurrScreenName() != ScreenName::MicInfo)
  {
    return;
  }

  char temp[32] = "";
  sprintf(temp, 
          "%d", 
          micData.data[0]);
  const std::string micData0 = temp;

  sprintf(temp, 
          "%d", 
          micData.data[1]);
  const std::string micData1 = temp;

  sprintf(temp, 
          "%d", 
          micData.data[2]);
  const std::string micData2 = temp;

  sprintf(temp, 
          "%d", 
          micData.data[3]);
  const std::string micData3 = temp;

  DrawTextOnScreen({"MICS", micData0, micData1, micData2, micData3});
}

void FaceInfoScreenManager::SetCustomText(const RobotInterface::DrawTextOnScreen& text)
{ 
  _customText = text;

  if(text.drawNow)
  {
    SetScreen(ScreenName::CustomText);
  }
}

void FaceInfoScreenManager::DrawCustomText()
{
  DrawTextOnScreen({std::string(_customText.text,
                                _customText.text_length)},
                   ColorRGBA(_customText.textColor.r,
                             _customText.textColor.g,
                             _customText.textColor.b),
                   ColorRGBA(_customText.bgColor.r,
                             _customText.bgColor.g,
                             _customText.bgColor.b),
                   { 0, FACE_DISPLAY_HEIGHT-10 }, 10, 3.f);
}

// Draws each element of the textVec on a separate line (spacing determined by textSpacing_pix)
// in textColor with a background of bgColor.
void FaceInfoScreenManager::DrawTextOnScreen(const std::vector<std::string>& textVec, 
                                    const ColorRGBA& textColor,
                                    const ColorRGBA& bgColor,
                                    const Point2f& loc,
                                    u32 textSpacing_pix,
                                    f32 textScale)
{
  _scratchDrawingImg->FillWith( {bgColor.r(), bgColor.g(), bgColor.b()} );

  const f32 textLocX = loc.x();
  f32 textLocY = loc.y();
  // TODO: Expose line and location(?) as arguments
  const u8  textLineThickness = 8;

  for(const auto& text : textVec)
  {
    _scratchDrawingImg->DrawText(
      {textLocX, textLocY},
      text.c_str(),
      textColor,
      textScale,
      textLineThickness);

    textLocY += textSpacing_pix;
  }

  DrawScratch();
}

void FaceInfoScreenManager::DrawTextOnScreen(const ColoredTextLines& lines, 
                                             const ColorRGBA& bgColor,
                                             const Point2f& loc,
                                             u32 textSpacing_pix,
                                             f32 textScale)
{
  _scratchDrawingImg->FillWith( {bgColor.r(), bgColor.g(), bgColor.b()} );

  const u8  textLineThickness = 8;

  f32 textLocY = loc.y();
  for(const auto& line : lines) 
  {
    f32 textLocX = loc.x();
    for(const auto& coloredText : line)
    {
      _scratchDrawingImg->DrawText(
        {textLocX, textLocY},
        coloredText.text.c_str(),
        coloredText.color,
        textScale,
        textLineThickness);

      auto bbox = _scratchDrawingImg->GetTextSize(coloredText.text.c_str(), textScale, textLineThickness);
      textLocX += bbox.x();
    }
    textLocY += textSpacing_pix;
  }

  DrawScratch();
}


void FaceInfoScreenManager::EnablePairingScreen(bool enable)
{
  if (enable && GetCurrScreenName() != ScreenName::Pairing) {
    LOG_INFO("FaceInfoScreenManager.EnablePairingScreen.Enable", "");
    SetScreen(ScreenName::Pairing);
  } else if (!enable && GetCurrScreenName() == ScreenName::Pairing) {
    LOG_INFO("FaceInfoScreenManager.EnablePairingScreen.Disable", "");
    SetScreen(ScreenName::None);
  }
}

void FaceInfoScreenManager::DrawScratch()
{
  _currScreen->DrawMenu(*_scratchDrawingImg);

  // Draw white pixel in top-right corner of main customer support screen
  // to indicate that debug screens are unlocked
  const bool drawDebugScreensEnabledPixel = _debugInfoScreensUnlocked &&
                                            GetCurrScreenName() == ScreenName::Main;
  if (drawDebugScreensEnabledPixel) {
    Rectangle<f32> rect(FACE_DISPLAY_WIDTH - 2, 0, 2, 2);
    _scratchDrawingImg->DrawFilledRect(rect, NamedColors::WHITE);
  }
  
  FaceDisplay::getInstance()->DrawToFaceDebug(*_scratchDrawingImg);
}

void FaceInfoScreenManager::Reboot()
{
#ifdef SIMULATOR
  LOG_WARNING("FaceInfoScreenManager.Reboot.NotSupportInSimulator", "");
#else
  
  // Need to call reboot in forked process for some reason.
  // Otherwise, reboot doesn't actually happen.
  // Also useful for transitioning to "REBOOTING..." screen anyway.
  sync(); sync(); sync(); // Linux voodoo
  pid_t pid = fork();
  if (pid == 0)
  {
    // child process
    execl("/bin/systemctl", "reboot", 0);  // Graceful reboot
  }
  else if (pid > 0)
  {
    // parent process
    LOG_INFO("FaceInfoScreenManager.Reboot.Rebooting", "");
  }
  else 
  {
    // fork failed
    LOG_WARNING("FaceInfoScreenManager.Reboot.Failed", "");
  }

#endif
}

} // namespace Cozmo
} // namespace Anki
