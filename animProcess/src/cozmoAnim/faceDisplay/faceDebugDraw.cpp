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
#include "anki/common/basestation/array2d_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/vision/basestation/image.h"
#include "util/helpers/templateHelpers.h"
#include "clad/robotInterface/messageRobotToEngine.h"

namespace Anki {
namespace Cozmo {

FaceDebugDraw::FaceDebugDraw()
: _scratchDrawingImg(new Vision::ImageRGB())
{
  _scratchDrawingImg->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
}

void FaceDebugDraw::ChangeDrawState()
{
  constexpr auto stateCount = Util::EnumToUnderlying(DrawState::Count);
  _drawState = static_cast<DrawState>((Util::EnumToUnderlying(_drawState) + 1) % stateCount);
}

void FaceDebugDraw::DrawConfidenceClock(const RobotInterface::MicDirection& micData)
{
  if (GetDrawState() != DrawState::MicDirectionClock)
  {
    return;
  }

  Vision::ImageRGB& drawImg = *_scratchDrawingImg;
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

  // Multiplying factors (cos/sin) for the clock directions
  static const std::array<Point2f, 12> barLenFactor = 
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
    {angleFactorB, angleFactorA} // 11 o'clock
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

  FaceDisplay::getInstance()->DrawToFaceDebug(drawImg);
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
