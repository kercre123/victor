#include <iostream>
#include "TcpServer.h"

using namespace std;

#define LISTEN_PORT 5556

// Echo server example
int main (int argc, char *argv[])
{
  char buf[2048];

  TcpServer server(LISTEN_PORT);
  server.StartListening();
  
  
  while (1) {

    if (!server.HasClient()) {
      if (!server.Accept()) {
        usleep(10000);
        continue;
      }
    }

    usleep(10000);
    int bytes_received = server.Recv(buf, sizeof(buf));
    if (bytes_received > 0) {
      server.Send(buf, bytes_received);
    }
  }

  server.DisconnectClient();
  server.StopListening();
 
  return(0);
}

