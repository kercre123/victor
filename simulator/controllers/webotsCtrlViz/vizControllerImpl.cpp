/**
* File: vizControllerImpl
*
* Author: damjan stulic
* Created: 9/15/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#include "simulator/controllers/shared/webotsHelpers.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/colorRGBA.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/vision/engine/image.h"
#include "clad/types/animationTypes.h"
#include "clad/vizInterface/messageViz.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/vision/visionModesHelpers.h"
#include "engine/viz/vizTextLabelTypes.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "vizControllerImpl.h"
#include <functional>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <webots/Display.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Supervisor.hpp>

#include <iomanip>

namespace Anki {
namespace Vector {


static const size_t kEmotionBuffersCapacity  = 300; // num ticks of emotion score values to store
  
  
VizControllerImpl::VizControllerImpl(webots::Supervisor& vs)
  : _vizSupervisor(vs)
{
  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    _emotionBuffers[i].Reset(kEmotionBuffersCapacity);
  }
  _emotionEventBuffer.Reset(kEmotionBuffersCapacity);
}
  

void VizControllerImpl::Init()
{
  // bind to specific handlers in the robot class
  Subscribe(VizInterface::MessageVizTag::SetRobot,
    std::bind(&VizControllerImpl::ProcessVizSetRobotMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::SetLabel,
    std::bind(&VizControllerImpl::ProcessVizSetLabelMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::DockingErrorSignal,
    std::bind(&VizControllerImpl::ProcessVizDockingErrorSignalMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::VisionMarker,
    std::bind(&VizControllerImpl::ProcessVizVisionMarkerMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CameraQuad,
    std::bind(&VizControllerImpl::ProcessVizCameraQuadMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CameraRect,
    std::bind(&VizControllerImpl::ProcessVizCameraRectMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CameraLine,
    std::bind(&VizControllerImpl::ProcessVizCameraLineMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CameraOval,
    std::bind(&VizControllerImpl::ProcessVizCameraOvalMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CameraText,
    std::bind(&VizControllerImpl::ProcessVizCameraTextMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::DisplayImage,
    std::bind(&VizControllerImpl::ProcessVizDisplayImageMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::ImageChunk,
    std::bind(&VizControllerImpl::ProcessVizImageChunkMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::TrackerQuad,
    std::bind(&VizControllerImpl::ProcessVizTrackerQuadMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::RobotStateMessage,
    std::bind(&VizControllerImpl::ProcessVizRobotStateMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CurrentAnimation,
    std::bind(&VizControllerImpl::ProcessVizCurrentAnimation, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::RobotMood,
    std::bind(&VizControllerImpl::ProcessVizRobotMoodMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::SaveImages,
    std::bind(&VizControllerImpl::ProcessSaveImages, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::SaveState,
    std::bind(&VizControllerImpl::ProcessSaveState, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CameraParams,
    std::bind(&VizControllerImpl::ProcessCameraParams, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::BehaviorStackDebug,
    std::bind(&VizControllerImpl::ProcessBehaviorStackDebug, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::VisionModeDebug,
    std::bind(&VizControllerImpl::ProcessVisionModeDebug, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::EnabledVisionModes,
    std::bind(&VizControllerImpl::ProcessEnabledVisionModes, this, std::placeholders::_1));


  // Get display devices
  _disp = _vizSupervisor.getDisplay("cozmo_viz_display");
  _dockDisp = _vizSupervisor.getDisplay("cozmo_docking_display");
  _moodDisp = _vizSupervisor.getDisplay("cozmo_mood_display");
  _bsmStackDisp = _vizSupervisor.getDisplay("victor_behavior_stack_display");
  _visionModeDisp = _vizSupervisor.getDisplay("victor_vision_mode_display");

  // Find all the debug image displays in the proto. Use the first as the camera feed and the rest for debug images.
  {
    webots::Node* vizNode = _vizSupervisor.getSelf();
    webots::Field* numDisplaysField = vizNode->getField("numDebugImageDisplays");
    s32 numDisplays = 1;
    if(numDisplaysField == nullptr)
    {
      PRINT_NAMED_WARNING("VizControllerImpl.Init.MissingNumDebugDisplaysField", "Assuming single display (camera)");
    }
    else 
    {
      numDisplays = numDisplaysField->getSFInt32() + 1; // +1 because this is in addition to the camera
    }
    
    _camDisp = nullptr;

    for(s32 displayCtr = 0; displayCtr < numDisplays; ++displayCtr)
    {
      webots::Display* display = _vizSupervisor.getDisplay("cozmo_debug_image_display" + std::to_string(displayCtr));
      DEV_ASSERT_MSG(display != nullptr, "VizControllerImpl.Init.NullDebugDisplay", "displayCtr=%d", displayCtr);
    
      if(displayCtr==0)
      {
        _camDisp = display;
      }
      else
      {
        _debugImages.emplace_back(display);
      }
    }
    
    DEV_ASSERT(_camDisp != nullptr, "VizControllerImpl.Init.NoCameraDisplay");
    PRINT_NAMED_DEBUG("VizControllerImpl.Init.ImageDisplaysCreated",
                      "Found camera display and %zu debug displays",
                      _debugImages.size()-1);
  }

  _disp->setFont("Lucida Console", 8, true);
  _moodDisp->setFont("Lucida Console", 8, true);
  _bsmStackDisp->setFont("Lucida Console", 8, true);
  _visionModeDisp->setFont("Lucida Console", 8, true);
  
  // === Look for CozmoBot in scene tree ===

  // Look for controller-less CozmoBot in children.
  // These will be used as visualization robots.
  auto nodeInfo = WebotsHelpers::GetFirstMatchingSceneTreeNode(&_vizSupervisor, "CozmoBot");
  if (nodeInfo.nodePtr == nullptr) {
    // If there's no Vector, look for a Whiskey
    nodeInfo = WebotsHelpers::GetFirstMatchingSceneTreeNode(&_vizSupervisor, "WhiskeyBot");
  }

  const auto* nd = nodeInfo.nodePtr;
  if (nd != nullptr) {
    DEV_ASSERT(nodeInfo.type == webots::Node::SUPERVISOR, "VizControllerImpl.Init.CozmoBotNotASupervisor");
    
    // Get the vizMode status
    bool vizMode = false;
    webots::Field* vizModeField = nd->getField("vizMode");
    if (vizModeField) {
      vizMode = vizModeField->getSFBool();
    }
    
    if (vizMode) {
      PRINT_NAMED_INFO("VizControllerImpl.Init.FoundVizRobot",
                       "Found Viz robot with name %s", nodeInfo.typeName.c_str());
      CozmoBotVizParams p;
      p.supNode = (webots::Supervisor*)nd;

      // Find pose fields
      p.trans = nd->getField("translation");
      p.rot = nd->getField("rotation");

      // Find lift and head angle fields
      p.headAngle = nd->getField("headAngle");
      p.liftAngle = nd->getField("liftAngle");

      if (p.supNode && p.trans && p.rot && p.headAngle && p.liftAngle) {
        PRINT_NAMED_INFO("VizControllerImpl.Init.AddedVizRobot",
                         "Added viz robot %s", nodeInfo.typeName.c_str());
        vizBots_.push_back(p);
      } else {
        PRINT_NAMED_ERROR("VizControllerImpl.Init.MissingFields",
                          "ERROR: Could not find all required fields in CozmoBot supervisor");
      }
    }
  }

}

void VizControllerImpl::ProcessMessage(VizInterface::MessageViz&& message)
{
  uint32_t type = static_cast<uint32_t>(message.GetTag());
  _eventMgr.Broadcast(AnkiEvent<VizInterface::MessageViz>(
    type, std::move(message)));
}
  
void VizControllerImpl::ProcessSaveImages(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SaveImages();
  _saveImageMode = payload.mode;
  if(_saveImageMode != ImageSendMode::Off)
  {
    if(payload.path.empty()) {
      _savedImagesFolder = "saved_images";
    } else {
      _savedImagesFolder = payload.path;
    }
    
    if (!_savedImagesFolder.empty() && !Util::FileUtils::CreateDirectory(_savedImagesFolder, false, true)) {
      PRINT_NAMED_WARNING("VizControllerImpl.ProcessSaveImages.CreateDirectoryFailed",
                          "Could not create: %s", _savedImagesFolder.c_str());
    }
    else {
      PRINT_NAMED_INFO("VizControllerImpl.ProcessSaveImages.DirectorySet",
                       "Will save to %s", _savedImagesFolder.c_str());
    }
  }
  else
  {
    PRINT_NAMED_INFO("VizControllerImpl.ProcessSaveImages.DisablingImageSaving",
                     "Disabling image saving");
  }
}
  
  
void VizControllerImpl::ProcessSaveState(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SaveState();
  _saveState = payload.enabled;
  if(_saveState)
  {
    if(_savedStateFolder.empty()) {
      _savedStateFolder = "saved_state";
    } else {
      _savedStateFolder = payload.path;
    }
  }
}

void VizControllerImpl::SetRobotPose(CozmoBotVizParams *p,
  const f32 x, const f32 y, const f32 z,
  const f32 rot_axis_x, const f32 rot_axis_y, const f32 rot_axis_z, const f32 rot_rad,
  const f32 headAngle, const f32 liftAngle)
{
  // Make sure we haven't tried to set these Webots fields in the current time step
  // (which causes weird behavior due to a Webots R2018a bug with the setSF* functions)
  // This should be removed once the Webots bug is fixed (COZMO-16021)
  static double lastUpdateTime = 0.0;
  const double currTime = _vizSupervisor.getTime();
  if (FLT_NEAR(currTime, lastUpdateTime)) {
    return;
  }
  lastUpdateTime = currTime;
  
  if (p) {
    double trans[3] = {x,y,z};
    p->trans->setSFVec3f(trans);

    // TODO: Transform roll pitch yaw to axis-angle.
    // Only using yaw for now.
    double rot[4] = {rot_axis_x,rot_axis_y,rot_axis_z, rot_rad};
    p->rot->setSFRotation(rot);

    p->liftAngle->setSFFloat(liftAngle + 0.199763);  // Adding LIFT_LOW_ANGLE_LIMIT since the model's lift angle does not correspond to robot's lift angle.
    // TODO: Make this less hard-coded.
    p->headAngle->setSFFloat(headAngle);
  }
}


void VizControllerImpl::ProcessVizSetRobotMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SetRobot();
  
  const uint8_t robotID = 1; // only ID ever used is 1
  
  std::map<u8, u8>::iterator it = robotIDToVizBotIdxMap_.find(robotID);
  if (it == robotIDToVizBotIdxMap_.end()) {
    if (robotIDToVizBotIdxMap_.size() < vizBots_.size()) {
      // Robot ID is not currently registered, but there are still some available vizBots.
      // Auto assign one here.
      robotIDToVizBotIdxMap_[robotID] = (uint8_t)robotIDToVizBotIdxMap_.size();
      it = robotIDToVizBotIdxMap_.end();
      it--;
      PRINT_NAMED_INFO("VizControllerImpl.ProcessVizSetRobotMessage.RegisteringRobot","Registering vizBot for robot %d\n", robotID);
    } else {
      // Print 'no more vizBots' message. Just once.
      static bool printedNoMoreVizBots = false;
      if (!printedNoMoreVizBots) {
        PRINT_NAMED_WARNING("VizControllerImpl.ProcessVizSetRobotMessage.NoMoreVizBots",
          "RobotID %d not registered. No more available Viz bots. Add more to world file!",
          robotID);
        printedNoMoreVizBots = true;
      }
      return;
    }
  }

  CozmoBotVizParams *p = &(vizBots_[it->second]);

  SetRobotPose(p,
    payload.x_trans_m, payload.y_trans_m, payload.z_trans_m,
    payload.rot_axis_x, payload.rot_axis_y, payload.rot_axis_z, payload.rot_rad,
    payload.head_angle, payload.lift_angle);
}

static inline void SetColorHelper(webots::Display* disp, u32 ankiColor)
{
  disp->setColor(ankiColor >> 8);
  
  const uint8_t alpha = (uint8_t)(ankiColor & 0xff);
  if(alpha < 0xff) {
    static const float oneOver255 = 1.f / 255.f;
    disp->setAlpha(oneOver255 * static_cast<f32>(alpha));
  } else {
    disp->setAlpha(1.f); // need to restore alpha to 1.0 in case it was lowered from a previous call
  }
}
  
void VizControllerImpl::DrawText(webots::Display* disp, u32 lineNum, u32 color, const char* text)
{
  if (disp == nullptr) {
    PRINT_NAMED_WARNING("VizControllerImpl.DrawText.NullDisplay", "");
    return;
  }
  
  const int baseXOffset = 8;
  const int baseYOffset = 8;
  const int yLabelStep = 10;  // Line spacing in pixels. Characters are 8x8 pixels in size.

  // Clear line specified by lineNum
  SetColorHelper(disp, NamedColors::BLACK);
  disp->fillRectangle(0, baseYOffset + yLabelStep * lineNum, disp->getWidth(), yLabelStep);

  // Draw text
  SetColorHelper(disp, color);

  std::string str(text);
  if(str.empty()) {
    str = " "; // Avoid webots warnings for empty text
  }
  disp->drawText(str, baseXOffset, baseYOffset + yLabelStep * lineNum);
}

void VizControllerImpl::DrawText(webots::Display* disp, u32 lineNum, const char* text)
{
  DrawText(disp, lineNum, 0xffffff, text);
}
  
void VizControllerImpl::ProcessVizSetLabelMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SetLabel();
  const u32 lineNum = ((uint32_t)VizTextLabelType::NUM_TEXT_LABELS + payload.labelID);
  DrawText(_disp, lineNum, payload.colorID, payload.text.c_str());
}

void VizControllerImpl::ProcessVizDockingErrorSignalMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  // TODO: This can overlap with text being displayed. Create a dedicated display for it?
  const auto& payload = msg.GetData().Get_DockingErrorSignal();
  // Pixel dimensions of display area
  const int baseXOffset = 8;
  const int baseYOffset = 40;
  const int rectW = 180;
  const int rectH = 180;
  const int halfBlockFaceLength = 20;

  const f32 MM_PER_PIXEL = 2.f;

  // Print values
  char text[111];
  sprintf(text, "ErrSig x:%.1f y:%.1f z:%.1f a:%.2f\n",
          payload.x_dist, payload.y_dist, payload.z_dist, payload.angle);
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_DOCK_ERROR_SIGNAL, text);
  _camDisp->setColor(0xff0000);
  _camDisp->drawText(text, 0, 0);


  // Clear the space
  _dockDisp->setColor(0x0);
  _dockDisp->fillRectangle(baseXOffset, baseYOffset, rectW, rectH);

  _dockDisp->setColor(0xffffff);
  _dockDisp->drawRectangle(baseXOffset, baseYOffset, rectW, rectH);

  // Draw robot position
  _dockDisp->drawOval((int)(baseXOffset + 0.5f*rectW), baseYOffset + rectH, 3, 3);


  // Get pixel coordinates of block face center where
  int blockFaceCenterX = (int)(0.5f*rectW - payload.y_dist / MM_PER_PIXEL);
  int blockFaceCenterY = (int)(rectH - payload.x_dist / MM_PER_PIXEL);

  // Check that center is within display area
  if (blockFaceCenterX < halfBlockFaceLength || (blockFaceCenterX > rectW - halfBlockFaceLength) ||
    blockFaceCenterY < halfBlockFaceLength || (blockFaceCenterY > rectH - halfBlockFaceLength) ) {
    return;
  }

  blockFaceCenterX += baseXOffset;
  blockFaceCenterY += baseYOffset;

  // Draw line representing the block face
  int dx = (int)(halfBlockFaceLength * cosf(payload.angle));
  int dy = (int)(-halfBlockFaceLength * sinf(payload.angle));
  _dockDisp->drawLine(blockFaceCenterX + dx, blockFaceCenterY + dy, blockFaceCenterX - dx, blockFaceCenterY - dy);
  _dockDisp->drawOval(blockFaceCenterX, blockFaceCenterY, 2, 2);

}

void VizControllerImpl::ProcessVizVisionMarkerMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_VisionMarker();
  if(payload.verified) {
    _camDisp->setColor(0xff0000);
  } else {
    _camDisp->setColor(0x0000ff);
  }
  _camDisp->drawLine(payload.topLeft_x, payload.topLeft_y, payload.bottomLeft_x, payload.bottomLeft_y);
  _camDisp->drawLine(payload.bottomLeft_x, payload.bottomLeft_y, payload.bottomRight_x, payload.bottomRight_y);
  _camDisp->drawLine(payload.bottomRight_x, payload.bottomRight_y, payload.topRight_x, payload.topRight_y);
  _camDisp->drawLine(payload.topRight_x, payload.topRight_y, payload.topLeft_x, payload.topLeft_y);
}

void VizControllerImpl::ProcessVizCameraQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraQuad();

  SetColorHelper(_camDisp, payload.color);
  _camDisp->drawLine((int)payload.xUpperLeft, (int)payload.yUpperLeft, (int)payload.xLowerLeft, (int)payload.yLowerLeft);
  _camDisp->drawLine((int)payload.xLowerLeft, (int)payload.yLowerLeft, (int)payload.xLowerRight, (int)payload.yLowerRight);
  _camDisp->drawLine((int)payload.xLowerRight, (int)payload.yLowerRight, (int)payload.xUpperRight, (int)payload.yUpperRight);
  
  if(payload.topColor != payload.color)
  {
    SetColorHelper(_camDisp, payload.topColor);
  }
  _camDisp->drawLine((int)payload.xUpperRight, (int)payload.yUpperRight, (int)payload.xUpperLeft, (int)payload.yUpperLeft);
}
  
void VizControllerImpl::ProcessVizCameraRectMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraRect();
  
  SetColorHelper(_camDisp, payload.color);
  if(payload.filled)
  {
    _camDisp->fillRectangle(payload.x, payload.y, payload.width, payload.height);
  }
  else
  {
    _camDisp->drawRectangle(payload.x, payload.y, payload.width, payload.height);
  }
}

void VizControllerImpl::ProcessVizCameraLineMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraLine();
  SetColorHelper(_camDisp, payload.color);
  _camDisp->drawLine((int)payload.xStart, (int)payload.yStart, (int)payload.xEnd, (int)payload.yEnd);
}

void VizControllerImpl::ProcessVizCameraOvalMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraOval();
  SetColorHelper(_camDisp, payload.color);
  _camDisp->drawOval((int)std::round(payload.xCen), (int)std::round(payload.yCen),
    (int)std::round(payload.xRad), (int)std::round(payload.yRad));
}

void VizControllerImpl::ProcessVizCameraTextMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraText();
  if (payload.text.size() > 0){
    // Drop shadow
    SetColorHelper(_camDisp, NamedColors::BLACK);
    _camDisp->drawText(payload.text, (int)payload.x+1, (int)payload.y+1);
    
    // Actual text
    SetColorHelper(_camDisp, payload.color);
    _camDisp->drawText(payload.text, (int)payload.x, (int)payload.y);
  }
}
  
static void DisplayImageHelper(const EncodedImage& encodedImage, webots::ImageRef* &imageRef, webots::Display* display)
{
  // Delete existing image if there is one
  if (imageRef != nullptr) {
    display->imageDelete(imageRef);
  }
  
  Vision::ImageRGB img;
  Result result = encodedImage.DecodeImageRGB(img);
  if(RESULT_OK != result) {
    PRINT_NAMED_WARNING("VizControllerImpl.DisplayImageHelper.DecodeFailed", "t=%d", (TimeStamp_t)encodedImage.GetTimeStamp());
    return;
  }
  
  if(img.IsEmpty()) {
    PRINT_NAMED_WARNING("VizControllerImpl.DisplayImageHelper.EmptyImageDecoded", "t=%d", (TimeStamp_t)encodedImage.GetTimeStamp());
    return;
  }
  
  if(img.GetNumCols() == display->getWidth() && img.GetNumRows() == display->getHeight())
  {
    // Simple case: image already the right size
    imageRef = display->imageNew(img.GetNumCols(), img.GetNumRows(), img.GetDataPointer(), webots::Display::RGB);
  }
  else
  {
    // Resize to fit the display
    // NOTE: making fixed-size data buffer static because resizedImage will change dims each time it's used. 
    static std::vector<u8> buffer(display->getWidth()*display->getHeight()*3);
    Vision::ImageRGB resizedImage(display->getHeight(), display->getWidth(), buffer.data());
    img.ResizeKeepAspectRatio(resizedImage, Vision::ResizeMethod::NearestNeighbor);
    imageRef = display->imageNew(resizedImage.GetNumCols(), resizedImage.GetNumRows(),
                                 resizedImage.GetDataPointer(), webots::Display::RGB);
  }
  
  display->imagePaste(imageRef, 0, 0);
}

void VizControllerImpl::ProcessVizImageChunkMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_ImageChunk();
  
  const s32 displayIndex = payload.displayIndex;
  
  if(displayIndex == 0)
  {
    // Display index 0 (camera feed) is special:
    // - If saving is enabled, we go ahead and save as soon as it is complete
    // - We don't display until we receive a DisplayImage message (see ProcessVizDisplayImageMessage())
    // - We do extra bookkeeping around the save counter so that we can also save the
    //   the visualized image (with any extra viz elements overlaid) when it is complete, and with
    //   a matching filename.
    EncodedImage& encodedImage = _bufferedImages[_imageBufferIndex];
    const bool isImageReady = encodedImage.AddChunk(payload);
    
    if(isImageReady)
    {
      DEV_ASSERT_MSG(payload.frameTimeStamp == encodedImage.GetTimeStamp(),
                     "VizControllerImpl.ProcessVizImageChunkMessage.TimestampMismath",
                     "Payload:%u Image:%u", payload.frameTimeStamp, (TimeStamp_t)encodedImage.GetTimeStamp());
      
      // Add an entry in EncodedImages map for this new image, now that it's complete
      auto result = _encodedImages.emplace(payload.frameTimeStamp, _imageBufferIndex);
      DEV_ASSERT_MSG(result.second, "VizControllerImpl.ProcessVizImageChunkMessage.DuplicateTimestamp",
                     "t=%u", payload.frameTimeStamp);
      DEV_ASSERT_MSG(result.first->second == _imageBufferIndex,
                     "VizControllerImpl.ProcessVizImageChunkMessage.BadInsertion",
                     "Expected index:%zu Got:%zu", _imageBufferIndex, result.first->second);
#     pragma unused(result) // Avoid unused variable error in Release (only used in DEV_ASSERTs)
      
      // Move to next buffered index circularly
      ++_imageBufferIndex;
      if(_imageBufferIndex == _bufferedImages.size())
      {
        _imageBufferIndex = 0;
      }
      
      // Invalidate anything in encodedImages using the index we are about to start adding chunks to (not the one we
      // just completed; i.e. encodedImage != _bufferedImages[_imageBufferIndex] now because we incremented the index!)
      _encodedImages.erase(_bufferedImages[_imageBufferIndex].GetTimeStamp());
      
      const bool saveImage = (_saveImageMode != ImageSendMode::Off);
      
      // Store the mapping for its timestamp to save counter so we can keep saved "viz" images' counters and filenames
      // in sync with these raw images files.
      // Have to do this anytime saveVizImage is enabled (which it could be even while _saveImageMode is Off, thanks
      // to the vision system potentially processing images more slowly than full frame rate) or when it is about to
      // be enabled (when saveImage is true)
      if(saveImage || _saveVizImage)
      {
        _bufferedSaveCtrs[encodedImage.GetTimeStamp()] = _saveCtr;
      }
      
      if(saveImage)
      {
        // Save original image
        std::stringstream origFilename;
        origFilename << "images_" << encodedImage.GetTimeStamp() << "_" << _saveCtr << ".jpg";
        encodedImage.Save(Util::FileUtils::FullFilePath({_savedImagesFolder, origFilename.str()}));
        _saveVizImage = true;
        ++_saveCtr;
        
        if(_saveImageMode == ImageSendMode::SingleShot) {
          _saveImageMode = ImageSendMode::Off;
        }
      }
      
    }
  }
  else
  {
    // For non-camera (debug) images, just display (and save) immediately. No need to wait for any additional
    // "viz" overlay to be added. Note: debug images are only saved in "Stream" mode (not "SingleShot")
    if(displayIndex < 1 || displayIndex > _debugImages.size())
    {
      PRINT_NAMED_WARNING("VizControllerImpl.ProcessVizImageChunkMessage.InvalidDisplayIndex",
                          "No debug display for index=%d", displayIndex);
    }
    else
    {
      DebugImage& debugImage = _debugImages.at(displayIndex-1);
      const bool isImageReady = debugImage.encodedImage.AddChunk(payload);
      
      if(isImageReady)
      {
        if(ImageSendMode::Stream == _saveImageMode)
        {
          std::stringstream debugFilename;
          debugFilename << "debug" << displayIndex << "_" << debugImage.encodedImage.GetTimeStamp() << ".jpg";
          debugImage.encodedImage.Save(Util::FileUtils::FullFilePath({_savedImagesFolder, debugFilename.str()}));
        }
        
        DisplayImageHelper(debugImage.encodedImage, debugImage.imageRef, debugImage.imageDisplay);
      }
    }
  }
  
}
  
void VizControllerImpl::ProcessVizDisplayImageMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_DisplayImage();
  
  auto encImgIter = _encodedImages.find(payload.timestamp);
  if(encImgIter == _encodedImages.end())
  {
    return;
  }
  
  const RobotTimeStamp_t timestamp = encImgIter->first;
  const EncodedImage& encodedImage = _bufferedImages[encImgIter->second];
  DEV_ASSERT_MSG(timestamp == encodedImage.GetTimeStamp(),
                 "VizControllerImpl.ProcessVizDisplayImage.TimeStampMisMatch",
                 "key=%u vs. encImg=%u", (TimeStamp_t)timestamp, (TimeStamp_t)encodedImage.GetTimeStamp());
  
  if(_saveVizImage && _curImageTimestamp > 0)
  {
    if (!_savedImagesFolder.empty() && !Util::FileUtils::CreateDirectory(_savedImagesFolder, false, true)) {
      PRINT_NAMED_WARNING("VizControllerImpl.CreateDirectory", "Could not create images directory");
    }
    
    auto saveCtrIter = _bufferedSaveCtrs.find(_curImageTimestamp);
    if(saveCtrIter != _bufferedSaveCtrs.end())
    {
      // Save previous image with any viz overlaid before we delete it
      webots::ImageRef* copyImg = _camDisp->imageCopy(0, 0, _camDisp->getWidth(), _camDisp->getHeight());
      std::stringstream vizFilename;
      vizFilename << "viz_images_" << _curImageTimestamp << "_" << saveCtrIter->second << ".png";
      _camDisp->imageSave(copyImg, Util::FileUtils::FullFilePath({_savedImagesFolder, vizFilename.str()}));
      _camDisp->imageDelete(copyImg);
      _saveVizImage = false;
      
      // Remove all saved counters up to and including timestamp we just saved (the assumption is we never
      // go backward, so once we've saved this one, we don't need it or anything that came before it)
      _bufferedSaveCtrs.erase(_bufferedSaveCtrs.begin(), ++saveCtrIter);
      
    }
  }

  DisplayImageHelper(encodedImage, _camImg, _camDisp);
 
  // Store the timestamp for the currently displayed image so we can use it to save
  // that image with the right filename next call
  _curImageTimestamp = timestamp;
  
  DisplayCameraInfo(timestamp);
  
  // Remove all encoded images up to and including the specified timestamp (the assumption is we never
  // go backward, so once we've displayed this one, we don't need it or anything that came before it)
  _encodedImages.erase(_encodedImages.begin(), ++encImgIter);
}

void VizControllerImpl::ProcessCameraParams(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraParams();
  _cameraParams = payload.cameraParams;
}

void VizControllerImpl::DisplayCameraInfo(const RobotTimeStamp_t timestamp)
{
  // Print values
  char text[42];
  snprintf(text, sizeof(text), "Exp:%u Gain:%.3f\n", 
           _cameraParams.exposureTime_ms, _cameraParams.gain);
  SetColorHelper(_camDisp, NamedColors::RED);
  _camDisp->drawText(std::to_string((TimeStamp_t)timestamp), 1, _camDisp->getHeight()-9); // display timestamp at lower left
  _camDisp->drawText(text, _camDisp->getWidth()-144, _camDisp->getHeight()-9); //display exposure in bottom right


  snprintf(text, sizeof(text), "AWB:%.3f %.3f %.3f\n", 
           _cameraParams.whiteBalanceGainR, 
           _cameraParams.whiteBalanceGainG, 
           _cameraParams.whiteBalanceGainB);
  SetColorHelper(_camDisp, NamedColors::RED);
  _camDisp->drawText(text, _camDisp->getWidth()-180, _camDisp->getHeight()-18);
}


void VizControllerImpl::ProcessVizTrackerQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_TrackerQuad();
  _camDisp->setColor(0x0000ff);
  _camDisp->drawLine((int)payload.topLeft_x, (int)payload.topLeft_y, (int)payload.topRight_x, (int)payload.topRight_y);
  _camDisp->setColor(0x00ff00);
  _camDisp->drawLine((int)payload.topRight_x, (int)payload.topRight_y, (int)payload.bottomRight_x, (int)payload.bottomRight_y);
  _camDisp->drawLine((int)payload.bottomRight_x, (int)payload.bottomRight_y, (int)payload.bottomLeft_x, (int)payload.bottomLeft_y);
  _camDisp->drawLine((int)payload.bottomLeft_x, (int)payload.bottomLeft_y, (int)payload.topLeft_x, (int)payload.topLeft_y);
}
  
void VizControllerImpl::ProcessVizRobotStateMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_RobotStateMessage();
  char txt[128];

  sprintf(txt, "Pose: %6.1f, %6.1f, ang: %4.1f  [fid: %u, oid: %u]",
    payload.state.pose.x,
    payload.state.pose.y,
    RAD_TO_DEG(payload.state.pose.angle),
    payload.state.pose_frame_id,
    payload.state.pose_origin_id);
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_POSE, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Head: %5.1f deg, Lift: %4.1f mm",
    RAD_TO_DEG(payload.state.headAngle),
    ConvertLiftAngleToLiftHeightMM(payload.state.liftAngle));
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_HEAD_LIFT, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Pitch: %4.1f deg (IMUHead: %4.1f deg)",
    RAD_TO_DEG(payload.state.pose.pitch_angle),
    RAD_TO_DEG(payload.state.pose.pitch_angle + payload.state.headAngle));
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_PITCH, Anki::NamedColors::GREEN, txt);
  
  sprintf(txt, "Acc:  %6.0f %6.0f %6.0f mm/s2  ImuTemp %+6.2f degC",
          payload.state.accel.x,
          payload.state.accel.y,
          payload.state.accel.z,
          payload.imuTemperature_degC);
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_ACCEL, Anki::NamedColors::GREEN, txt);
  
  sprintf(txt, "Gyro: %6.1f %6.1f %6.1f deg/s",
    RAD_TO_DEG(payload.state.gyro.x),
    RAD_TO_DEG(payload.state.gyro.y),
    RAD_TO_DEG(payload.state.gyro.z));
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_GYRO, Anki::NamedColors::GREEN, txt);

  bool cliffDetected = payload.state.cliffDetectedFlags > 0;
  sprintf(txt, "Cliff: {%4u, %4u, %4u, %4u} thresh: {%4u, %4u, %4u, %4u}",
          payload.state.cliffDataRaw[0],
          payload.state.cliffDataRaw[1],
          payload.state.cliffDataRaw[2],
          payload.state.cliffDataRaw[3],
          payload.cliffThresholds[0],
          payload.cliffThresholds[1],
          payload.cliffThresholds[2],
          payload.cliffThresholds[3]);
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_CLIFF, cliffDetected ? Anki::NamedColors::RED : Anki::NamedColors::GREEN, txt);

  const auto& proxData = payload.state.proxData;
  sprintf(txt, "Dist: %4u mm, sigStrength: %5.3f, ambient: %5.3f status %s",
          proxData.distance_mm,
          proxData.signalIntensity / proxData.spadCount,
          100.f * proxData.ambientIntensity / proxData.spadCount,
          RangeStatusToString(proxData.rangeStatus));
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_DIST, Anki::NamedColors::GREEN, txt);
  
  sprintf(txt, "Speed L: %4d  R: %4d mm/s",
    (int)payload.state.lwheel_speed_mmps,
    (int)payload.state.rwheel_speed_mmps);
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_SPEEDS, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Touch: %u", 
    payload.state.backpackTouchSensorRaw
  );
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_TOUCH, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Batt: %2.2fV, %2uC [%c%c]", 
    payload.batteryVolts,
    payload.state.battTemp_C,
    payload.state.status & (uint32_t)RobotStatusFlag::IS_BATTERY_OVERHEATED ? 'H' : ' ',
    payload.state.status & (uint32_t)RobotStatusFlag::IS_BATTERY_DISCONNECTED ? 'D' : ' ');
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_BATTERY, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Anim: %32s [%d], ProcFaceFrames: %d",
        _currAnimName.c_str(), 
        _currAnimTag,
         payload.numProcAnimFaceKeyframes);
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_ANIM, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Locked: %c%c%c, InUse: %c%c%c",
        (payload.lockedAnimTracks & (u8)AnimTrackFlag::LIFT_TRACK) ? 'L' : ' ',
        (payload.lockedAnimTracks & (u8)AnimTrackFlag::HEAD_TRACK) ? 'H' : ' ',
        (payload.lockedAnimTracks & (u8)AnimTrackFlag::BODY_TRACK) ? 'B' : ' ',
        (payload.animTracksInUse  & (u8)AnimTrackFlag::LIFT_TRACK) ? 'L' : ' ',
        (payload.animTracksInUse  & (u8)AnimTrackFlag::HEAD_TRACK) ? 'H' : ' ',
        (payload.animTracksInUse  & (u8)AnimTrackFlag::BODY_TRACK) ? 'B' : ' ');
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_ANIM_TRACK_LOCKS, Anki::NamedColors::GREEN, txt);


  sprintf(txt, "Video: %.1f Hz   Proc: %.1f Hz",
    1000.f / (f32)payload.videoFramePeriodMs, 1000.f / (f32)payload.imageProcPeriodMs);
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_VID_RATE, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Status: %5s %5s %6s %4s %4s",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_CARRYING_BLOCK ? "CARRY" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_PICKING_OR_PLACING ? "PAP" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_PICKED_UP ? "PICKUP" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_BEING_HELD ? "HELD" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_FALLING ? "FALL" : "");
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_STATUS_FLAG, Anki::NamedColors::GREEN, txt);
  
  sprintf(txt, "   %8s %10s %7s %4s",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_CHARGING ? "CHARGING" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_ON_CHARGER ? "ON_CHARGER" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_BUTTON_PRESSED ? "PWR_BTN" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::CALM_POWER_MODE ? "CALM" : "");
  
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_STATUS_FLAG_2, Anki::NamedColors::GREEN, txt);
  
  sprintf(txt, "   %4s %7s %7s %6s",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_PATHING ? "PATH" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::LIFT_IN_POS ? "" : "LIFTING",
    payload.state.status & (uint32_t)RobotStatusFlag::HEAD_IN_POS ? "" : "HEADING",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_MOVING ? "MOVING" : "");
  DrawText(_disp, (u32)VizTextLabelType::TEXT_LABEL_STATUS_FLAG_3, Anki::NamedColors::GREEN, txt);
    
  // Save state to file
  if(_saveState)
  {
    const size_t kMaxPayloadSize = 256;
    if(payload.Size() > kMaxPayloadSize) {
      PRINT_NAMED_WARNING("VizController.ProcessVizRobotStateMessage.PayloadSizeTooLarge",
                          "%zu > %zu", payload.Size(), kMaxPayloadSize);
    } else {
      // Compose line for entire state msg in hex
      char stateMsgLine[2*kMaxPayloadSize + 1];
      memset(stateMsgLine,0,kMaxPayloadSize);
      u8 msgBytes[kMaxPayloadSize];
      payload.Pack(msgBytes, kMaxPayloadSize);
      for (int i=0; i < payload.Size(); i++){
        sprintf(&stateMsgLine[2*i], "%02x", (unsigned char)msgBytes[i]);
      }
      sprintf(&stateMsgLine[payload.Size() * 2],"\n");
      
      FILE *stateFile;
      stateFile = fopen("RobotState.txt", "at");
      fputs(stateMsgLine, stateFile);
      fclose(stateFile);
    }
  } // if(_saveState)
}

void VizControllerImpl::ProcessVizCurrentAnimation(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CurrentAnimation();
  _currAnimName = payload.animName;
  _currAnimTag = payload.tag;
}
  
  
static const int kTextSpacingY = 10;
static const int kTextOffsetY  = -3;
  
  
// ========== Mood Display ==========
  
  
bool VizControllerImpl::IsMoodDisplayEnabled() const
{
  // maybe check settings or pixel size too?
  return (_emotionBuffers[0].capacity() > 0);
}


void VizControllerImpl::ProcessVizRobotMoodMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  if (!IsMoodDisplayEnabled())
  {
    return;
  }
  
  const VizInterface::RobotMood& robotMood = msg.GetData().Get_RobotMood();
  assert(robotMood.emotion.size() == (size_t)EmotionType::Count);
  
  const int windowWidth  = _moodDisp->getWidth();
  const int windowHeight = _moodDisp->getHeight();

  // Calculate y coordinate range and scaling for graph points
  
  const int   labelOffsetX  = 120; // Minimum indentation from right for the catagory label (e.g. "Happy X.XX")
  const float xStep         = float(windowWidth-labelOffsetX) / float(_emotionBuffers[0].capacity());
  
  const int   yValueFor1    = 16;
  const int   yValueForNeg1 = windowHeight - yValueFor1;
  const float yValueFor0    = float(yValueForNeg1 + yValueFor1) * 0.5f;
  const float yScalar       = float(yValueFor1) - yValueFor0; // y-is-down so larger y value = lower graph value
  
  // Clear Window
  
  _moodDisp->setColor(0x000000);
  _moodDisp->fillRectangle(0, 0, windowWidth, windowHeight);
  
  // Draw Graph Axis labels

  _moodDisp->setColor(0xffffff);
  _moodDisp->drawText("1.0",  0, yValueFor1 + kTextOffsetY);
  _moodDisp->drawText("-1.0", 0, yValueForNeg1 + kTextOffsetY);
  
  // Sort emotion indices based on the most recent value, sorting from largest to smallest value
  // so that we can draw in order (important for label positioning on right as we prvent labels drawing on top of each other)
  
  int sortedEmoIndices[(uint32_t)EmotionType::Count];
  for (uint32_t eT=0; eT < (uint32_t)EmotionType::Count; ++eT)
  {
    sortedEmoIndices[eT] = eT;
  }
  std::sort(std::begin(sortedEmoIndices), std::end(sortedEmoIndices),
            [robotMood](const int& lhs, const int& rhs)
            {
              return robotMood.emotion[lhs] > robotMood.emotion[rhs];
            } );
  
  // Calculate line spacing and top/bottom range
  
  const int kTopTextY    = (kTextSpacingY/2);
  const int kBottomTextY = windowHeight - (kTextSpacingY/2);
  
  int lastTextY = kTopTextY - kTextSpacingY;
  
  _emotionEventBuffer.push_back( robotMood.recentEvents );
  
  // Draw all the events
  
  {
    int eventY = kTopTextY;
    
    _moodDisp->setColor(0xffffff);
    float xValF = 0.0f;
    
    for (size_t j=0; j < _emotionEventBuffer.size(); ++j)
    {
      const std::vector<std::string>& eventsThisTick = _emotionEventBuffer[j];
      
      if (eventsThisTick.size() > 0)
      {
        const int xVal = (int)(xValF);
        
        for (const std::string& eventText : eventsThisTick)
        {
          _moodDisp->drawLine(xVal, eventY, xVal, eventY + 30);
          _moodDisp->drawText(eventText, xVal, eventY + kTextOffsetY);
          
          eventY += kTextSpacingY;
          if (eventY > kBottomTextY)
          {
            eventY = kTopTextY;
          }
        }
      }
      
      xValF += xStep;
    }
  }
  
  // Draw each emotion graph in order, from top to bottom
  
  for (size_t i=0; i < (size_t)EmotionType::Count; ++i)
  {
    const uint32_t eT = sortedEmoIndices[i];
    EmotionType emotionType = (EmotionType)eT;
    Util::CircularBuffer<float>& emotionBuffer = _emotionBuffers[eT];
    const float latestValue = robotMood.emotion[eT];
    emotionBuffer.push_back(latestValue);
  
    _moodDisp->setColor( ColorRGBA::CreateFromColorIndex(eT).As0RGB() );
    
    float xValF = 0.0f;
    int lastX = 0;
    int lastY = 0;
    
    // Draw a line graph connecting all of the sample points
    
    for (size_t j=0; j < emotionBuffer.size(); ++j)
    {
      const float emotionValue = emotionBuffer[j];
      const int xVal = (int)(xValF);
      const int yVal = (int)(yValueFor0 + (yScalar * emotionValue));
      
      if (j > 0)
      {
        _moodDisp->drawLine(lastX, lastY, xVal, yVal);
      }
      
      xValF += xStep;
      lastX = xVal;
      lastY = yVal;
    }
    
    // Draw the label, ideally next to the last sample, but above maxTextY (so there's room for the rest of the labels)
    // and at least 1 line down from the last category, clamped to the top/bottom range
    
    const int textX = MIN(lastX, windowWidth-labelOffsetX);
    const int maxTextY = kBottomTextY - (kTextSpacingY * int(size_t(EmotionType::Count)-(i+1)));
    const int textY = CLIP(MAX(MIN(lastY, maxTextY), lastTextY+kTextSpacingY), kTopTextY, kBottomTextY);
    lastTextY = textY;
    
    char valueString[32];
    snprintf(valueString, sizeof(valueString), "%1.2f: ", latestValue);
    std::string text = std::string(valueString) + EmotionTypeToString(emotionType);
    _moodDisp->drawText(text, textX, textY + kTextOffsetY);
  }
}


void VizControllerImpl::ProcessBehaviorStackDebug(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  if( _bsmStackDisp == nullptr ) {
    return;
  }

  // Clear the space
  _bsmStackDisp->setColor(0x0);
  _bsmStackDisp->fillRectangle(0, 0, _bsmStackDisp->getWidth(), _bsmStackDisp->getHeight());
  
  const VizInterface::BehaviorStackDebug& debugData = msg.GetData().Get_BehaviorStackDebug();

  for( size_t i=0; i < debugData.debugStrings.size(); ++i ) {
    DrawText(_bsmStackDisp, (u32)i, (u32)Anki::NamedColors::WHITE, debugData.debugStrings[i].c_str());
  }
}

void VizControllerImpl::ProcessVisionModeDebug(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  if( _visionModeDisp == nullptr ) {
    return;
  }

  // Clear the space
  _visionModeDisp->setColor(0x0);
  _visionModeDisp->fillRectangle(0, 0, _visionModeDisp->getWidth(), _visionModeDisp->getHeight());

  const VizInterface::VisionModeDebug& debugData = msg.GetData().Get_VisionModeDebug();

  DrawText(_visionModeDisp, 0, (u32)Anki::NamedColors::WHITE, "Vision Schedule:       Mode:");
  for( size_t i=0; i < debugData.debugStrings.size(); ++i ) {
    DrawText(_visionModeDisp, (u32)(i+1), (u32)Anki::NamedColors::GREEN, debugData.debugStrings[i].c_str());
  }

}

void VizControllerImpl::ProcessEnabledVisionModes(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  if( _disp == nullptr ) {
    return;
  }

  const auto& data = msg.GetData().Get_EnabledVisionModes();

  _disp->setColor(NamedColors::BLACK.As0RGB());
  const u32 fillY = ((uint32_t)VizTextLabelType::NUM_TEXT_LABELS + (uint32_t)TextLabelType::VISION_MODE + 1)*10;
  _disp->fillRectangle(0, fillY, _disp->getWidth(), 10*11);

  std::stringstream ss;
  const u32 kTextWidth = 15;
  const u32 kNumModesPerLine = 4;
  const u32 kCharWidth = 6;
  const u32 kLineHeight = 10;

  // x,y position to draw each VisionMode at in the display
  u32 x = 0;
  u32 y = fillY;

  for(VisionMode m = VisionMode::Idle; m < VisionMode::Count; m++)
  {
    // Left align text with kTextWidth+1 padding of spaces (+1 for space between modes)
    ss << std::setw(kTextWidth + 1) << std::left;
    std::string s(EnumToString(m));
    ss << s.substr(0, kTextWidth);
    
    // If this mode was processed then draw it in white
    if(std::find(data.modes.begin(), data.modes.end(), m) != data.modes.end())
    {
      _disp->setColor(NamedColors::WHITE.As0RGB());
    }
    // Otherwise draw it in gray
    else
    {
      _disp->setColor(0x808080);
    }

    _disp->drawText(ss.str(), x, y);

    // Increase x by VisionMode text length + 1 (for spacing)
    x += kCharWidth*(kTextWidth+1);

    // Only draw kNumModesPerLine
    if((static_cast<u32>(m)+1) % kNumModesPerLine == 0)
    {
      x = 0;
      y += kLineHeight;
    }

    ss.str(std::string(""));
  }
}

} // end namespace Vector
} // end namespace Anki
