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


#include "vizControllerImpl.h"
#include "anki/common/basestation/colorRGBA.h"
#include "anki/vision/basestation/image.h"
#include "clad/vizInterface/messageViz.h"
#include <webots/Supervisor.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Display.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <functional>

namespace Anki {
namespace Cozmo {


static const size_t kEmotionBuffersCapacity  = 300; // num ticks of emotion score values to store
static const size_t kBehaviorBuffersCapacity = 300; // num ticks of behavior score values to store
  
  
VizControllerImpl::VizControllerImpl(webots::Supervisor& vs)
  : vizSupervisor(vs)
{
  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    _emotionBuffers[i].Reset(kEmotionBuffersCapacity);
  }
  _emotionEventBuffer.Reset(kEmotionBuffersCapacity);
  _behaviorEventBuffer.Reset(kBehaviorBuffersCapacity);
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
  Subscribe(VizInterface::MessageVizTag::CameraLine,
    std::bind(&VizControllerImpl::ProcessVizCameraLineMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CameraOval,
    std::bind(&VizControllerImpl::ProcessVizCameraOvalMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::CameraText,
    std::bind(&VizControllerImpl::ProcessVizCameraTextMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::ImageChunk,
    std::bind(&VizControllerImpl::ProcessVizImageChunkMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::TrackerQuad,
    std::bind(&VizControllerImpl::ProcessVizTrackerQuadMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::RobotStateMessage,
    std::bind(&VizControllerImpl::ProcessVizRobotStateMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::RobotMood,
    std::bind(&VizControllerImpl::ProcessVizRobotMoodMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::RobotBehaviorSelectData,
    std::bind(&VizControllerImpl::ProcessVizRobotBehaviorSelectDataMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::NewBehaviorSelected,
    std::bind(&VizControllerImpl::ProcessVizNewBehaviorSelectedMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::StartRobotUpdate,
    std::bind(&VizControllerImpl::ProcessVizStartRobotUpdate, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::EndRobotUpdate,
    std::bind(&VizControllerImpl::ProcessVizEndRobotUpdate, this, std::placeholders::_1));

  // Get display devices
  disp = vizSupervisor.getDisplay("cozmo_viz_display");
  dockDisp = vizSupervisor.getDisplay("cozmo_docking_display");
  moodDisp = vizSupervisor.getDisplay("cozmo_mood_display");
  behaviorDisp = vizSupervisor.getDisplay("cozmo_behavior_display");
  camDisp = vizSupervisor.getDisplay("cozmo_cam_viz_display");


  // === Look for CozmoBot in scene tree ===

  // Get world root node
  webots::Node* root = vizSupervisor.getRoot();

  // Look for controller-less CozmoBot in children.
  // These will be used as visualization robots.
  webots::Field* rootChildren = root->getField("children");
  int numRootChildren = rootChildren->getCount();
  for (int n = 0 ; n<numRootChildren; ++n) {
    webots::Node* nd = rootChildren->getMFNode(n);

    // Get the node name
    std::string nodeName = "";
    webots::Field* nameField = nd->getField("name");
    if (nameField) {
      nodeName = nameField->getSFString();
    }

    // Get the vizMode status
    bool vizMode = false;
    webots::Field* vizModeField = nd->getField("vizMode");
    if (vizModeField) {
      vizMode = vizModeField->getSFBool();
    }

    //printf(" Node %d: name \"%s\" typeName \"%s\" controllerName \"%s\"\n",
    //       n, nodeName.c_str(), nd->getTypeName().c_str(), controllerName.c_str());

    if (nd->getTypeName().find("Supervisor") != std::string::npos &&
      nodeName.find("CozmoBot") != std::string::npos &&
      vizMode) {

      printf("Found Viz robot with name %s\n", nodeName.c_str());
      CozmoBotVizParams p;
      p.supNode = (webots::Supervisor*)nd;

      // Find pose fields
      p.trans = nd->getField("translation");
      p.rot = nd->getField("rotation");

      // Find lift and head angle fields
      p.headAngle = nd->getField("headAngle");
      p.liftAngle = nd->getField("liftAngle");

      if (p.supNode && p.trans && p.rot && p.headAngle && p.liftAngle) {
        printf("Added viz robot %s\n", nodeName.c_str());
        vizBots_.push_back(p);
      } else {
        printf("ERROR: Could not find all required fields in CozmoBot supervisor\n");
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

void VizControllerImpl::SetRobotPose(CozmoBotVizParams *p,
  const f32 x, const f32 y, const f32 z,
  const f32 rot_axis_x, const f32 rot_axis_y, const f32 rot_axis_z, const f32 rot_rad,
  const f32 headAngle, const f32 liftAngle)
{
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
  // Find robot by ID
  uint8_t robotID = (uint8_t)payload.robotID;
  std::map<u8, u8>::iterator it = robotIDToVizBotIdxMap_.find(robotID);
  if (it == robotIDToVizBotIdxMap_.end()) {
    if (robotIDToVizBotIdxMap_.size() < vizBots_.size()) {
      // Robot ID is not currently registered, but there are still some available vizBots.
      // Auto assign one here.
      robotIDToVizBotIdxMap_[robotID] = (uint8_t)robotIDToVizBotIdxMap_.size();
      it = robotIDToVizBotIdxMap_.end();
      it--;
      printf("Registering vizBot for robot %d\n", robotID);
    } else {
      // Print 'no more vizBots' message. Just once.
      static bool printedNoMoreVizBots = false;
      if (!printedNoMoreVizBots) {
        printf("WARNING: RobotID %d not registered. No more available Viz bots. Add more to world file!\n", robotID);
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

void VizControllerImpl::DrawText(VizTextLabelType labelID, u32 color, const char* text)
{
  const int baseXOffset = 8;
  const int baseYOffset = 8;
  const int yLabelStep = 10;  // Line spacing in pixels. Characters are 8x8 pixels in size.

  // Clear line specified by labelID
  disp->setColor(0x0);
  disp->fillRectangle(0, baseYOffset + yLabelStep * (uint32_t)labelID, disp->getWidth(), 8);

  // Draw text
  disp->setColor(color >> 8);
  disp->setAlpha(static_cast<double>(0xff & color)/255.);

  std::string str(text);
  if(str.empty()) {
    str = " "; // Avoid webots warnings for empty text
  }
  disp->drawText(str, baseXOffset, baseYOffset + yLabelStep * (uint32_t)labelID);
}

void VizControllerImpl::DrawText(VizTextLabelType labelID, const char* text)
{
  DrawText(labelID, 0xffffff, text);
}

void VizControllerImpl::ProcessVizSetLabelMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SetLabel();
  if (payload.text.size() > 0){
    VizTextLabelType labelID = (VizTextLabelType)((uint32_t)VizTextLabelType::NUM_TEXT_LABELS + payload.labelID);
    DrawText(labelID, payload.colorID, payload.text[0].c_str());
  }
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
  sprintf(text, "ErrSig x: %.1f, y: %.1f, ang: %.2f\n", payload.x_dist, payload.y_dist, payload.angle);
  DrawText(VizTextLabelType::TEXT_LABEL_DOCK_ERROR_SIGNAL, text);


  // Clear the space
  dockDisp->setColor(0x0);
  dockDisp->fillRectangle(baseXOffset, baseYOffset, rectW, rectH);

  dockDisp->setColor(0xffffff);
  dockDisp->drawRectangle(baseXOffset, baseYOffset, rectW, rectH);

  // Draw robot position
  dockDisp->drawOval((int)(baseXOffset + 0.5f*rectW), baseYOffset + rectH, 3, 3);


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
  dockDisp->drawLine(blockFaceCenterX + dx, blockFaceCenterY + dy, blockFaceCenterX - dx, blockFaceCenterY - dy);
  dockDisp->drawOval(blockFaceCenterX, blockFaceCenterY, 2, 2);

}

void VizControllerImpl::ProcessVizVisionMarkerMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_VisionMarker();
  if(payload.verified) {
    camDisp->setColor(0xff0000);
  } else {
    camDisp->setColor(0x0000ff);
  }
  camDisp->drawLine(payload.topLeft_x, payload.topLeft_y, payload.bottomLeft_x, payload.bottomLeft_y);
  camDisp->drawLine(payload.bottomLeft_x, payload.bottomLeft_y, payload.bottomRight_x, payload.bottomRight_y);
  camDisp->drawLine(payload.bottomRight_x, payload.bottomRight_y, payload.topRight_x, payload.topRight_y);
  camDisp->drawLine(payload.topRight_x, payload.topRight_y, payload.topLeft_x, payload.topLeft_y);
}

void VizControllerImpl::ProcessVizCameraQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraQuad();
  const f32 oneOver255 = 1.f / 255.f;

  camDisp->setColor(payload.color >> 8);
  uint8_t alpha = (uint8_t)(payload.color & 0xff);
  if(alpha < 0xff) {
    camDisp->setAlpha(oneOver255 * static_cast<f32>(alpha));
  }
  camDisp->drawLine((int)payload.xUpperLeft, (int)payload.yUpperLeft, (int)payload.xLowerLeft, (int)payload.yLowerLeft);
  camDisp->drawLine((int)payload.xLowerLeft, (int)payload.yLowerLeft, (int)payload.xLowerRight, (int)payload.yLowerRight);
  camDisp->drawLine((int)payload.xLowerRight, (int)payload.yLowerRight, (int)payload.xUpperRight, (int)payload.yUpperRight);
  camDisp->drawLine((int)payload.xUpperRight, (int)payload.yUpperRight, (int)payload.xUpperLeft, (int)payload.yUpperLeft);
}

void VizControllerImpl::ProcessVizCameraLineMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraLine();
  camDisp->setColor(payload.color >> 8);
  uint8_t alpha = (uint8_t)(payload.color & 0xff);
  if(alpha < 0xff) {
    const f32 oneOver255 = 1.f / 255.f;
    camDisp->setAlpha(oneOver255 * static_cast<f32>(alpha));
  }
  camDisp->drawLine((int)payload.xStart, (int)payload.yStart, (int)payload.xEnd, (int)payload.yEnd);
}

void VizControllerImpl::ProcessVizCameraOvalMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraOval();
  camDisp->setColor(payload.color >> 8);
  uint8_t alpha = (uint8_t)(payload.color & 0xff);
  if(alpha < 0xff) {
    const f32 oneOver255 = 1.f / 255.f;
    camDisp->setAlpha(oneOver255 * static_cast<f32>(alpha));
  }
  camDisp->drawOval((int)std::round(payload.xCen), (int)std::round(payload.yCen),
    (int)std::round(payload.xRad), (int)std::round(payload.yRad));
}

void VizControllerImpl::ProcessVizCameraTextMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_CameraText();
  if (payload.text.size() > 0){
    camDisp->setColor(payload.color >> 8);
    uint8_t alpha = (uint8_t)(payload.color & 0xff);
    if(alpha < 0xff) {
      const f32 oneOver255 = 1.f / 255.f;
      camDisp->setAlpha(oneOver255 * static_cast<f32>(alpha));
    }
    camDisp->drawText(payload.text[0], (int)payload.x, (int)payload.y);
  }
}

void VizControllerImpl::ProcessVizImageChunkMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  // TODO: Support timestamps
  const auto& payload = msg.GetData().Get_ImageChunk();
  const u16 width = Vision::CameraResInfo[(int)payload.resolution].width;
  const u16 height = Vision::CameraResInfo[(int)payload.resolution].height;
  const bool isImageReady = _imageDeChunker.AppendChunk(payload.imageId, 0, height, width,
    payload.imageEncoding,
    payload.imageChunkCount, payload.chunkId, payload.data.data(), (uint32_t)payload.data.size());

  if(isImageReady)
  {
    cv::Mat cvImg = _imageDeChunker.GetImage();

    if(cvImg.channels() == 1) {
      cvtColor(cvImg, cvImg, CV_GRAY2RGB);
    }
    // Delete existing image if there is one.
    if (camImg != nullptr) {
      camDisp->imageDelete(camImg);
    }

    //printf("Displaying image %d x %d\n", imgWidth, imgHeight);

    camImg = camDisp->imageNew(cvImg.cols, cvImg.rows, cvImg.data, webots::Display::RGB);
    camDisp->imagePaste(camImg, 0, 0);
  }
}


void VizControllerImpl::ProcessVizTrackerQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_TrackerQuad();
  camDisp->setColor(0x0000ff);
  camDisp->drawLine((int)payload.topLeft_x, (int)payload.topLeft_y, (int)payload.topRight_x, (int)payload.topRight_y);
  camDisp->setColor(0x00ff00);
  camDisp->drawLine((int)payload.topRight_x, (int)payload.topRight_y, (int)payload.bottomRight_x, (int)payload.bottomRight_y);
  camDisp->drawLine((int)payload.bottomRight_x, (int)payload.bottomRight_y, (int)payload.bottomLeft_x, (int)payload.bottomLeft_y);
  camDisp->drawLine((int)payload.bottomLeft_x, (int)payload.bottomLeft_y, (int)payload.topLeft_x, (int)payload.topLeft_y);
}

void VizControllerImpl::ProcessVizRobotStateMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_RobotStateMessage();
  char txt[128];

  sprintf(txt, "Pose: %6.1f, %6.1f, ang: %4.1f",
    payload.state.pose.x,
    payload.state.pose.y,
    RAD_TO_DEG_F32(payload.state.pose.angle));
  DrawText(VizTextLabelType::TEXT_LABEL_POSE, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Head: %5.1f deg, Lift: %4.1f mm",
    RAD_TO_DEG_F32(payload.state.headAngle),
    payload.state.liftHeight);
  DrawText(VizTextLabelType::TEXT_LABEL_HEAD_LIFT, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Pitch: %4.1f deg (IMUHead: %4.1f deg)",
    RAD_TO_DEG_F32(payload.state.pose.pitch_angle),
    RAD_TO_DEG_F32(payload.state.pose.pitch_angle + payload.state.headAngle));
  DrawText(VizTextLabelType::TEXT_LABEL_IMU, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Speed L: %4d  R: %4d mm/s",
    (int)payload.state.lwheel_speed_mmps,
    (int)payload.state.rwheel_speed_mmps);
  DrawText(VizTextLabelType::TEXT_LABEL_SPEEDS, Anki::NamedColors::GREEN, txt);

  /*
  sprintf(txt, "Prox: (%2u, %2u, %2u) %d%d%d",
    payload.state.proxLeft,
    payload.state.proxForward,
    payload.state.proxRight,
    payload.state.status & IS_PROX_SIDE_BLOCKED,
    payload.state.status & IS_PROX_FORWARD_BLOCKED,
    payload.state.status & IS_PROX_SIDE_BLOCKED);
  DrawText(VizTextLabelType::TEXT_LABEL_PROX_SENSORS, Anki::NamedColors::GREEN, txt);
  */

  sprintf(txt, "Batt: %2.1f V",
    (f32)payload.state.battVolt10x/10);
  DrawText(VizTextLabelType::TEXT_LABEL_BATTERY, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Video: %d HZ   AnimBufFree: %d",
    payload.videoFramefateHz, payload.numAnimBytesFree);
  DrawText(VizTextLabelType::TEXT_LABEL_VID_RATE, Anki::NamedColors::GREEN, txt);

  sprintf(txt, "Status: %5s %5s %7s",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_CARRYING_BLOCK ? "CARRY" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_PICKING_OR_PLACING ? "PAP" : "",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_PICKED_UP ? "PICKDUP" : "");
  DrawText(VizTextLabelType::TEXT_LABEL_STATUS_FLAG, Anki::NamedColors::GREEN, txt);

  char animLabel[16] = {0};
  if(payload.animTag == 255) {
    sprintf(animLabel, "ANIM_IDLE");
  } else if(payload.animTag != 0) {
    sprintf(animLabel, "ANIM[%d]", payload.animTag);
  }
  
  sprintf(txt, "    %10s %10s",
          animLabel,
          payload.state.status & (uint32_t)RobotStatusFlag::IS_CHARGING ? "CHARGING" :
          (payload.state.status & (uint32_t)RobotStatusFlag::IS_ON_CHARGER ? "ON_CHARGER" : ""));
  
  DrawText(VizTextLabelType::TEXT_LABEL_STATUS_FLAG_2, Anki::NamedColors::GREEN, txt);
  
  sprintf(txt, "        %7s %7s %6s",
    payload.state.status & (uint32_t)RobotStatusFlag::LIFT_IN_POS ? "" : "LIFTING",
    payload.state.status & (uint32_t)RobotStatusFlag::HEAD_IN_POS ? "" : "HEADING",
    payload.state.status & (uint32_t)RobotStatusFlag::IS_MOVING ? "MOVING" : "");
  DrawText(VizTextLabelType::TEXT_LABEL_STATUS_FLAG_3, Anki::NamedColors::GREEN, txt);
}

  
  
static const int kTextSpacingY = 10;
static const int kTextOffsetY  = -3;
  
  
// ========== Mood Display ==========
  
  
bool VizControllerImpl::IsMoodDisplayEnabled() const
{
  // maybe check settings or pixel size too?
  return ((behaviorDisp != nullptr) && (_emotionBuffers[0].capacity() > 0));
}


void VizControllerImpl::ProcessVizRobotMoodMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  if (!IsMoodDisplayEnabled())
  {
    return;
  }
  
  const VizInterface::RobotMood& robotMood = msg.GetData().Get_RobotMood();
  assert(robotMood.emotion.size() == (size_t)EmotionType::Count);
  
  const int windowWidth  = moodDisp->getWidth();
  const int windowHeight = moodDisp->getHeight();

  // Calculate y coordinate range and scaling for graph points
  
  const int   labelOffsetX  = 120; // Minimum indentation from right for the catagory label (e.g. "Happy X.XX")
  const float xStep         = float(windowWidth-labelOffsetX) / float(_emotionBuffers[0].capacity());
  
  const int   yValueFor1    = 16;
  const int   yValueForNeg1 = windowHeight - yValueFor1;
  const float yValueFor0    = float(yValueForNeg1 + yValueFor1) * 0.5f;
  const float yScalar       = float(yValueFor1) - yValueFor0; // y-is-down so larger y value = lower graph value
  
  // Clear Window
  
  moodDisp->setColor(0x000000);
  moodDisp->fillRectangle(0, 0, windowWidth, windowHeight);
  
  // Draw Graph Axis labels
  
  moodDisp->setColor(0xffffff);
  moodDisp->drawText("1.0",  0, yValueFor1 + kTextOffsetY);
  moodDisp->drawText("-1.0", 0, yValueForNeg1 + kTextOffsetY);
  
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
    
    moodDisp->setColor(0xffffff);
    float xValF = 0.0f;
    
    for (size_t j=0; j < _emotionEventBuffer.size(); ++j)
    {
      const std::vector<std::string>& eventsThisTick = _emotionEventBuffer[j];
      
      if (eventsThisTick.size() > 0)
      {
        const int xVal = (int)(xValF);
        
        for (const std::string& eventText : eventsThisTick)
        {
          moodDisp->drawLine(xVal, eventY, xVal, eventY + 30);
          moodDisp->drawText(eventText, xVal, eventY + kTextOffsetY);
          
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
  
    moodDisp->setColor( ColorRGBA::CreateFromColorIndex(eT).As0RGB() );
    
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
        moodDisp->drawLine(lastX, lastY, xVal, yVal);
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
    moodDisp->drawText(text, textX, textY + kTextOffsetY);
  }
}

  
// ========== BehaviorSelection Display ==========
  
  
bool VizControllerImpl::IsBehaviorDisplayEnabled() const
{
  // maybe check settings or pixel size too?
  return ((behaviorDisp != nullptr) && (_behaviorEventBuffer.capacity() > 0));
}

  
void VizControllerImpl::PreUpdateBehaviorDisplay()
{
  if (!IsBehaviorDisplayEnabled())
  {
    return;
  }
  
  // Advance all behaviors by one empty tick - any active ones will be updated later
  
  for (auto& kv : _behaviorScoreBuffers)
  {
    BehaviorScoreBuffer& behaviorScoreBuffer = kv.second;
    behaviorScoreBuffer.push_back( behaviorScoreBuffer.back() );
  }
  
  _behaviorEventBuffer.push_back(std::vector<std::string>()); // empty entry, expanded in other message
}

  
VizControllerImpl::BehaviorScoreBuffer& VizControllerImpl::FindOrAddScoreBuffer(const std::string& inName)
{
  BehaviorScoreBufferMap::iterator it = _behaviorScoreBuffers.find(inName);
  if (it != _behaviorScoreBuffers.end())
  {
    return it->second;
  }
  
  // Not found - add one and return that
  
  it = _behaviorScoreBuffers.insert(BehaviorScoreBufferMap::value_type(inName, BehaviorScoreBuffer(kBehaviorBuffersCapacity))).first;
  return it->second;
}


void VizControllerImpl::ProcessVizNewBehaviorSelectedMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  if (!IsBehaviorDisplayEnabled())
  {
    return;
  }
  
  const VizInterface::NewBehaviorSelected& selectData = msg.GetData().Get_NewBehaviorSelected();
  
  if (_behaviorEventBuffer.size() > 0)
  {
    std::vector<std::string>& latestEvents =_behaviorEventBuffer.back();
    
    if (!selectData.newCurrentBehavior.empty())
    {
      latestEvents.push_back(selectData.newCurrentBehavior);
    }
  }
}
  

void VizControllerImpl::ProcessVizRobotBehaviorSelectDataMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  if (!IsBehaviorDisplayEnabled())
  {
    return;
  }
  
  const VizInterface::RobotBehaviorSelectData& selectData = msg.GetData().Get_RobotBehaviorSelectData();
  
  // Build a sorted vector of NamedScoreBuffer containing all the behaviors currently being graphed, so that they're in
  // order of the most recent value, top-to-bottom in the graph
  
  for (const VizInterface::BehaviorScoreData& scoreData : selectData.scoreData)
  {
    BehaviorScoreBuffer& scoreBuffer = FindOrAddScoreBuffer(scoreData.name);
    if (!scoreBuffer.empty())
    {
      // Remove the dummy entry we added during preUpdate
      scoreBuffer.pop_back();
    }
    scoreBuffer.push_back(scoreData.totalScore);
  }
}
  
  
void VizControllerImpl::DrawBehaviorDisplay()
{
  if (!IsBehaviorDisplayEnabled())
  {
    return;
  }
  
  // Build a sorted vector of NamedScoreBuffer containing all the active behaviors, so that they're in
  // order of the most recent value, top-to-bottom in the graph
  
  struct NamedScoreBuffer
  {
    BehaviorScoreBuffer*  _scoreBuffer;
    const char*           _name;
    uint32_t              _color;
  };
  
  std::vector<NamedScoreBuffer> activeScoreBuffers;
  
  size_t maxBufferValues = 0;
  
  {
    uint32_t colorIndex = 0;
    
    for (auto& kv : _behaviorScoreBuffers)
    {
      BehaviorScoreBuffer& behaviorScoreBuffer = kv.second;
      
      maxBufferValues = MAX(maxBufferValues, behaviorScoreBuffer.size());
      activeScoreBuffers.push_back({&behaviorScoreBuffer,
                                    kv.first.c_str(),
                                    ColorRGBA::CreateFromColorIndex(colorIndex).As0RGB()});
      ++colorIndex;
    }
    
    maxBufferValues = MAX(maxBufferValues, _behaviorEventBuffer.size());
    
    std::sort(activeScoreBuffers.begin(), activeScoreBuffers.end(),
              [](const NamedScoreBuffer& lhs, const NamedScoreBuffer& rhs)
              {
                return lhs._scoreBuffer->back() > rhs._scoreBuffer->back();
              } );
  }
  
  // Draw everything
  
  const int windowWidth  = behaviorDisp->getWidth();
  const int windowHeight = behaviorDisp->getHeight();
  
  // Calculate y coordinate range and scaling for graph points
  
  const int yValueFor0 = windowHeight - 16;
  const int yValueFor1 = 16;
  float yScalar = (yValueFor1 - yValueFor0);
  
  // Clear Window
  
  behaviorDisp->setColor(0x000000);
  behaviorDisp->fillRectangle(0, 0, windowWidth, windowHeight);
  
  // Draw Graph Axis labels
  
  behaviorDisp->setColor(0xffffff);
  behaviorDisp->drawText("1.0", 0, yValueFor1 + kTextOffsetY);
  behaviorDisp->drawText("0.0", 0, yValueFor0 + kTextOffsetY);
  
  if (activeScoreBuffers.empty() || (activeScoreBuffers[0]._scoreBuffer->capacity() == 0))
  {
    return;
  }
  
  // Calculate line spacing and top/bottom range
  
  const int labelOffset = 170;
  const float xStep = float(windowWidth-labelOffset) / float(activeScoreBuffers[0]._scoreBuffer->capacity());
  
  const int textSpacingY = kTextSpacingY;
  
  const int kTopTextY    = (kTextSpacingY/2);
  const int kBottomTextY = windowHeight - (kTextSpacingY/2);
  
  int lastTextY = kTopTextY - textSpacingY;
  
  int numLinesLeft = (int)activeScoreBuffers.size();
  
  // Draw all the events
  {
    int eventY = kTopTextY;
    
    behaviorDisp->setColor(0xffffff);
    float xValF = 0.0f;
    
    for (size_t j=0; j < maxBufferValues; ++j)
    {
      const std::vector<std::string>& eventsThisTick = _behaviorEventBuffer[j];
      
      if (eventsThisTick.size() > 0)
      {
        const int xVal = (int)(xValF);
        
        for (const std::string& eventText : eventsThisTick)
        {
          behaviorDisp->drawLine(xVal, eventY, xVal, eventY + 30);
          behaviorDisp->drawText(eventText, xVal, eventY + kTextOffsetY);
          
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
  
  for (const NamedScoreBuffer& namedScoreBuffer : activeScoreBuffers)
  {
    const BehaviorScoreBuffer& scoreBuffer = *namedScoreBuffer._scoreBuffer;
    
    behaviorDisp->setColor(namedScoreBuffer._color);
    
    const size_t numValues = scoreBuffer.size();
    assert(numValues > 0);
    
    // Draw a line graph connecting all of the sample points
    
    float xValF = (xStep * float(maxBufferValues - numValues)); // start indented if behavior has fewer values than the max
    int lastX = 0;
    int lastY = 0;
    
    for (size_t j=0; j < numValues; ++j)
    {
      const float scoreVal = scoreBuffer[j];
      
      const int xVal = (int)(xValF);
      const int yVal = yValueFor0 + (int)(yScalar * scoreVal);
      
      if (j > 0)
      {
        behaviorDisp->drawLine(lastX, lastY, xVal, yVal);
      }
      
      xValF += xStep;
      lastX = xVal;
      lastY = yVal;
    }
    
    // Draw the label, ideally next to the last sample, but above maxTextY (so there's room for the rest of the labels)
    // and at least 1 line down from the last category, clamped to the top/bottom range
    
    const int textX = MIN(lastX, windowWidth-labelOffset);
    --numLinesLeft;
    const int maxTextY = kBottomTextY - (kTextSpacingY * numLinesLeft);
    const int textY = CLIP(MAX(MIN(lastY, maxTextY), lastTextY+kTextSpacingY), kTopTextY, kBottomTextY);
    lastTextY = textY;
    
    char valueString[32];
    snprintf(valueString, sizeof(valueString), "%1.2f: ", scoreBuffer.back());
    std::string text = std::string(valueString) + namedScoreBuffer._name;
    
    behaviorDisp->drawText(text, textX, textY + kTextOffsetY);
  }
}

  
// ========== Start/End of Robot Updates ==========
  

void VizControllerImpl::ProcessVizStartRobotUpdate(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  PreUpdateBehaviorDisplay();
}
  
  
void VizControllerImpl::ProcessVizEndRobotUpdate(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  // This signals end of the Robot::Update() and is where we tick and update the drawing for live graph windows etc.
  DrawBehaviorDisplay();
}
  
  
} // end namespace Cozmo
} // end namespace Anki
