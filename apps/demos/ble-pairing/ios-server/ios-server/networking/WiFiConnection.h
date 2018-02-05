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

#define SB_PORT 5031
#define SB_BUFFER_SIZE 1024

namespace Anki {
  namespace Networking {
    
    class WifiConnection {
      using WiFiConnectedSignal = Signal::Signal<void (Anki::Networking::WiFiNetworkStream*)>;
      
    public:
      // Handle WiFi Client Connected
      WiFiConnectedSignal& OnWiFiClientConnectedEvent() {
        return _wifiClientConnectedSignal;
      }
      
      void StartServer();
      
    private:
      WiFiConnectedSignal _wifiClientConnectedSignal;
      uint16_t _numberConnectedClients = 0;
      
      static void AcceptCallback(struct ev_loop *loop, struct ev_io *watcher, int revents);
    };
  }
}

#endif /* WiFiConnection_hpp */
