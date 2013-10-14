/*
 * File:          CozmoWorldComms.cpp
 * Date:          09-26-2013
 * Description:   Communication hub through which all messages travelling between robots and the basestation are routed,
 *                thus providing a single connection point through which a basestation can interact with a simulated world. 
 *  
 *                Messages travelling between CozmoWorldComms and a connected basestation have the same format as
 *                regular BTLE messages (see cozmoMsgProtocol.h) except that they are prepended with 3 bytes.
 *  
 *                byte 0: 0xbe
 *                byte 1: 0xef
 *                byte 2: destination channel (i.e. RobotID) of the message
 *  
 *                The robot-side communications interface is via Webots Emitter/Receiver and the message format is the same as
 *                regular messages.
 *  
 *                Basestation-side communications interface is via TCP socket.
 *                The basestation would need to invoke a CozmoWorldClient to communicate with simulated robots via TCP
 *                (instead of BTLE with physical robots).
 *  
 *                (If someone can figure out how to use the MacBookPro BTLE device to appear as if it's multiple robots,
 *                then we could potentially have a single basestation using only BTLE communicate with both physical and simulated robots!)
 * Author:        Kevin Yoon
 * Modifications: 
 */

#include "CozmoWorldComms.h"
#include "anki/cozmo/MessageProtocol.h"
#include "anki/messaging/TcpServer.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include <iostream>

#define TX "radio_tx"
#define RX "radio_rx"


CozmoWorldComms::CozmoWorldComms() : Supervisor() 
{
  tx_ = getEmitter(TX);
  rx_ = getReceiver(RX);

  server_ = new TcpServer();
}


void CozmoWorldComms::Init() 
{
  rx_->enable(Anki::Cozmo::TIME_STEP);

  // Listen on all channels
  rx_->setChannel(BASESTATION_SIM_COMM_CHANNEL);


  // TODO: Create TCP server socket to listen for basestation connection
  server_->StartListening(COZMO_WORLD_LISTEN_PORT);
}

void CozmoWorldComms::run()
{

  int msgSize;
  const void *msg;
  while (step(Anki::Cozmo::TIME_STEP) != -1) {

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

}

/*
int CozmoWorldComms::GetMsgWithHeader(const char* data, char* startOfMsg, int* robotID)
{

}

int CozmoWorldComms::PopFrontBuffer(char* data, const int numBytesToPop)
{}


*/
  



