/*
 * File:          cozmo_viz_controller.cpp
 * Date:          03-19-2014
 * Description:   Interface for basestation to all visualization functions in Webots including 
 *                cozmo_physics draw functions, display window text printing, and other custom display
 *                methods.
 * Author:        Kevin Yoon
 * Modifications: 
 */

#include <cstdio>
#include <string>
#include <webots/Supervisor.hpp>
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/VizStructs.h"
#include "anki/messaging/shared/UdpServer.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/vision/CameraSettings.h"

webots::Supervisor vizSupervisor;


namespace Anki {
  namespace Cozmo{
    

#define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"

    typedef void (*DispatchFcn_t)(const u8* buffer);
    
    const size_t NUM_TABLE_ENTRIES = Anki::Cozmo::NUM_VIZ_MSG_IDS + 1;
    DispatchFcn_t DispatchTable_[NUM_TABLE_ENTRIES] = {
      0, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_FCN_TABLE_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"
      0 // Final dummy entry without comma at end
    };
    
    namespace {
      // For displaying misc debug data
      webots::Display* disp;
      
      // For displaying docking data
      webots::Display* dockDisp;
      
      // For displaying images
      webots::Display* camDisp;
      
      // Image reference for display in camDisp
      webots::ImageRef* camImg = nullptr;
      
      // Image message processing
      u8 imgID = 0;
      u8 imgData[3*640*480];
      u32 imgBytes = 0;
      u32 imgWidth, imgHeight = 0;
      
      // Cozmo bots for visualization
      typedef struct  {
        webots::Supervisor* supNode = NULL;
        webots::Field* trans = NULL;
        webots::Field* rot = NULL;
        webots::Field* liftAngle = NULL;
        webots::Field* headAngle = NULL;
      } CozmoBotVizParams;
      
      // Vector of available CozmoBots for vizualization
      std::vector<CozmoBotVizParams> vizBots_;
      
      // Map of robotID to vizBot index
      std::map<u8, u8> robotIDToVizBotIdxMap_;
    }
    
