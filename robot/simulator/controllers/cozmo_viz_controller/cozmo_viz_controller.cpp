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

webots::Supervisor vizSupervisor;


namespace Anki {
  namespace Cozmo{
    
    // TODO: Move the following to VizStructs.cpp?
#define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
    
    typedef struct {
      u8 priority;
      u8 size;
      void (*dispatchFcn)(const u8* buffer);
    } TableEntry;
    
    const size_t NUM_TABLE_ENTRIES = Anki::Cozmo::NUM_VIZ_MSG_IDS + 1;
    TableEntry LookupTable_[NUM_TABLE_ENTRIES] = {
      {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
      {0, 0, 0} // Final dummy entry without comma at end
    };
    
    
    webots::Display* disp;
    void Init()
    {
      disp = vizSupervisor.getDisplay("cozmo_viz_display");
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
      const int blockFaceLength = 40;
      
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
      if (blockFaceCenterX < 0 || blockFaceCenterX > rectW ||
          blockFaceCenterY < 0 || blockFaceCenterY > rectH ) {
        return;
      }
      
      blockFaceCenterX += baseXOffset;
      blockFaceCenterY += baseYOffset;
      
      // Draw line representing the block face
      int dx = 0.5 * blockFaceLength * cosf(msg.angle);
      int dy = -0.5 * blockFaceLength * sinf(msg.angle);
      disp->drawLine(blockFaceCenterX + dx, blockFaceCenterY + dy, blockFaceCenterX - dx, blockFaceCenterY - dy);
      disp->drawOval(blockFaceCenterX, blockFaceCenterY, 2, 2);
      
    }
    
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
  const int maxPacketSize = 1024;
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
          (*Anki::Cozmo::LookupTable_[msgID].dispatchFcn)((unsigned char*)(data + 1));
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

