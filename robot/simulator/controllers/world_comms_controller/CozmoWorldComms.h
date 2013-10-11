#ifndef COZMO_WORLD_COMMS_H
#define COZMO_WORLD_COMMS_H

#include <webots/Supervisor.hpp>

#define BASESTATION_RECV_BUFFER_SIZE 2048

#define DEBUG_COZMO_WORLD_COMMS 1

using namespace webots;

// Forward declarations
class TcpServer;

class CozmoWorldComms : public Supervisor
{

private:
  
  Emitter *tx_; 
  Receiver *rx_;

  TcpServer *server_;
  char recvBuf[BASESTATION_RECV_BUFFER_SIZE];

public:
  CozmoWorldComms();
  virtual ~CozmoWorldComms() {}

  void Init();
  void run();
};


#endif