    void Init()
    {
      // Get display devices
      disp = vizSupervisor.getDisplay("cozmo_viz_display");
      dockDisp = vizSupervisor.getDisplay("cozmo_docking_display");
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
    
    void SetRobotPose(CozmoBotVizParams *p,
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
    
    
    void ProcessVizSetRobotMessage(const VizSetRobot& msg)
    {
      // Find robot by ID
      u8 robotID = msg.robotID;
      std::map<u8, u8>::iterator it = robotIDToVizBotIdxMap_.find(robotID);
      if (it == robotIDToVizBotIdxMap_.end()) {
        if (robotIDToVizBotIdxMap_.size() < vizBots_.size()) {
          // Robot ID is not currently registered, but there are still some available vizBots.
          // Auto assign one here.
          robotIDToVizBotIdxMap_[robotID] = robotIDToVizBotIdxMap_.size();
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
                   msg.x_trans_m, msg.y_trans_m, msg.z_trans_m,
                   msg.rot_axis_x, msg.rot_axis_y, msg.rot_axis_z, msg.rot_rad,
                   msg.head_angle, msg.lift_angle);
    }
    
    void DrawText(u32 labelID, u32 color, const char* text)
    {
      const int baseXOffset = 8;
      const int baseYOffset = 8;
      const int yLabelStep = 10;  // Line spacing in pixels. Characters are 8x8 pixels in size.
      
      // Clear line specified by labelID
      disp->setColor(0x0);
      disp->fillRectangle(0, baseYOffset + yLabelStep * labelID, disp->getWidth(), 8);
      
      // Draw text
      disp->setColor(color >> 8);
      disp->setAlpha(static_cast<double>(0xff & color)/255.);
      
      std::string str(text);
      if(str.empty()) {
        str = " "; // Avoid webots warnings for empty text
      }
      disp->drawText(str, baseXOffset, baseYOffset + yLabelStep * labelID);
    }
    
    void DrawText(u32 labelID, const char* text) {
      DrawText(labelID, 0xffffff, text);
    }
    
    void ProcessVizSetLabelMessage(const VizSetLabel& msg)
    {
      DrawText(msg.labelID, msg.colorID, (char*)msg.text);
    }
    
    void ProcessVizDockingErrorSignalMessage(const VizDockingErrorSignal& msg)
    {
      // TODO: This can overlap with text being displayed. Create a dedicated display for it?
      
      // Pixel dimensions of display area
      const int baseXOffset = 8;
      const int baseYOffset = 40;
      const int rectW = 180;
      const int rectH = 180;
      const int halfBlockFaceLength = 20;
      
      const f32 MM_PER_PIXEL = 2.f;

      // Print values
      char text[111];
      sprintf(text, "ErrSig x: %.1f, y: %.1f, ang: %.2f\n", msg.x_dist, msg.y_dist, msg.angle);
      DrawText(msg.textLabelID, text);
      
      
      // Clear the space
      dockDisp->setColor(0x0);
      dockDisp->fillRectangle(baseXOffset, baseYOffset, rectW, rectH);
      
      dockDisp->setColor(0xffffff);
      dockDisp->drawRectangle(baseXOffset, baseYOffset, rectW, rectH);
      
      // Draw robot position
      dockDisp->drawOval(baseXOffset + 0.5f*rectW, baseYOffset + rectH, 3, 3);
      
      
      // Get pixel coordinates of block face center where
      int blockFaceCenterX = 0.5f*rectW - msg.y_dist / MM_PER_PIXEL;
      int blockFaceCenterY = rectH - msg.x_dist / MM_PER_PIXEL;
      
      // Check that center is within display area
      if (blockFaceCenterX < halfBlockFaceLength || (blockFaceCenterX > rectW - halfBlockFaceLength) ||
          blockFaceCenterY < halfBlockFaceLength || (blockFaceCenterY > rectH - halfBlockFaceLength) ) {
        return;
      }
      
      blockFaceCenterX += baseXOffset;
      blockFaceCenterY += baseYOffset;
      
      // Draw line representing the block face
      int dx = halfBlockFaceLength * cosf(msg.angle);
      int dy = -halfBlockFaceLength * sinf(msg.angle);
      dockDisp->drawLine(blockFaceCenterX + dx, blockFaceCenterY + dy, blockFaceCenterX - dx, blockFaceCenterY - dy);
      dockDisp->drawOval(blockFaceCenterX, blockFaceCenterY, 2, 2);
      
    }
    
    void ProcessVizVisionMarkerMessage(const VizVisionMarker& msg)
    {
      if(msg.verified) {
        camDisp->setColor(0xff0000);
      } else {
        camDisp->setColor(0x0000ff);
      }
      camDisp->drawLine(msg.topLeft_x, msg.topLeft_y, msg.bottomLeft_x, msg.bottomLeft_y);
      camDisp->drawLine(msg.bottomLeft_x, msg.bottomLeft_y, msg.bottomRight_x, msg.bottomRight_y);
      camDisp->drawLine(msg.bottomRight_x, msg.bottomRight_y, msg.topRight_x, msg.topRight_y);
      camDisp->drawLine(msg.topRight_x, msg.topRight_y, msg.topLeft_x, msg.topLeft_y);
    }
    
    void ProcessVizCameraQuadMessage(const VizCameraQuad& msg)
    {
      const f32 oneOver255 = 1.f / 255.f;
      
      camDisp->setColor(msg.color >> 8);
      u8 alpha = msg.color & 0xff;
      if(alpha < 0xff) {
        camDisp->setAlpha(oneOver255 * static_cast<f32>(alpha));
      }
      camDisp->drawLine(msg.xUpperLeft, msg.yUpperLeft, msg.xLowerLeft, msg.yLowerLeft);
      camDisp->drawLine(msg.xLowerLeft, msg.yLowerLeft, msg.xLowerRight, msg.yLowerRight);
      camDisp->drawLine(msg.xLowerRight, msg.yLowerRight, msg.xUpperRight, msg.yUpperRight);
      camDisp->drawLine(msg.xUpperRight, msg.yUpperRight, msg.xUpperLeft, msg.yUpperLeft);
    }
    
    void ProcessVizImageChunkMessage(const VizImageChunk& msg)
    {
      // If this is a new image, then reset everything
      if (msg.imgId != imgID) {
        //printf("Resetting image (img %d, res %d)\n", msg.imgId, msg.resolution);
        imgID = msg.imgId;
        imgBytes = 0;
        imgWidth = Vision::CameraResInfo[msg.resolution].width;
        imgHeight = Vision::CameraResInfo[msg.resolution].height;
      }
      
      // Copy chunk into the appropriate location in the imgData array.
      // Triplicate channels for viewability. (Webots only supports RGB)
      //printf("Processing chunk %d of size %d\n", msg.chunkId, msg.chunkSize);
      u8* chunkStart = imgData + 3 * msg.chunkId * MAX_VIZ_IMAGE_CHUNK_SIZE;
      for(int i=0; i<msg.chunkSize; ++i) {
        chunkStart[3*i] = msg.data[i];
        chunkStart[3*i+1] = msg.data[i];
        chunkStart[3*i+2] = msg.data[i];
      }
      
      // Do we have all the data for this image?
      imgBytes += msg.chunkSize;
      if (imgBytes < imgWidth * imgHeight) {
        return;
      }
      
      // Delete existing image if there is one.
      if (camImg != nullptr) {
        camDisp->imageDelete(camImg);
      }
      
      //printf("Displaying image %d x %d\n", imgWidth, imgHeight);
      
      camImg = camDisp->imageNew(imgWidth, imgHeight, imgData, webots::Display::RGB);
      camDisp->imagePaste(camImg, 0, 0);
    };
  
    
    void ProcessVizTrackerQuadMessage(const VizTrackerQuad& msg)
    {
      camDisp->setColor(0x0000ff);
      camDisp->drawLine(msg.topLeft_x, msg.topLeft_y, msg.topRight_x, msg.topRight_y);
      camDisp->setColor(0x00ff00);
      camDisp->drawLine(msg.topRight_x, msg.topRight_y, msg.bottomRight_x, msg.bottomRight_y);
      camDisp->drawLine(msg.bottomRight_x, msg.bottomRight_y, msg.bottomLeft_x, msg.bottomLeft_y);
      camDisp->drawLine(msg.bottomLeft_x, msg.bottomLeft_y, msg.topLeft_x, msg.topLeft_y);
    }
    
    
    // Stubs
    // These messages are handled by cozmo_physics.
    void ProcessVizObjectMessage(const VizObject& msg){};
    void ProcessVizQuadMessage(const VizQuad& msg){};
    void ProcessVizEraseQuadMessage(const VizEraseQuad& msg){};
    void ProcessVizErasePathMessage(const VizErasePath& msg){};
    void ProcessVizDefineColorMessage(const VizDefineColor& msg){};
    void ProcessVizEraseObjectMessage(const VizEraseObject& msg){};
    void ProcessVizSetPathColorMessage(const VizSetPathColor& msg){};
    void ProcessVizAppendPathSegmentLineMessage(const VizAppendPathSegmentLine& msg){};
    void ProcessVizAppendPathSegmentArcMessage(const VizAppendPathSegmentArc& msg){};
    void ProcessVizShowObjectsMessage(const VizShowObjects& msg){};
    
  }  // namespace Cozmo
} // namespace Anki



using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  const int maxPacketSize = MAX_VIZ_MSG_SIZE;
  char data[maxPacketSize];
  int numBytesRecvd;
  
  // Setup server to listen for commands
  UdpServer server;
  server.StartListening(Anki::Cozmo::VIZ_SERVER_PORT);
  
  
  // Setup client to forward relevant commands to cozmo_physics plugin
  UdpClient physicsClient;
  physicsClient.Connect("127.0.0.1", Anki::Cozmo::PHYSICS_PLUGIN_SERVER_PORT);
  
  Init();
  
  //
  // Main Execution loop
  //
  while (vizSupervisor.step(Anki::Cozmo::TIME_STEP) != -1)
  {
    // Any messages received?
    while ((numBytesRecvd = server.Recv(data, maxPacketSize)) > 0) {
      int msgID = static_cast<Anki::Cozmo::VizMsgID>(data[0]);
      //printf( "VizController: Got msg %d (%d bytes)\n", msgID, numBytesRecvd);
      
      switch(msgID)
      {
        // Messages that are handled in cozmo_viz_controller
        case VizSetLabel_ID:
        case VizDockingErrorSignal_ID:
        case VizImageChunk_ID:
        case VizSetRobot_ID:
        case VizTrackerQuad_ID:
        case VizVisionMarker_ID:
        case VizCameraQuad_ID:
          (*Anki::Cozmo::DispatchTable_[msgID])((unsigned char*)(data + 1));
          break;
        // All other messages are forwarded to cozmo_physics plugin
        default:
          physicsClient.Send(data, numBytesRecvd);
          break;
      }
      
    } // while server.Recv
    
  } // while step

  
  return 0;
}

