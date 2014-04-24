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
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/VizStructs.h"
#include "anki/messaging/shared/UdpServer.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/vision/CameraSettings.h"

webots::Supervisor vizSupervisor;


namespace Anki {
  namespace Cozmo{
    

#define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"

    typedef void (*DispatchFcn_t)(const u8* buffer);
    
    const size_t NUM_TABLE_ENTRIES = Anki::Cozmo::NUM_VIZ_MSG_IDS + 1;
    DispatchFcn_t DispatchTable_[NUM_TABLE_ENTRIES] = {
      0, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_FCN_TABLE_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
      0 // Final dummy entry without comma at end
    };
    
    namespace {
      // For displaying misc debug data
      webots::Display* disp;
      
      // For displaying images
      webots::Display* camDisp;
      
      // Image reference for display in camDisp
      webots::ImageRef* camImg = nullptr;
      
      // Image message processing
      u8 imgID = 0;
      u8 imgData[3*320*240];
      u32 imgBytes = 0;
      u32 imgWidth, imgHeight = 0;
    }
    
    void Init()
    {
      disp = vizSupervisor.getDisplay("cozmo_viz_display");
      camDisp = vizSupervisor.getDisplay("cozmo_cam_viz_display");
    }
    

    
    void DrawText(u32 labelID, const char* text)
    {
      const int baseXOffset = 8;
      const int baseYOffset = 8;
      const int yLabelStep = 10;  // Line spacing in pixels. Characters are 8x8 pixels in size.
      
      // Clear line specified by labelID
      disp->setColor(0x0);
      disp->fillRectangle(0, baseYOffset + yLabelStep * labelID, disp->getWidth(), 8);
      
      // Draw text
      disp->setColor(0xffffff);
      disp->drawText(std::string(text), baseXOffset, baseYOffset + yLabelStep * labelID);
    }
    
    void ProcessVizSetLabelMessage(const VizSetLabel& msg)
    {
      DrawText(msg.labelID, (char*)msg.text);
    }
    
    void ProcessVizDockingErrorSignalMessage(const VizDockingErrorSignal& msg)
    {
      // TODO: This can overlap with text being displayed. Create a dedicated display for it?
      
      // Pixel dimensions of display area
      const int baseXOffset = 8;
      const int baseYOffset = 60;
      const int rectW = 130;
      const int rectH = 130;
      const int halfBlockFaceLength = 20;
      
      const f32 MM_PER_PIXEL = 2.f;

      // Print values
      char text[111];
      sprintf(text, "ErrSig x: %.1f, y: %.1f, ang: %.2f\n", msg.x_dist, msg.y_dist, msg.angle);
      DrawText(3, text);
      
      
      // Clear the space
      disp->setColor(0x0);
      disp->fillRectangle(baseXOffset, baseYOffset, rectW, rectH);
      
      disp->setColor(0xffffff);
      disp->drawRectangle(baseXOffset, baseYOffset, rectW, rectH);
      
      // Draw robot position
      disp->drawOval(baseXOffset + 0.5f*rectW, baseYOffset + rectH, 3, 3);
      
      
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
      disp->drawLine(blockFaceCenterX + dx, blockFaceCenterY + dy, blockFaceCenterX - dx, blockFaceCenterY - dy);
      disp->drawOval(blockFaceCenterX, blockFaceCenterY, 2, 2);
      
    }
    
    void ProcessVizImageChunkMessage(const VizImageChunk& msg)
    {
      // If this is a new image, then reset everything
      if (msg.imgId != imgID) {
        printf("Resetting image (img %d, res %d)\n", msg.imgId, msg.resolution);
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
      
      printf("Displaying image %d x %d\n", imgWidth, imgHeight);
      
      camImg = camDisp->imageNew(imgWidth, imgHeight, imgData, webots::Display::RGB);
      camDisp->imagePaste(camImg, 0, 0);
    };
  
    
    
    // Stubs
    // These messages are handled by cozmo_physics.
    void ProcessVizObjectMessage(const VizObject& msg){};
    void ProcessVizErasePathMessage(const VizErasePath& msg){};
    void ProcessVizDefineColorMessage(const VizDefineColor& msg){};
    void ProcessVizEraseObjectMessage(const VizEraseObject& msg){};
    void ProcessVizSetPathColorMessage(const VizSetPathColor& msg){};
    void ProcessVizAppendPathSegmentLineMessage(const VizAppendPathSegmentLine& msg){};
    void ProcessVizAppendPathSegmentArcMessage(const VizAppendPathSegmentArc& msg){};
    
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

