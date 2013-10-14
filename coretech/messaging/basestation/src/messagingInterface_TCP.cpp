#include "anki/messaging/basestation/messagingInterface.h"

#ifdef USE_TCP_MESSAGING_INTERFACE

#include "anki/messaging/TcpServer.h"

namespace Anki {
  
  
  MessagingInterface_TCP::MessagingInterface_TCP()
  : MessagingInterface()
  {
    tx_ = getEmitter(TX);
    rx_ = getReceiver(RX);
    
    server_ = new TcpServer();
  }
  
  
  void MessagingInterface_TCP::Init()
  {
    rx_->enable(TIME_STEP);
    
    // Listen on all channels
    rx_->setChannel(BASESTATION_SIM_COMM_CHANNEL);
    
    // TODO: Create TCP server socket to listen for basestation connection
    server_->StartListening(COZMO_WORLD_LISTEN_PORT);
    
  } // MessagingInterface::Impl::Init()
  
  void MessagingInterface_TCP::Run()
  {
    
    int msgSize;
    const void *msg;
    while (step(TIME_STEP) != -1) {
      
      // Do we have a basestation connection?
      if (!server_->HasClient()) {
        if (!server_->Accept()) {
          continue;
        }
        
        if (server_->HasClient()) {
          // Once connected, clear all robot messages that may have been queueing up
          if (rx_->getQueueLength() > 0) {
            msg = rx_->getData();
            msgSize = rx_->getDataSize();
            rx_->nextPacket();
          }
        }
      }
      
      // Read receiver for as long as it is not empty.
      // Expecting only messages from robots that are bound for the basestation.
      while (rx_->getQueueLength() > 0) {
        
        // Get head packet
        msg = rx_->getData();
        msgSize = rx_->getDataSize();
        
        // Forward on to basestation
        server_->Send((char*)msg, msgSize);
        
        // Delete processed packet from queue
        rx_->nextPacket();
      }
      
      
      // Check TCP socket for inbound basestation messages
      // and relay to the destination robot.
      int bs_recvd = server_->Recv(recvBuf, BASESTATION_RECV_BUFFER_SIZE);
      unsigned char *p = (unsigned char*)recvBuf;
      
      while (bs_recvd > 0) {
#if(DEBUG_COZMO_WORLD_COMMS)
        std::cout << "BS Recvd msg\n";
#endif
        // Check if message contains 0xbeef
        if (bs_recvd >= 5) { // 2 bytes: 0xbeef, 1 byte robot ID, 1 byte message size, 1 byte message type
          if ((p[0] == 0xbe) && (p[1] == 0xef)) {
            // Valid beef header found!
            // Next message must indicate the destination robot
            int destRobot = p[2];
            tx_->setChannel(destRobot);
            
            // Next byte is size of message.
            // Check that we actually read enough of it.
            //int toRobotMsgSize = p[3];
            //if (toRobotMsgSize + 3 <=) {
            //  tx_->sendData(&p[3]);
            //}
            
          } else {
            std::cout << "ERROR: Invalid message. No beef found.\n";
            // TODO: Flush buffer until beef found...
          }
        } else {
          std::cout << "ERROR: Either somebody is sending messages that are too short or we need to not be lazy and handle partial messages here.\n";
        }
        
      }
    }
    
  } // MessagingInterface::Impl::Run()

  
  
} // namespace Anki



#endif // USE_TCP_MESSAGING_INTERFACE
