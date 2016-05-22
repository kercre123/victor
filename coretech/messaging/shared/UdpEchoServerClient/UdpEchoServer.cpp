#include <iostream>
#include <unistd.h>
#include "UdpServer.h"

using namespace std;

#define LISTEN_PORT 5557

// Echo server example
int main (int argc, char *argv[])
{
  char buf[2048];

  UdpServer server;
  if (!server.StartListening(LISTEN_PORT)) {
    printf("Failed to listen on port %d\n", LISTEN_PORT);
    return -1;
  }
  std::cout << "Waiting for client...\n";
  
  while (1) {

    usleep(10000);
    int bytes_received = server.Recv(buf, sizeof(buf));
    if (bytes_received > 0) {
      std::cout << "Echoing " << buf << "\n";
      int bytes_sent = server.Send(buf, bytes_received);
    }
  }

  std::cout << "Shutting down\n";
  server.StopListening();
 
  return(0);
}

