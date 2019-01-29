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
#include "engine/vision/visionModesHelpers.h"
#include "engine/vision/visionSystem.h"
#include "util/console/consoleInterface.h"

#include <iomanip>

namespace Anki {
namespace Vector {
  
namespace {
  
  // If > 0, displays detected marker names in Viz Camera Display (still at fixed scale) and
  // and in mirror mode (at specified scale)
  CONSOLE_VAR_RANGED(f32,  kDisplayMarkerNamesScale,           "Vision.MirrorMode", 0.f, 0.f, 1.f);
  CONSOLE_VAR(bool,        kDisplayMarkersInMirrorMode,        "Vision.MirrorMode", true);
  CONSOLE_VAR(bool,        kDisplayFacesInMirrorMode,          "Vision.MirrorMode", true);
  // THEBOX: NOTE: disable salient point display for the box because the behavior is doing it manually
  CONSOLE_VAR(bool,        kDisplaySalientPointsInMirrorMode,  "Vision.MirrorMode", false);
  CONSOLE_VAR(bool,        kDisplayExposureInMirrorMode,       "Vision.MirrorMode", false);
  CONSOLE_VAR(bool,        kDisplayFaceNamesInMirrorMode,      "Vision.MirrorMode", true);
  CONSOLE_VAR(f32,         kMirrorModeGamma,                   "Vision.MirrorMode", 1.f);
  CONSOLE_VAR(s32,         kDrawMirrorModeSalientPointsFor_ms, "Vision.MirrorMode", 0);
  CONSOLE_VAR_RANGED(f32,  kMirrorModeFaceFontScale,      "Vision.MirrorMode", 0.7f, 0.1f, 1.f);
  
  // Set to true to have the default image be rotated 180º from "normal" (upside down on Vector's face)
  CONSOLE_VAR(bool, kTheBox_RotateImage180ByDefault, "TheBox.Screen", false);
  
  // Set to true to use normal MirrorMode, in which the screen display is flipped to be like a mirror.
  // Set to false, to just display the image as seen by the camera.
  CONSOLE_VAR(bool, kTheBox_UseMirroredImages, "TheBox.Screen", true);
  
  CONSOLE_VAR(s32,  kTheBox_MaxFaceStrings, "TheBox.Screen", 3);
  CONSOLE_VAR(bool, kTheBox_DrawFaceStringsAtTop, "TheBox.Screen", true);
  
  // Set to false to skip displaying names/data strings for faces with "unknown" ID
  CONSOLE_VAR(bool, kTheBox_DisplayUnknownFaceNames, "TheBox.Screen", false);
  
  // Set to true to use ID to select consistent draw color.
  // Set to false draws faces with eyes detected in yellow and faces with no eyes detected in red.
  CONSOLE_VAR(bool, kTheBox_ColorFacesBasedOnID, "TheBox.Screen", true);
  
  // Set to true to only draw salient points that are expected to produced localized results
  CONSOLE_VAR(bool, kTheBox_OnlyDrawLocalizableSalientPoints, "TheBox.Screen", true);
  
  // SalientPoints must have at least (at most) this as their area fraction to be drawn
  CONSOLE_VAR_RANGED(f32, kTheBox_SalientPointAreaFraction_Min, "TheBox.Screen", (f32)(20*20)/(f32)FACE_DISPLAY_NUM_PIXELS, 0.f, 1.f);
  CONSOLE_VAR_RANGED(f32, kTheBox_SalientPointAreaFraction_Max, "TheBox.Screen", 0.8f, 0.f, 1.f);
  
  // SalientPoints that overlap existing ones by this much or more will not be drawn
  CONSOLE_VAR_RANGED(f32, kTheBox_SalientPointMaxOverlap, "TheBox.Screen", 0.5f, 0.f, 1.f);
  
