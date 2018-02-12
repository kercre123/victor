//
//  WiFiConnection.hpp
//  ios-server
//
//  Created by Paul Aluri on 2/2/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef WiFiConnection_hpp
#define WiFiConnection_hpp

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "../libev/ev.h"
#include "../libev/ev++.h"
#include "WiFiNetworkStream.h"

#define SB_BUFFER_SIZE 1024

namespace Anki {
namespace Switchboard {
  class WifiServer {
    using ClientConnectedSignal = Signal::Signal<void (Anki::Switchboard::WiFiNetworkStream*)>;
    using AcceptedSocketSignal = Signal::Signal<void (struct sockaddr* client_address, int size)>;
    
  public:
    WifiServer(struct ev_loop* loop, int port);
    
    // Handle WiFi Client Connected
    ClientConnectedSignal& OnWifiClientConnectedEvent() {
      return _wifiClientConnectedSignal;
    }
    
    void StartServer();
    
  private:
    ClientConnectedSignal _wifiClientConnectedSignal;
    uint16_t _numberConnectedClients = 0;
    
    struct ev_loop* _Loop;
    int _Port = 0;
    
    static void AcceptCallback(struct ev_loop *loop, struct ev_io *watcher, int revents);
  };
} // Switchboard
} // Anki

#endif /* WiFiConnection_hpp */
