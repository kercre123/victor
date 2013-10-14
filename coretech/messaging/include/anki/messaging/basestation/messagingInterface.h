
#ifndef ANKI_MESSAGING_INTERFACE_H
#define ANKI_MESSAGING_INTERFACE_H

#include <memory>

#include "anki/common/types.h"

//#define USE_TCP_MESSAGING_INTERFACE
//#define USE_BTLE_MESSAGING_INTERFACE

namespace Anki
{
  
  class MessagingInterface
  {
  public:
    
    MessagingInterface();
    
    virtual void Init() = 0;
    virtual void Run()  = 0;
    
  };
  
#ifdef USE_TCP_MESSAGING_INTERFACE

#include "webots/Supervisor.hpp"

#define DEBUG_TCP_MESSAGING_INTERFACE 1
  
  const int BASESTATION_RECV_BUFFER_SIZE = 2048;
  
  // Forward declarations
  class TcpServer;
  
  class MessagingInterface_TCP : public MessagingInterface
  {
  public:
    
    MessagingInterface_TCP();
    
    virtual void Init();
    virtual void Run();
    
  private:
    webots::Supervisor webotsRobot;
    webots::Emitter  *tx_;
    webots::Receiver *rx_;
    
    TcpServer *server_;
    char recvBuf[BASESTATION_RECV_BUFFER_SIZE];
    
  }; // class MessagingInterface_TCP
  
#endif // USE_TCP_MESSAGING_INTERFACE
  

} // namespace Anki

#endif // ANKI_MESSAGING_INTERFACE_H