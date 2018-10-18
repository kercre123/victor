/**
 * File: mirrorModeManager.cpp
 *
 * Author: Andrew Stein
 * Date:   09/28/2018
 *
 * Description: Handles creating a "MirrorMode" image for displaying the camera feed on the robot's face,
 *              along with various detections in a VisionProcessingResult.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/engine/image_impl.h"
#include "engine/vision/mirrorModeManager.h"
#include "engine/vision/visionSystem.h"
#include "util/console/consoleInterface.h"

#include <iomanip>

namespace Anki {
namespace Vector {
  
namespace {
  
  // If > 0, displays detected marker names in Viz Camera Display (still at fixed scale) and
  // and in mirror mode (at specified scale)
  CONSOLE_VAR_RANGED(f32,  kDisplayMarkerNamesScale,    "Vision.MirrorMode", 0.f, 0.f, 1.f);
  CONSOLE_VAR(bool, kDisplayDetectionsInMirrorMode,     "Vision.MirrorMode", true); // objects, faces, markers
  CONSOLE_VAR(bool, kDisplayExposureInMirrorMode,       "Vision.MirrorMode", true);
  CONSOLE_VAR(f32,  kMirrorModeGamma,                   "Vision.MirrorMode", 1.f);
  CONSOLE_VAR(s32,  kDrawMirrorModeSalientPointsFor_ms, "Vision.MirrorMode", 0);
  
  // TODO: Figure out the original image resolution? This just assumes "Default" for marker/face detection
  constexpr f32 kXmax = (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;
  constexpr f32 kHeightScale = (f32)FACE_DISPLAY_HEIGHT / (f32)DEFAULT_CAMERA_RESOLUTION_HEIGHT;
  constexpr f32 kWidthScale  = (f32)FACE_DISPLAY_WIDTH / (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MirrorModeManager::MirrorModeManager()
: _screenImg(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH)
, _gammaLUT{}
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static Rectangle<f32> DisplayMirroredRectHelper(f32 x_topLeft, f32 y_topLeft, f32 width, f32 height)
{
  const f32 x_topRight = x_topLeft + width; // will become upper left after mirroring
  const Rectangle<f32> rect((f32)FACE_DISPLAY_WIDTH - kWidthScale*x_topRight, // mirror rectangle for display
                            y_topLeft * kHeightScale,
                            width * kWidthScale,
                            height * kHeightScale);
  
  return rect;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
static inline Point<2,T> MirrorPointHelper(const Point<2,T>& pt)
{
  const Point<2,T> pt_mirror(kWidthScale*(kXmax - pt.x()), pt.y()*kHeightScale);
  return pt_mirror;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static Quad2f DisplayMirroredQuadHelper(const Quad2f& quad)
{
  // Mirror x coordinates, swap left/right points, and scale for each point in the quad:
  const Quad2f quad_mirrored(MirrorPointHelper(quad.GetTopRight()),
                             MirrorPointHelper(quad.GetBottomRight()),
                             MirrorPointHelper(quad.GetTopLeft()),
                             MirrorPointHelper(quad.GetBottomLeft()));
  
  return quad_mirrored;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
static Polygon<2,T> DisplayMirroredPolyHelper(const Polygon<2,T>& poly)
{
  Polygon<2,T> poly_mirrored;
  for(auto & pt : poly)
  {
    poly_mirrored.emplace_back(MirrorPointHelper(pt));
  }
  
  return poly_mirrored;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MirrorModeManager::DrawVisionMarkers(const std::list<Vision::ObservedMarker>& visionMarkers)
{
  for(auto const& visionMarker : visionMarkers)
  {
    const ColorRGBA& drawColor = (visionMarker.GetCode() == Vision::MARKER_UNKNOWN ?
                                  NamedColors::BLUE : NamedColors::RED);
    
    const auto& quad = visionMarker.GetImageCorners();
    const auto& name = std::string(visionMarker.GetCodeName());
    
    _screenImg.DrawQuad(DisplayMirroredQuadHelper(quad), drawColor, 3);
    if(Util::IsFltGTZero(kDisplayMarkerNamesScale))
    {
      _screenImg.DrawText({1., _screenImg.GetNumRows()-1}, name.substr(strlen("MARKER_"),std::string::npos),
                          drawColor, kDisplayMarkerNamesScale);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MirrorModeManager::DrawFaces(const std::list<Vision::TrackedFace>& faceDetections)
{
  for(auto const& faceDetection : faceDetections)
  {
    const auto& faceID = faceDetection.GetID();
    const auto& rect = faceDetection.GetRect();
    const auto& name = faceDetection.GetName();

    _screenImg.DrawRect(DisplayMirroredRectHelper(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight()),
                 NamedColors::YELLOW, 3);
    
    std::string dispName(name.empty() ? "<unknown>" : name);
    dispName += "[" + std::to_string(faceID) + "]";
    _screenImg.DrawText({1.f, _screenImg.GetNumRows()-1}, dispName, NamedColors::YELLOW, 0.6f, true);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MirrorModeManager::DrawAutoExposure(const VisionProcessingResult& procResult)
{
  // Draw exposure and gain in the lower left of the screen
  // Use a static to keep the last params displayed even when AE wasn't run (since it doesn't run every frame)
  static std::string exposureStr = "";
  if(procResult.modesProcessed.Contains(VisionMode::AutoExposure))
  {
    const Vision::CameraParams& params = procResult.cameraParams;
    std::stringstream ss;
    ss << params.exposureTime_ms << " " << std::fixed << std::setprecision(2) << params.gain;
    exposureStr = ss.str();
  }
  const f32  kFontScale = 0.4f;
  const bool kUseDropShadow = true;
  Vec2f textSize = _screenImg.GetTextSize(exposureStr, kFontScale, 1);
  _screenImg.DrawText({_screenImg.GetNumCols() - textSize.x() - 1, _screenImg.GetNumRows()-1}, 
                      exposureStr, NamedColors::RED, kFontScale, kUseDropShadow);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MirrorModeManager::DrawSalientPoints(const std::list<Vision::SalientPoint>& salientPoints)
{
  const EngineTimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  const bool usingFixedDrawTime = (kDrawMirrorModeSalientPointsFor_ms > 0);
  
  // Remove any "expired" points to draw
  if(usingFixedDrawTime)
  {
    auto iter = _salientPointsToDraw.begin();
    while(iter != _salientPointsToDraw.end() && iter->first < currentTime_ms - kDrawMirrorModeSalientPointsFor_ms)
    {
      iter = _salientPointsToDraw.erase(iter);
    }
  }
  else
  {
    _salientPointsToDraw.clear();
  }
  
  // Update the list of points to draw
  for(auto const& salientPoint_norm : salientPoints)
  {   
    // Salient points are in normalized coordinates, so scale directly to screen image size
    auto salientPoint = salientPoint_norm;
    salientPoint.x_img *= _screenImg.GetNumCols();
    salientPoint.y_img *= _screenImg.GetNumRows();
    for(auto &pt : salientPoint.shape)
    {
      pt.x *= DEFAULT_CAMERA_RESOLUTION_WIDTH;
      pt.y *= DEFAULT_CAMERA_RESOLUTION_HEIGHT;
    }
    
    _salientPointsToDraw.emplace_back(currentTime_ms, salientPoint);
  }
  
  // Draw whatever is left in the list to draw
  s32 colorIndex = 0;
  for(auto const& salientPointToDraw : _salientPointsToDraw)
  {
    const Vision::SalientPoint& salientPoint = salientPointToDraw.second;
    
    const Poly2f poly(salientPoint.shape);
    const ColorRGBA color = (salientPoint.description.empty() ?
                             NamedColors::RED :
                             ColorRGBA::CreateFromColorIndex(colorIndex++));
    
    const std::string caption(salientPoint.description + "[" + std::to_string((s32)std::round(100.f*salientPoint.score))
                              + "] t:" + std::to_string(salientPoint.timestamp));

    std::string str(salientPoint.description);
    if(!str.empty()) {
      str += ":" + std::to_string((s32)std::round(salientPoint.score*100.f));
    }
    
    const Point2f mirroredCentroid(_screenImg.GetNumCols()-salientPoint.x_img, salientPoint.y_img);
    if(!str.empty())
    {
      const bool kDropShadow = true;
      const bool kCentered = true;
      _screenImg.DrawText(mirroredCentroid, str, NamedColors::YELLOW, 0.6f, kDropShadow, 1, kCentered);
    }
    
    _screenImg.DrawFilledCircle(mirroredCentroid, color, 3);
    _screenImg.DrawPoly(DisplayMirroredPolyHelper(poly), color, 2);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result MirrorModeManager::CreateMirrorModeImage(const Vision::ImageRGB& cameraImg,
                                                VisionProcessingResult& visionProcResult)
{
  cameraImg.Resize(_screenImg, Vision::ResizeMethod::NearestNeighbor);

  // Flip image around the y axis (before we draw anything on it)
  cv::flip(_screenImg.get_CvMat_(), _screenImg.get_CvMat_(), 1);

  if(kDisplayDetectionsInMirrorMode)
  {
    DrawVisionMarkers(visionProcResult.observedMarkers);
    DrawFaces(visionProcResult.faces);
    DrawSalientPoints(visionProcResult.salientPoints);
  }
  
  if(kDisplayExposureInMirrorMode)
  {
    DrawAutoExposure(visionProcResult);
  }
  
  // Use gamma to make it easier to see
  if(!Util::IsFltNear(_currentGamma, kMirrorModeGamma)) {
    _currentGamma = kMirrorModeGamma;
    const f32 invGamma = 1.f / _currentGamma;
    const f32 divisor = 1.f / 255.f;
    for(s32 value=0; value<256; ++value)
    {
      _gammaLUT[value] = std::round(255.f * std::powf((f32)value * divisor, invGamma));
    }
  }
  
  visionProcResult.mirrorModeImg.SetFromImageRGB(_screenImg, _gammaLUT);

  return RESULT_OK;
}
  
} // namespace Vector
} // namespace Anki