  // Darken edge of the screen to soften it a bit
  CONSOLE_VAR_ENUM(s32, kTheBox_ScreenEdgeVignettingMode, "TheBox.Screen", 1, "Off,Camera,All");
  CONSOLE_VAR_RANGED(s32, kTheBox_ScreenEdgeVignettingDist, "TheBox.Screen", 5, 0, 10);
  CONSOLE_VAR_RANGED(f32, kTheBox_ScreenEdgeVignettingSigma, "TheBox.Screen", 3.f, 0.f, 10.f);
  
  // TODO: Figure out the original image resolution? This just assumes "Default" for marker/face detection
  constexpr f32 kXmax = (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;
  constexpr f32 kHeightScale = (f32)FACE_DISPLAY_HEIGHT / (f32)DEFAULT_CAMERA_RESOLUTION_HEIGHT;
  constexpr f32 kWidthScale  = (f32)FACE_DISPLAY_WIDTH / (f32)DEFAULT_CAMERA_RESOLUTION_WIDTH;
  const char* const kUnknownName = "Person_";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MirrorModeManager::MirrorModeManager()
: _screenImg(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH)
, _gammaLUT{}
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static Rectangle<f32> DisplayMirroredRectHelper(f32 x_topLeft, f32 y_topLeft, f32 width, f32 height, bool doMirror)
{
  if(doMirror)
  {
    const f32 x_topRight = x_topLeft + width; // will become upper left after mirroring
    const Rectangle<f32> rect((f32)FACE_DISPLAY_WIDTH - kWidthScale*x_topRight, // mirror rectangle for display
                              y_topLeft * kHeightScale,
                              width * kWidthScale,
                              height * kHeightScale);
    return rect;
  }
  else
  {
    const Rectangle<f32> rect(x_topLeft * kWidthScale,
                              y_topLeft * kHeightScale,
                              width * kWidthScale,
                              height * kHeightScale);
    return rect;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
static inline Point<2,T> MirrorPointHelper(const Point<2,T>& pt, bool doMirror)
{
  const Point<2,T> pt_mirror((doMirror ? kWidthScale*(kXmax - pt.x()) : kWidthScale*pt.x()),
                             pt.y()*kHeightScale);
  return pt_mirror;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static Quad2f DisplayMirroredQuadHelper(const Quad2f& quad, bool doMirror)
{
  // Mirror x coordinates, swap left/right points, and scale for each point in the quad:
  const Quad2f quad_mirrored(MirrorPointHelper(quad.GetTopRight(), doMirror),
                             MirrorPointHelper(quad.GetBottomRight(), doMirror),
                             MirrorPointHelper(quad.GetTopLeft(), doMirror),
                             MirrorPointHelper(quad.GetBottomLeft(), doMirror));
  
  return quad_mirrored;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
static Polygon<2,T> DisplayMirroredPolyHelper(const Polygon<2,T>& poly, bool doMirror)
{
  Polygon<2,T> poly_mirrored;
  for(auto & pt : poly)
  {
    poly_mirrored.emplace_back(MirrorPointHelper(pt, doMirror));
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
    
    _screenImg.DrawQuad(DisplayMirroredQuadHelper(quad, _doMirror), drawColor, 3);
    if(Util::IsFltGTZero(kDisplayMarkerNamesScale))
    {
      _screenImg.DrawText({1., _screenImg.GetNumRows()-1}, name.substr(strlen("MARKER_"),std::string::npos),
                          drawColor, kDisplayMarkerNamesScale);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline bool ShouldDisplayFace(const Vision::TrackedFace& face)
{
  return (kTheBox_DisplayUnknownFaceNames || (face.GetID() > 0));
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MirrorModeManager::DrawFaces(const std::list<Vision::TrackedFace>& faceDetections)
{
  // Create a list of iterators for display, to avoid copying all the TrackedFaces
  // Only keep those faces we "should" display
  std::list<std::list<Vision::TrackedFace>::const_iterator> facesToDisplay;
  
  s32 numDisplayFaces = 0;
  for(auto iter = faceDetections.begin();
      iter != faceDetections.end() && numDisplayFaces < kTheBox_MaxFaceStrings;
      ++iter)
  {
    if(ShouldDisplayFace(*iter))
    {
      facesToDisplay.emplace_back(iter);
      ++numDisplayFaces;
    }
  }
  
  // Nothing to display? Just abort now
  if(facesToDisplay.empty())
  {
    return;
  }
  
  // Sort the faces in order of ID to keep display order consistent
  facesToDisplay.sort([](std::list<Vision::TrackedFace>::const_iterator& faceIter1,
                         std::list<Vision::TrackedFace>::const_iterator& faceIter2)
  {
    return faceIter1->GetID() < faceIter2->GetID();
  });
  
  s32 faceLine = 1;
  for(const auto& faceToDisplay : facesToDisplay)
  {
    const Vision::TrackedFace& faceDetection = *faceToDisplay;
    const auto& faceID = faceDetection.GetID();
    const auto& rect = faceDetection.GetRect();
    const auto& name = faceDetection.GetName();

    ColorRGBA color = NamedColors::RED;
    if(kTheBox_ColorFacesBasedOnID)
    {
      static const std::vector<ColorRGBA> kPalette{
        {(u8)250,  50,  37},
        {(u8)250, 150,  44},
        {(u8)250, 250,  80},
        {(u8)105, 175,  60},
        {(u8) 23,  77, 250},
        {(u8)105, 175,  60},
        {(u8)175,  45, 220},
        {(u8)250,  90,  36},
        {(u8)250, 190,  50},
        {(u8)215, 230,  70},
        {(u8) 30, 145, 200},
        {(u8) 60,  20, 160},
        {(u8)200,  30,  80},
      };
      
      color = kPalette.at(faceDetection.GetID() % kPalette.size());
    }
    else if (faceDetection.HasEyes())
    {
      // Only draw a yellow rectangle around the face if the face "has parts"
      // to which the HasEyes method is a proxy
      color = NamedColors::YELLOW;
    }
    
    _screenImg.DrawRect(DisplayMirroredRectHelper(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight(), _doMirror),
                        color, 3);
    
    const bool kUseDropShadow = true;
    if(kDisplayFaceNamesInMirrorMode && ShouldDisplayFace(faceDetection))
    {
      const Vec2f fontSize = _screenImg.GetTextSize("Test", kMirrorModeFaceFontScale, 1);
      
      const auto& debugInfo = faceDetection.GetRecognitionDebugInfo();
      if(!debugInfo.empty() && kTheBox_MaxFaceStrings==1)
      {
        s32 debugLine = 1;
        for(const auto& info : debugInfo)
        {
          const std::string& name = info.name;
          std::string dispName(name.empty() ? kUnknownName : name);
          dispName += "[" + std::to_string(info.matchedID) + "]: " + std::to_string(info.score);
          const Point2f position{1.f, _screenImg.GetNumRows()-1-(debugInfo.size()-debugLine)*(fontSize.y()+1)};
          _screenImg.DrawText(position, dispName, color, kMirrorModeFaceFontScale, kUseDropShadow);
          ++debugLine;
        }
      }
      else
      {
        std::string dispName;
        if(name.empty())
        {
          dispName = kUnknownName + std::to_string(faceID);
        }
        else
        {
          dispName = name;
        }
        
        if(faceDetection.GetAge() > 0)
        {
          dispName += ": " + std::to_string(faceDetection.GetAge());
        }
        
        //        const Vision::FacialExpression expression = faceDetection.GetMaxExpression();
        //        if(Vision::FacialExpression::Unknown != expression)
        //        {
        //          dispName += " (";
        //          dispName += EnumToString(expression);
        //          dispName += ")";
        //        }
        
        Point2f position;
        if(kTheBox_DrawFaceStringsAtTop)
        {
          position = {1.f, faceLine*(fontSize.y()+1)};
        }
        else
        {
          position = {1.f, _screenImg.GetNumRows()-1-(numDisplayFaces-faceLine)*(fontSize.y()+1)};
        }

        _screenImg.DrawText(position, dispName, color, kMirrorModeFaceFontScale, kUseDropShadow);
        ++faceLine;
        if(faceLine > kTheBox_MaxFaceStrings)
        {
          break;
        }
      }
    }
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
void MirrorModeManager::DrawSalientPoints(const VisionProcessingResult& procResult)
{
  const EngineTimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  const bool usingFixedDrawTime = (kDrawMirrorModeSalientPointsFor_ms > 0);
  
  // Remove any "expired" points to draw
  if(usingFixedDrawTime)
  {
    auto iter = _salientPointsToDraw.begin();
    while(iter != _salientPointsToDraw.end())
    {
      if(currentTime_ms > (iter->first + kDrawMirrorModeSalientPointsFor_ms)) {
        iter = _salientPointsToDraw.erase(iter);
      } else {
        ++iter;
      }
    }
  }
  
  if(procResult.modesProcessed.Contains(VisionMode::DetectingBrightColors) ||
     procResult.modesProcessed.ContainsAnyOf(GetVisionModesUsingNeuralNets()))
  {
    if(!usingFixedDrawTime)
    {
      // If not using a fixed draw time, clear next time we get salient points
      _salientPointsToDraw.clear();
    }
    
    // Update the list of points to draw
    const std::list<Vision::SalientPoint>& salientPoints = procResult.salientPoints;
    for(auto const& salientPoint_norm : salientPoints)
    {
      const Vision::SalientPointType salientType = salientPoint_norm.salientType;
      if(kTheBox_OnlyDrawLocalizableSalientPoints && !Vision::IsSalientPointTypeLocalized(salientType, false))
      {
        continue;
      }
      
      if(!Util::InRange(salientPoint_norm.area_fraction,
                        kTheBox_SalientPointAreaFraction_Min,
                        kTheBox_SalientPointAreaFraction_Max))
      {
        continue;
      }
      
      // Salient points are in normalized coordinates, so scale directly to screen image size
      auto salientPoint = salientPoint_norm;
      salientPoint.x_img *= _screenImg.GetNumCols();
      salientPoint.y_img *= _screenImg.GetNumRows();
      for(auto &pt : salientPoint.shape)
      {
        pt.x *= DEFAULT_CAMERA_RESOLUTION_WIDTH;
        pt.y *= DEFAULT_CAMERA_RESOLUTION_HEIGHT;
      }
      
      bool keep = true;
      if(kTheBox_SalientPointMaxOverlap < 1.f)
      {
        // Make sure there's not already a salient point that overlaps this one too much
        for(const auto& salientPointToDraw : _salientPointsToDraw)
        {
          if(salientPointToDraw.second.timestamp == salientPoint.timestamp)
          {
            const Rectangle<f32> rect(Poly2f(salientPoint.shape));
            const Rectangle<f32> rectToDraw(Poly2f(salientPointToDraw.second.shape));
            if(rect.ComputeOverlapScore(rectToDraw) > kTheBox_SalientPointMaxOverlap)
            {
              // Found an existing salient point to draw at the same timestamp that overlaps too much
              // Don't draw this one
              keep = false;
              break;
            }
          }
        }
      }
      
      if(keep)
      {
        _salientPointsToDraw.emplace(currentTime_ms, salientPoint);
      }
    }
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
    _screenImg.DrawPoly(DisplayMirroredPolyHelper(poly, _doMirror), color, 2);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline f32 GetVignettingFactor(f32 x)
{
  return std::expf(-0.5f * (x*x) / (kTheBox_ScreenEdgeVignettingSigma*kTheBox_ScreenEdgeVignettingSigma));
}
  
static void AddVignetting(Vision::ImageRGB& img)
{
  for(s32 i=0; i<kTheBox_ScreenEdgeVignettingDist; ++i)
  {
    img.get_CvMat_().row(i) *= GetVignettingFactor(kTheBox_ScreenEdgeVignettingDist-i);
  }
  
  const s32 bottom = img.GetNumRows()-(kTheBox_ScreenEdgeVignettingDist+1);
  for(s32 i=bottom; i<img.GetNumRows(); ++i)
  {
    img.get_CvMat_().row(i) *= GetVignettingFactor(i-bottom);
  }
  
  for(s32 j=0; j<kTheBox_ScreenEdgeVignettingDist; ++j)
  {
    img.get_CvMat_().col(j) *= GetVignettingFactor(kTheBox_ScreenEdgeVignettingDist-j);
  }
  
  const s32 right  = img.GetNumCols()-(kTheBox_ScreenEdgeVignettingDist+1);
  for(s32 j=right; j<img.GetNumCols(); ++j)
  {
    img.get_CvMat_().col(j) *= GetVignettingFactor(j-right);;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result MirrorModeManager::CreateMirrorModeImage(const Vision::ImageRGB& cameraImg,
                                                VisionProcessingResult& visionProcResult,
                                                const std::list<VisionProcessingResult>& additionalDetectionResults)
{
  cameraImg.Resize(_screenImg, Vision::ResizeMethod::NearestNeighbor);
  
  if(kTheBox_ScreenEdgeVignettingMode == 1)
  {
    // Vignette the camera feed, but nothing draw over it
    AddVignetting(_screenImg);
  }

  // Only do mirroring if it's enabled and we're not being told to "unmirror" by the VisionModes in the result
  // Note that Unmirroring does nothing if mirroring is not enabled in the first place.
  _doMirror = (kTheBox_UseMirroredImages && !visionProcResult.modesProcessed.Contains(VisionMode::MirrorModeUnmirrored));
  
  // Flip image around the y axis (before we draw anything on it)
  if(_doMirror)
  {
    cv::flip(_screenImg.get_CvMat_(), _screenImg.get_CvMat_(), 1);
  }
  
  DrawAllDetections(visionProcResult);
  
  for(const auto& additionalDetectionResult : additionalDetectionResults)
  {
    DrawAllDetections(additionalDetectionResult);
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
  
  // Whether rotate "upside down" for display is an XOR. Either:
  //  - we rotate by default and the special vision mode is NOT present
  //    (since that would be double rotation, meaning upright image)
  //  - we don't rotate by default but the special vision mode IS present
  if(kTheBox_RotateImage180ByDefault ^ visionProcResult.modesProcessed.Contains(VisionMode::MirrorModeRotate180))
  {
    cv::rotate(_screenImg.get_CvMat_(), _screenImg.get_CvMat_(), cv::ROTATE_180);
  }
  
  if(kTheBox_ScreenEdgeVignettingMode == 2)
  {
    // Vignette the final image
    // NOTE: Still does not affecting the string drawn by VisionComponent!
    AddVignetting(_screenImg);
  }
  
  visionProcResult.mirrorModeImg.SetFromImageRGB(_screenImg, _gammaLUT);
  visionProcResult.mirrorModeImg.SetTimestamp(cameraImg.GetTimestamp());
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MirrorModeManager::DrawAllDetections(const VisionProcessingResult& visionProcResult)
{
  if(kDisplayMarkersInMirrorMode)
  {
    DrawVisionMarkers(visionProcResult.observedMarkers);
  }
  
  if( kDisplayFacesInMirrorMode ) {
    DrawFaces(visionProcResult.faces);
  }
  
  if( kDisplaySalientPointsInMirrorMode ) {
    DrawSalientPoints(visionProcResult);
  }
  
  if(kDisplayExposureInMirrorMode)
  {
    DrawAutoExposure(visionProcResult);
  }
}
  
} // namespace Vector
} // namespace Anki
