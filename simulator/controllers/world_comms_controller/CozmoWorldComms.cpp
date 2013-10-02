/*
 * File:          CozmoWorldComms.cpp
 * Date:          09-26-2013
 * Description:   Communication hub through which all messages travelling between robots and basestation go. 
 *  
 *                V2B messages have the same format as regular BTLE messages (see cozmoMsgProtocol.h).
 *                B2V messages are similar except that they are prepended with one byte indicating
 *                the destination channel (i.e. RobotID) of the message.
 *  
 *                Robot-side communications interface is via Webots Emitter/Receiver.
 *  
 *                Basestation-side communications interface is via Webots Emitter/Receiver for now so that it can talk to Matlab version of basestation.
 *  
 *                Eventually TCP socket support will be added to communicate with the actual C++ basestation.
 *                Basestation will need to have a switch to flip between communicating with physical robots via BTLE and
 *                simulated robots via TCP socket.
 *                (If someone can figure out how to use the MacBookPro BTLE device to appear as if it's multiple robots,
 *                then we could potentially have a single basestation using only BTLE communicate with both physical and simulated robots!)
 * Author:        Kevin Yoon
 * Modifications: 
 */

#include "CozmoWorldComms.h"
#include "comms/cozmoMsgProtocol.h"
#include "cozmoConfig.h"

#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <fcntl.h>

#define TX "radio_tx"
#define RX "radio_rx"



CozmoWorldComms::CozmoWorldComms() : Supervisor() 
{
  tx_ = getEmitter(TX);
  rx_ = getReceiver(RX);
}


void CozmoWorldComms::Init() 
{
  rx_->enable(TIME_STEP);

  // Listen on all channels
  rx_->setChannel(BASESTATION_SIM_COMM_CHANNEL);


  // TODO: Create TCP server socket to listen for basestation connection
  SetupListenSocket();
  bs_sd = -1;
}

void CozmoWorldComms::run()
{

  int msgSize;
  const void *msg;
  while (step(TIME_STEP) != -1) {

    // Do we have a basestation connection?
    if (bs_sd < 0) {
      // Blocking wait for a basestation connection
      WaitForBasestationConnection();

      if (bs_sd >= 0) {
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
      SendToBasestation((char*)msg, msgSize);

      // Delete processed packet from queue
      rx_->nextPacket();
    }


    // TODO: Check TCP socket for inbound basestation messages
    //       and relay to the destination robot.
    


    


  }
}


void set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    if (flags < 0)
    {
      printf("***ERROR (CozmoWorldComms): Failed to set socket to non-blocking. Aborting!\n");
      exit(-1);
    }
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

void CozmoWorldComms::SendToBasestation(char* data, int size)
{

}

int CozmoWorldComms::SetupListenSocket()
{
  int status;
  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.

  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
  // is created in c++, it will be given a block of memory. This memory is not nessesary
  // empty. Therefor we use the memset function to make sure all fields are NULL.
  memset(&host_info, 0, sizeof host_info);

  host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
  host_info.ai_flags = AI_PASSIVE;     // IP Wildcard

  // Now fill up the linked list of host_info structs with google's address information.
  status = getaddrinfo(NULL, COZMO_WORLD_LISTEN_PORT, &host_info, &host_info_list);
  // getaddrinfo returns 0 on succes, or some other value when an error occured.
  // (translated into human readable text by the gai_gai_strerror function).
  if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;


  std::cout << "Creating a socket..."  << std::endl;
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
  if (socketfd == -1)  std::cout << "socket error " ;

  std::cout << "Binding socket..."  << std::endl;
  // we use to make the setsockopt() function to make sure the port is not in use
  // by a previous execution of our code. (see man page for more information)
  int yes = 1;
  status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1)  std::cout << "bind error" << std::endl ;

  std::cout << "Listen()ing for connections..."  << std::endl;
  status =  listen(socketfd, 5);
  if (status == -1)  std::cout << "listen error" << std::endl ;


  return 0;
}

void CozmoWorldComms::DisconnectClient()
{
  close(bs_sd);
  bs_sd;
}


int CozmoWorldComms::RecvFromBasestation(char* data) 
{
    std::cout << "Waiting to recieve data..."  << std::endl;
    ssize_t bytes_recieved;
    char incomming_data_buffer[1000];
    bytes_recieved = recv(bs_sd, incomming_data_buffer,1000, 0);
    // If no data arrives, the program will just wait here until some data arrives.
    if (bytes_recieved == 0) std::cout << "host shut down." << std::endl ;
    if (bytes_recieved == -1)std::cout << "recieve error!" << std::endl ;
    std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
    incomming_data_buffer[bytes_recieved] = '\0';
    std::cout << incomming_data_buffer << std::endl;


    std::cout << "send()ing back a message..."  << std::endl;
    const char *msg = "thank you.";
    int len;
    ssize_t bytes_sent;
    len = strlen(msg);
    bytes_sent = send(bs_sd, msg, len, 0);

    std::cout << "Stopping server..." << std::endl;
    freeaddrinfo(host_info_list);
    
    close(socketfd);
}


int CozmoWorldComms::WaitForBasestationConnection()
{
    
    int new_sd;
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    new_sd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_sd == -1)
    {
        std::cout << "listen error" << std::endl ;
    }
    else
    {
        std::cout << "Connection accepted. Using new socketfd : "  <<  new_sd << std::endl;
    }

    set_nonblock(new_sd);

    bs_sd = new_sd;

    return 0 ;
}
