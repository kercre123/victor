#ifndef COZMO_WORLD_COMMS_H
#define COZMO_WORLD_COMMS_H

#include <webots/Supervisor.hpp>

using namespace webots;

class CozmoWorldComms : public Supervisor
{

private:
  
  Emitter *tx_; 
  Receiver *rx_;


  // Basestation socket descriptor
  int socketfd ; // The socket descripter
  int bs_sd; 
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  int SetupListenSocket();
  void DisconnectClient();
  int WaitForBasestationConnection();
  int RecvFromBasestation(char* data);
  void SendToBasestation(char* data, int size);

public:
  CozmoWorldComms();
  virtual ~CozmoWorldComms() {}

  void Init();
  void run();
};


#endif
