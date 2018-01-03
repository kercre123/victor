#include <iostream>
#include <unistd.h>
#include <vector>
#include "TcpMultiClientServer.h"

using namespace std;

#define LISTEN_PORT 5556

// Echo server example
int main (int argc, char *argv[])
{
  bool shutdown = false;
  char buf[2048];

  TcpMultiClientServer server;
  server.Start(LISTEN_PORT);
  if (!server.IsRunning()) {
    printf("Failed to listen on port %d\n", LISTEN_PORT);
    return -1;
  }
  cout << "Waiting for clients...\n";
  
  int num_clients = 0;
  vector<int> client_ids;
  
  while (!shutdown) {

    if (server.GetNumClients() != num_clients) {
      num_clients = server.GetConnectedClientIDs(client_ids);
      cout << "Num clients connected: " << num_clients << endl;
    }

    usleep(10000);
    for (int i=0; i< client_ids.size(); ++i) {
      memset(buf, 0, sizeof(buf));
      int bytes_received = server.Recv(client_ids[i], buf, sizeof(buf));
      if (bytes_received > 0) {
        if (buf[0] == 'q' && buf[1] == 'u' && buf[2] == 'i' && buf[3] == 't') {
          shutdown = true;
          break;
        }
        
        int bytes_sent = server.Send(client_ids[i], buf, bytes_received);
        cout << "Echoing " << buf << " (" << bytes_sent << " bytes)\n";
      }
    }

  }

  std::cout << "Shutting down\n";
  server.Stop();
 
  return(0);
}

