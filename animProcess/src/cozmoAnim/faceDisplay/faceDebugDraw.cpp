/**
* File: faceDebugDraw.cpp
*
* Author: Lee Crippen
* Created: 12/19/2017
*
* Description: Handles drawing debug data to the robot's face.
* Usage: Add drawing functionality as needed from various components. Add a corresponding DrawState.
*        In the new drawing functionality, return early if the DrawState does not match appropriately.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "cozmoAnim/faceDisplay/faceDebugDraw.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/engine/image.h"
#include "micDataTypes.h"
#include "util/helpers/templateHelpers.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "webServerProcess/src/webService.h"

#include "json/json.h"
#include "osState/osState.h"

#include <chrono>

namespace Anki {
namespace Cozmo {

FaceDebugDraw::FaceDebugDraw()
: _scratchDrawingImg(new Vision::ImageRGB565())
  , _webService( nullptr )
{
  _scratchDrawingImg->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);

  memset(&_customText, 0, sizeof(_customText));
}

void FaceDebugDraw::SetShouldDrawFAC(bool draw) 
{ 
  // TODO(Al): Remove once BC is written to persistent storage and it is easy to revert robots
  // to factory firmware to rerun them through playpen
  if(!FACTORY_TEST)
  {
    return;
  }

  bool changed = (_drawFAC != draw);
  _drawFAC = draw; 

  if(changed) 
  { 
    if(draw)
    {
      _drawState = DrawState::FAC;
      DrawFAC();
    }
    else
    {
      _drawState = DrawState::None;
    }
  }
}

void FaceDebugDraw::ChangeDrawState()
{
  constexpr auto stateCount = Util::EnumToUnderlying(DrawState::Count);
  _drawState = static_cast<DrawState>((Util::EnumToUnderlying(_drawState) + 1) % stateCount);

  if(_drawFAC && _drawState == DrawState::None)
  {
    _drawState = DrawState::FAC;
  }
  else if(!_drawFAC && _drawState == DrawState::FAC)
  {
    _drawState = static_cast<DrawState>((Util::EnumToUnderlying(_drawState) + 1) % stateCount);
  }

  _scratchDrawingImg->FillWith(0);
  DrawScratch();

  // Any debug drawing that does not update very frequently should immediately try to
  // draw on state change
  DrawFAC();
  DrawCustomText();

  if(_drawState == DrawState::GeneralInfo)
  {
    // Try to update the ip address if we are transitioning to the GeneralInfo screen
    OSState::getInstance()->GetIPAddress(true);
  }
}

void FaceDebugDraw::DrawFAC()
{
  if(_drawFAC && GetDrawState() == DrawState::FAC)
  {
    DrawTextOnScreen({"FAC"},
                     NamedColors::BLACK,
                     NamedColors::RED,
                     { 0, FACE_DISPLAY_HEIGHT-10 });
  }
}

void FaceDebugDraw::DrawConfidenceClock(
  const RobotInterface::MicDirection& micData,
  float bufferFullPercent,
  uint32_t secondsRemaining,
  bool triggerRecognized)
{
  // since we're always sending this data to the server, let's compute the max confidence
  // values each time and send the pre-computed values to the web server so that if any
  // of these default values change the server gets them too

  const auto& confList = micData.confidenceList;
  const auto& winningIndex = micData.selectedDirection;
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
  const auto maxDelayTime_ms = (float) MicData::kRawAudioPerBuffer_ms;
  const auto delayTime_ms = (int) (maxDelayTime_ms * bufferFullPercent);


  // always send web server data until we feel this is too much a perf hit
  if (nullptr != _webService)
  {
    using namespace std::chrono;

    // if we send this data every tick, we crash the robot;
    // only send the web data every X seconds
    static double nextWebServerUpdateTime = 0.0;
    const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
    if (currentTime > nextWebServerUpdateTime)
    {
      nextWebServerUpdateTime = currentTime + 0.150;

      Json::Value webData;
      webData["confidence"] = micData.confidence;
      webData["dominant"] = micData.direction;
      webData["maxConfidence"] = maxConf;
      webData["triggerDetected"] = triggerRecognized;
      webData["delayTime"] = delayTime_ms;

      Json::Value& directionValues = webData["directions"];
      for ( float confidence : micData.confidenceList )
      {
        directionValues.append(confidence);
      }

      static const std::string moduleName = "micdata";
      _webService->SendToWebViz( moduleName, webData );
    }
  }

  if (GetDrawState() != DrawState::MicDirectionClock)
  {
    return;
  }

  if (secondsRemaining > 0)
  {
    const auto drawText = std::string(" ") + std::to_string(secondsRemaining);
    DrawTextOnScreen({drawText}, NamedColors::WHITE, NamedColors::BLACK);
    return;
  }

  DEV_ASSERT(_scratchDrawingImg != nullptr, "FaceDebugDraw::DrawConfidenceClock.InvalidScratchImage");
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

void FaceDebugDraw::DrawStateInfo(const RobotState& state)
{
  const auto& drawState = GetDrawState();

  if(drawState == DrawState::GeneralInfo)
  {
    const std::string ip        = OSState::getInstance()->GetIPAddress();
    const std::string serialNo  = OSState::getInstance()->GetSerialNumberAsString();
    const std::string osVer     = OSState::getInstance()->GetOSBuildVersion();
    const std::string sha       = OSState::getInstance()->GetBuildSha();
    const std::string robotName = OSState::getInstance()->GetRobotName();

    std::vector<std::string> text = {ip, serialNo, robotName, osVer, sha};

    if(FACTORY_TEST)
    {
      text.push_back("V2");
    }

    DrawTextOnScreen(text, 
                     NamedColors::WHITE, 
                     NamedColors::BLACK, 
                     {0, 30}, 
                     15,
                     0.5f);
  }
  else if(drawState == DrawState::SensorInfo1)
  {
    char temp[32] = "";
    sprintf(temp, 
            "%u %u %u %u", 
            state.cliffDataRaw[0], 
            state.cliffDataRaw[1], 
            state.cliffDataRaw[2], 
            state.cliffDataRaw[3]);
    const std::string cliffs = temp;

    sprintf(temp, 
            "%.2f %.2f %.1f %.1f", 
            state.headAngle, 
            state.liftAngle, 
            state.lwheel_speed_mmps, 
            state.rwheel_speed_mmps);
    const std::string motors = temp;

    const float batteryVolts = OSState::getInstance()->GetBatteryVoltage_uV() / 1'000'000.f;
    
    sprintf(temp, 
            "%u %0.2fv", 
            state.backpackTouchSensorRaw,
            batteryVolts);
    const std::string touchAndBat = temp;

    sprintf(temp, 
            "%.1f %.1f %.1f %u", 
            state.proxData.signalIntensity,
            state.proxData.ambientIntensity,
            state.proxData.spadCount,
            state.proxData.distance_mm);
    const std::string prox = temp;

    DrawTextOnScreen({cliffs, motors, prox, touchAndBat}, 
                     NamedColors::WHITE, 
                     NamedColors::BLACK, 
                     {0, 30}, 
                     20, 
                     0.5f);
  }
  else if(drawState == DrawState::SensorInfo2)
  {
    char temp[32] = "";
    sprintf(temp, 
            "%*.2f %*.2f", 
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

    DrawTextOnScreen({accelGyroX, accelGyroY, accelGyroZ}, 
                     NamedColors::WHITE, 
                     NamedColors::BLACK, 
                     {0, 30}, 
                     20, 
                     0.5f);
  }
}

void FaceDebugDraw::DrawMicInfo(const RobotInterface::MicData& micData)
{
  if(GetDrawState() != DrawState::MicInfo)
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

  DrawTextOnScreen({micData0, micData1, micData2, micData3}, 
                   NamedColors::WHITE, 
                   NamedColors::BLACK, 
                   {0, 30}, 
                   20, 
                   0.5f);
}

void FaceDebugDraw::SetCustomText(const RobotInterface::DrawTextOnScreen& text)
{ 
  _customText = text;

  if(text.drawNow)
  {
    _drawState = DrawState::CustomText;
  }

  DrawCustomText(); 
}

void FaceDebugDraw::DrawCustomText()
{
  if(GetDrawState() != DrawState::CustomText)
  {
    return;
  }

  DrawTextOnScreen({std::string(_customText.text,
                                _customText.text_length)},
                   ColorRGBA(_customText.textColor.r,
                             _customText.textColor.g,
                             _customText.textColor.b),
                   ColorRGBA(_customText.bgColor.r,
                             _customText.bgColor.g,
                             _customText.bgColor.b),
                   { 0, FACE_DISPLAY_HEIGHT-10 });
}

// Draws each element of the textVec on a separate line (spacing determined by textSpacing_pix)
// in textColor with a background of bgColor.
void FaceDebugDraw::DrawTextOnScreen(const std::vector<std::string>& textVec, 
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

  // Draw the word "Factory" in the top right corner if this is a 
  // factory build
  if(FACTORY_TEST)
  {
    const Point2f factoryTextLoc = {0, 10};
    const f32 factoryScale = 0.5f;
    _scratchDrawingImg->DrawText(factoryTextLoc,
                                 "Factory",
                                 NamedColors::WHITE,
                                 factoryScale);
  }

  DrawScratch();
}

void FaceDebugDraw::DrawScratch()
{
  static const std::string kDebugCount = std::to_string(Util::EnumToUnderlying(DrawState::Count) - 1);
  const std::string text = std::to_string(Util::EnumToUnderlying(_drawState)) + "/" + kDebugCount;

  const Point2f textLoc = {static_cast<float>(FACE_DISPLAY_WIDTH - (text.length()*16)), 10};
  static const f32 textScale = 0.5f;
  _scratchDrawingImg->DrawText(textLoc,
                               text,
                               NamedColors::WHITE,
                               textScale);

  FaceDisplay::getInstance()->DrawToFaceDebug(*_scratchDrawingImg);
}

void FaceDebugDraw::ClearFace()
{
  const auto& clearColor = NamedColors::BLACK;
  _scratchDrawingImg->FillWith( {clearColor.r(), clearColor.g(), clearColor.b()} );
  FaceDisplay::getInstance()->DrawToFaceDebug(*_scratchDrawingImg);
}

} // namespace Cozmo
} // namespace Anki
